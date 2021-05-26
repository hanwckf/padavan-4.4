#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netlink.h>
#include <net/netlink.h>
#include <net/ip.h>
#include <linux/netfilter/nfnetlink.h>
#include <net/netfilter/nf_flow_table.h>

enum ft_netlink_msg_types {
	FT_MSG_DEL,
	FT_MSG_ADD,
	FT_MSG_FLUSH,
	FT_MSG_MAX
};

enum ftattr_type {
	FTA_UNSPEC,
	FTA_TUPLE,
	__FTA_MAX
};
#define FTA_MAX (__FTA_MAX - 1)

enum ftattr_tuple {
	FTA_TUPLE_UNSPEC,
	FTA_TUPLE_IP,
	FTA_TUPLE_PROTO,
	FTA_TUPLE_ZONE,
	__FTA_TUPLE_MAX
};
#define FTA_TUPLE_MAX (__FTA_TUPLE_MAX - 1)

enum ftattr_ip {
	FTA_IP_UNSPEC,
	FTA_IP_V4_SRC,
	FTA_IP_V4_DST,
	__FTA_IP_MAX
};
#define FTA_IP_MAX (__FTA_IP_MAX - 1)

enum ftattr_l4proto {
	FTA_PROTO_UNSPEC,
	FTA_PROTO_NUM,
	FTA_PROTO_SPORT,
	FTA_PROTO_DPORT,
	__FTA_PROTO_MAX
};
#define FTA_PROTO_MAX (__FTA_PROTO_MAX - 1)

static const struct nla_policy tuple_nla_policy[FTA_TUPLE_MAX+1] = {
	[FTA_TUPLE_IP]		= { .type = NLA_NESTED },
	[FTA_TUPLE_PROTO]	= { .type = NLA_NESTED },
	[FTA_TUPLE_ZONE]	= { .type = NLA_U16 },
};

static const struct nla_policy ip_nla_policy[FTA_IP_MAX+1] = {
	[FTA_IP_V4_SRC]	= { .type = NLA_U32 },
	[FTA_IP_V4_DST]	= { .type = NLA_U32 },
};

static const struct nla_policy l4proto_nla_policy[FTA_PROTO_MAX+1] = {
	[FTA_PROTO_NUM]	= { .type = NLA_U8 },
	[FTA_PROTO_SPORT] = {.type = NLA_U16},
	[FTA_PROTO_DPORT] = {.type = NLA_U16},
};

static inline int ftnetlink_parse_tuple_ip(struct nlattr *attr,
					   struct flow_offload_tuple *tuple)
{
	struct nlattr *tb[FTA_IP_MAX+1];
	int err;

	err = nla_parse_nested(tb, FTA_IP_MAX, attr, ip_nla_policy);

	if (err < 0)
		return err;

	switch (tuple->l3proto) {
	case NFPROTO_IPV4:
		if (!tb[FTA_IP_V4_SRC] || !tb[FTA_IP_V4_DST])
			return -EINVAL;
		tuple->src_v4.s_addr = nla_get_in_addr(tb[FTA_IP_V4_SRC]);
		tuple->dst_v4.s_addr = nla_get_in_addr(tb[FTA_IP_V4_DST]);
	}

	return err;
	
}

static inline int ftnetlink_parse_tuple_proto(struct nlattr *attr,
					      struct flow_offload_tuple *tuple)
{
	struct nlattr *tb[FTA_PROTO_MAX+1];
	int err;

	err = nla_parse_nested(tb, FTA_PROTO_MAX, attr, l4proto_nla_policy);

	if(err < 0)
		return err;
	
	if (!tb[FTA_PROTO_NUM] || !tb[FTA_PROTO_SPORT] || !tb[FTA_PROTO_DPORT])
		return -EINVAL;

	tuple->l4proto = nla_get_u8(tb[FTA_PROTO_NUM]);
	tuple->src_port = nla_get_u16(tb[FTA_PROTO_SPORT]);
	tuple->dst_port = nla_get_u16(tb[FTA_PROTO_DPORT]);

	return err;
}

static int ftnetlink_parse_tuple(const struct nlattr * const cda[],
				 struct flow_offload_tuple *tuple,
				 int attrtype, int l3proto)
{
	struct nlattr *tb[FTA_TUPLE_MAX+1];
	int err;

	memset(tuple, 0, sizeof(*tuple));

	err = nla_parse_nested(tb, FTA_TUPLE_MAX, cda[attrtype], tuple_nla_policy);
	if (err < 0)
		return err;

	if (!tb[FTA_TUPLE_IP])
		return -EINVAL;

	/*parse IP*/
	tuple->l3proto = l3proto;
	err = ftnetlink_parse_tuple_ip(tb[FTA_TUPLE_IP], tuple);
	if (err < 0)
		return err;

	/*parse proto*/
	if (!tb[FTA_TUPLE_PROTO])
		return -EINVAL;
	err = ftnetlink_parse_tuple_proto(tb[FTA_TUPLE_PROTO],tuple);

	if (err >= 0)
		printk("tuple info:sip=%pI4,dip=%pI4 proto=%d "
		       "sport=%d dport=%d\n",
		       &tuple->src_v4, &tuple->dst_v4, tuple->l4proto,
		       ntohs(tuple->src_port), ntohs(tuple->dst_port));

	return err;
}

static int ftnetlink_del_nf_flow(struct sock *ftnl, struct sk_buff *skb, 
			       const struct nlmsghdr *nlh,
			       const struct nlattr * const cda[])
{
	struct flow_offload_tuple tuple;
	int err = -1;
	struct nfgenmsg *nfmsg = nlmsg_data(nlh);
	u_int8_t u3 = nfmsg->nfgen_family;

	/*parse tuple*/
	if(!cda[FTA_TUPLE]) 
		return -EINVAL;

	err = ftnetlink_parse_tuple(cda, &tuple, FTA_TUPLE, u3);
	if (err < 0)
		return err;

	/*teardown the flow*/
	flow_offload_teardown_by_tuple(&tuple);
	return 0;
}

static int ftnetlink_add_nf_flow(struct sock *ftnl, struct sk_buff *skb, 
			       const struct nlmsghdr *nlh,
			       const struct nlattr * const cda[])
{
	return 0;
}

static int ftnetlink_flush_table(struct sock *ftnl, struct sk_buff *skb, 
			       const struct nlmsghdr *nlh,
			       const struct nlattr * const cda[])
{
	struct net *net = sock_net(ftnl);

	nf_flow_table_cleanup(net, NULL);
	return 0;
}

static const struct nla_policy ft_nla_policy[FTA_MAX+1] = {
	[FTA_TUPLE] = { .type = NLA_NESTED },
};

static const struct nfnl_callback flow_table_cb[FT_MSG_MAX] = {
	[FT_MSG_DEL] = {
		.call = ftnetlink_del_nf_flow,
		.attr_count = FTA_MAX,
		.policy = ft_nla_policy
	},
	[FT_MSG_ADD] = {
		.call = ftnetlink_add_nf_flow,
		.attr_count = FTA_MAX,
		.policy = ft_nla_policy
	},
	[FT_MSG_FLUSH] = {
		.call = ftnetlink_flush_table,
		.attr_count = FTA_MAX,
		.policy = ft_nla_policy
	},
};

static const struct nfnetlink_subsystem ftnl_subsys = {
	.name = "flowtable",
	.subsys_id = NFNL_SUBSYS_FLOWTABLE,
	.cb_count = FT_MSG_MAX,
	.cb = flow_table_cb,
};

static int __init ftnetlink_init(void)
{
        int ret;

        ret = nfnetlink_subsys_register(&ftnl_subsys);

        return ret;
}

static void ftnetlink_exit(void)
{
        nfnetlink_subsys_unregister(&ftnl_subsys);
}

module_init(ftnetlink_init);
module_exit(ftnetlink_exit);

MODULE_LICENSE("GPL");