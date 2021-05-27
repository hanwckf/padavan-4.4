/*
 * Copyright (C) 2018 Felix Fietkau <nbd@nbd.name>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/netfilter.h>
#include <linux/netfilter/xt_FLOWOFFLOAD.h>
#include <net/ip.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_flow_table.h>

static struct nf_flowtable nf_flowtable;
static HLIST_HEAD(hooks);
static DEFINE_SPINLOCK(hooks_lock);
static struct delayed_work hook_work;

struct xt_flowoffload_hook {
	struct hlist_node list;
	struct nf_hook_ops ops;
	struct net *net;
	bool registered;
	bool used;
};

static unsigned int
xt_flowoffload_net_hook(void *priv, struct sk_buff *skb,
			  const struct nf_hook_state *state)
{
	switch (skb->protocol) {
	case htons(ETH_P_IP):
		return nf_flow_offload_ip_hook(priv, skb, state);
	case htons(ETH_P_IPV6):
		return nf_flow_offload_ipv6_hook(priv, skb, state);
	}

	return NF_ACCEPT;
}

static int
xt_flowoffload_create_hook(const struct net_device *dev)
{
	struct xt_flowoffload_hook *hook;
	struct nf_hook_ops *ops;

	hook = kzalloc(sizeof(*hook), GFP_ATOMIC);
	if (!hook)
		return -ENOMEM;

	ops = &hook->ops;
	ops->pf = NFPROTO_NETDEV;
	ops->hooknum = NF_NETDEV_INGRESS;
	ops->priority = 10;
	ops->priv = &nf_flowtable;
	ops->hook = xt_flowoffload_net_hook;
	ops->dev = dev;

	hlist_add_head(&hook->list, &hooks);
	mod_delayed_work(system_power_efficient_wq, &hook_work, 0);

	return 0;
}

static struct xt_flowoffload_hook *
xt_flowoffload_lookup_hook(const struct net_device *dev)
{
	struct xt_flowoffload_hook *hook;

	hlist_for_each_entry(hook, &hooks, list) {
		if (hook->ops.dev == dev)
			return hook;
	}

	return NULL;
}

static void
xt_flowoffload_check_device(const struct net_device *dev)
{
	struct xt_flowoffload_hook *hook;

	spin_lock_bh(&hooks_lock);
	hook = xt_flowoffload_lookup_hook(dev);
	if (hook)
		hook->used = true;
	else
		xt_flowoffload_create_hook(dev);
	spin_unlock_bh(&hooks_lock);
}

static void
xt_flowoffload_register_hooks(void)
{
	struct xt_flowoffload_hook *hook;

restart:
	hlist_for_each_entry(hook, &hooks, list) {
		if (hook->registered)
			continue;

		hook->registered = true;
		hook->net = dev_net(hook->ops.dev);
		spin_unlock_bh(&hooks_lock);
		nf_register_net_hook(hook->net, &hook->ops);
		spin_lock_bh(&hooks_lock);
		goto restart;
	}

}

static void
xt_flowoffload_cleanup_hooks(void)
{
	struct xt_flowoffload_hook *hook;

restart:
	hlist_for_each_entry(hook, &hooks, list) {
		if (hook->used || !hook->registered)
			continue;

		hlist_del(&hook->list);
		spin_unlock_bh(&hooks_lock);
		nf_unregister_net_hook(hook->net, &hook->ops);
		kfree(hook);
		spin_lock_bh(&hooks_lock);
		goto restart;
	}

}

static void
xt_flowoffload_check_hook(struct flow_offload *flow, void *data)
{
	struct flow_offload_tuple *tuple = &flow->tuplehash[0].tuple;
	struct xt_flowoffload_hook *hook;
	bool *found = data;

	spin_lock_bh(&hooks_lock);
	hlist_for_each_entry(hook, &hooks, list) {
		if (hook->ops.dev->ifindex != tuple->iifidx &&
		    hook->ops.dev->ifindex != tuple->oifidx)
			continue;

		hook->used = true;
		*found = true;
	}
	spin_unlock_bh(&hooks_lock);
}

static void
xt_flowoffload_hook_work(struct work_struct *work)
{
	struct xt_flowoffload_hook *hook;
	bool found = false;
	int err;

	spin_lock_bh(&hooks_lock);
	xt_flowoffload_register_hooks();
	hlist_for_each_entry(hook, &hooks, list)
		hook->used = false;
	spin_unlock_bh(&hooks_lock);

	err = nf_flow_table_iterate(&nf_flowtable, xt_flowoffload_check_hook,
				    &found);
	if (err && err != -EAGAIN)
	    goto out;

	spin_lock_bh(&hooks_lock);
	xt_flowoffload_cleanup_hooks();
	spin_unlock_bh(&hooks_lock);

out:
	if (found)
		queue_delayed_work(system_power_efficient_wq, &hook_work, HZ);
}

static bool
xt_flowoffload_skip(struct sk_buff *skb)
{
	struct ip_options *opt = &(IPCB(skb)->opt);

	if (unlikely(opt->optlen))
		return true;
	if (skb_sec_path(skb))
		return true;

	return false;
}

static struct dst_entry *
xt_flowoffload_dst(const struct nf_conn *ct, enum ip_conntrack_dir dir,
		   const struct xt_action_param *par)
{
	struct dst_entry *dst = NULL;
	struct flowi fl;
	u_int8_t pf;
	const struct nf_afinfo *afinfo;

	memset(&fl, 0, sizeof(fl));
	switch (par->family) {
	case NFPROTO_IPV4:
		fl.u.ip4.daddr = ct->tuplehash[dir].tuple.src.u3.ip;
		pf = AF_INET;
		break;
	case NFPROTO_IPV6:
		fl.u.ip6.saddr = ct->tuplehash[dir].tuple.dst.u3.in6;
		fl.u.ip6.daddr = ct->tuplehash[dir].tuple.src.u3.in6;
		pf = AF_INET6;
		break;
	}
	rcu_read_lock();
	afinfo = nf_get_afinfo(pf);
	if (afinfo)
		afinfo->route(par->net, &dst, &fl, false);
	rcu_read_unlock();

	return dst;
}

static int
xt_flowoffload_route(struct sk_buff *skb, const struct nf_conn *ct,
		   const struct xt_action_param *par,
		   struct nf_flow_route *route, enum ip_conntrack_dir dir)
{
	struct dst_entry *this_dst, *other_dst;

	this_dst = xt_flowoffload_dst(ct, dir, par);
	other_dst = xt_flowoffload_dst(ct, !dir, par);
	if (!this_dst || !other_dst)
		return -ENOENT;

	if (dst_xfrm(this_dst) || dst_xfrm(other_dst))
		return -EINVAL;

	route->tuple[dir].dst		= this_dst;
	route->tuple[dir].ifindex	= par->in->ifindex;
	route->tuple[!dir].dst		= other_dst;
	route->tuple[!dir].ifindex	= par->out->ifindex;

	return 0;
}

static unsigned int
flowoffload_tg(struct sk_buff *skb, const struct xt_action_param *par)
{
	const struct xt_flowoffload_target_info *info = par->targinfo;
	enum ip_conntrack_info ctinfo;
	enum ip_conntrack_dir dir;
	struct nf_flow_route route;
	struct flow_offload *flow;
	struct nf_conn *ct;

	if (xt_flowoffload_skip(skb))
		return XT_CONTINUE;

	ct = nf_ct_get(skb, &ctinfo);
	if (ct == NULL)
		return XT_CONTINUE;

	switch (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum) {
	case IPPROTO_TCP:
		if (ct->proto.tcp.state != TCP_CONNTRACK_ESTABLISHED)
			return XT_CONTINUE;
		break;
	case IPPROTO_UDP:
		break;
	default:
		return XT_CONTINUE;
	}

	if (test_bit(IPS_HELPER_BIT, &ct->status))
		return XT_CONTINUE;

	if (ctinfo == IP_CT_NEW ||
	    ctinfo == IP_CT_RELATED)
		return XT_CONTINUE;

	if (!par->in || !par->out)
		return XT_CONTINUE;

	if (test_and_set_bit(IPS_OFFLOAD_BIT, &ct->status))
		return XT_CONTINUE;

	dir = CTINFO2DIR(ctinfo);

	if (xt_flowoffload_route(skb, ct, par, &route, dir) < 0)
		goto err_flow_route;

	flow = flow_offload_alloc(ct, &route);
	if (!flow)
		goto err_flow_alloc;

	if (flow_offload_add(&nf_flowtable, flow) < 0)
		goto err_flow_add;

	xt_flowoffload_check_device(par->in);
	xt_flowoffload_check_device(par->out);

	/*only accelerate traffic from assigned direction*/
	if (info->dir)
		flow->dir = info->dir;
	else
		flow->dir = 1 << FLOW_OFFLOAD_DIR_ORIGINAL |
			    1 << FLOW_OFFLOAD_DIR_REPLY;

	if (info->flags & XT_FLOWOFFLOAD_HW)
		nf_flow_offload_hw_add(par->net, flow, ct);

	return XT_CONTINUE;

err_flow_add:
	flow_offload_free(flow);
err_flow_alloc:
	dst_release(route.tuple[!dir].dst);
err_flow_route:
	clear_bit(IPS_OFFLOAD_BIT, &ct->status);
	return XT_CONTINUE;
}


static int flowoffload_chk(const struct xt_tgchk_param *par)
{
	struct xt_flowoffload_target_info *info = par->targinfo;

	if (info->flags & ~XT_FLOWOFFLOAD_MASK)
		return -EINVAL;

	return 0;
}

static struct xt_target offload_tg_reg __read_mostly = {
	.family		= NFPROTO_UNSPEC,
	.name		= "FLOWOFFLOAD",
	.revision	= 0,
	.targetsize	= sizeof(struct xt_flowoffload_target_info),
	.checkentry	= flowoffload_chk,
	.target		= flowoffload_tg,
	.me		= THIS_MODULE,
};

static int xt_flowoffload_table_init(struct nf_flowtable *table)
{
//	table->flags = NF_FLOWTABLE_F_HW;
	nf_flow_table_init(table);
	return 0;
}

static void xt_flowoffload_table_cleanup(struct nf_flowtable *table)
{
	nf_flow_table_free(table);
}

static int xt_flowoffload_netdev_event(struct notifier_block *this,
				     unsigned long event, void *ptr)
{
	struct xt_flowoffload_hook *hook = NULL;
	struct net_device *dev = netdev_notifier_info_to_dev(ptr);

	if (event != NETDEV_UNREGISTER)
		return NOTIFY_DONE;

	spin_lock_bh(&hooks_lock);
	hook = xt_flowoffload_lookup_hook(dev);
	if (hook) {
		hlist_del(&hook->list);
	}
	spin_unlock_bh(&hooks_lock);
	if (hook) {
		nf_unregister_net_hook(hook->net, &hook->ops);
		kfree(hook);
	}

	nf_flow_table_cleanup(dev_net(dev), dev);

	return NOTIFY_DONE;
}

static struct notifier_block xt_flowoffload_netdev_notifier = {
	.notifier_call	= xt_flowoffload_netdev_event,
};

static int __init xt_flowoffload_tg_init(void)
{
	int ret;

	register_netdevice_notifier(&xt_flowoffload_netdev_notifier);

	INIT_DELAYED_WORK(&hook_work, xt_flowoffload_hook_work);

	ret = xt_flowoffload_table_init(&nf_flowtable);
	if (ret)
		return ret;

	ret = xt_register_target(&offload_tg_reg);
	if (ret)
		xt_flowoffload_table_cleanup(&nf_flowtable);

	return ret;
}

static void xt_flowoffload_tg_exit(void)
{
	xt_unregister_target(&offload_tg_reg);
	xt_flowoffload_table_cleanup(&nf_flowtable);
	unregister_netdevice_notifier(&xt_flowoffload_netdev_notifier);
}


MODULE_LICENSE("GPL");
module_init(xt_flowoffload_tg_init);
module_exit(xt_flowoffload_tg_exit);
