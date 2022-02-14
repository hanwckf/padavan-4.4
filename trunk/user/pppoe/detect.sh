#/bin/sh
logger -t "MWAN" "flush the rules"
PPP_NUM="$(nvram get pppoe_num)"
CMD="ip route replace default"
lanip=$(ifconfig br0|grep inet|awk '{print $2}'|tr -d "addr:")
flush_iptables() {
		ipt="iptables -t $1"
		DAT=$(iptables-save -t $1)
		TAG="$2"
		eval $(echo "$DAT" | grep "$TAG" | sed -e 's/^-A/$ipt -D/' -e 's/$/;/')
		
	}

flush_iptables mangle nth
flush_iptables mangle br0
flush_iptables nat ppp


for i in $(seq 1 $PPP_NUM)
do
let count=$i-1
ppp_ip=$(ifconfig ppp"$count" | grep "inet addr" | cut -d":" -f3 | cut -d" " -f1)

if echo $ppp_ip | grep -E "^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$" >/dev/null; then
echo " PPP"$count" IP:$ppp_ip "
iptables -t mangle -A PREROUTING -s "$lanip"/24 -m conntrack --ctstate NEW -m statistic --mode nth --every $PPP_NUM --packet "$count" -j CONNMARK --set-mark 2"$i"
iptables -t mangle -A PREROUTING -i br0 -m connmark --mark 2"$i" -j MARK --set-mark 2"$i"
iptables -t nat -A POSTROUTING -o ppp"$count" -s "$lanip"/24  -j FULLCONENAT 
iptables -t nat -A PREROUTING  -i ppp"$count" -j FULLCONENAT
ip rule add fwmark 2"$i" table 2"$i"0
ip route add default via $ppp_ip dev ppp"$count" table 2"$i"0
pppindex=" ppp"$count" "
CMD=' '$CMD' nexthop via '$ppp_ip' dev '$pppindex' weight 1 '
fi
done

${CMD}
ip route flush cache
