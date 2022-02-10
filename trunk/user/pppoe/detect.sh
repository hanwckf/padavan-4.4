#/bin/sh
logger -t "MWAN" "flush the rules"
PPP_NUM="$(nvram get pppoe_num)"
CMD="ip route replace default"
lanip=$(ifconfig br0|grep inet|awk '{print $2}'|tr -d "addr:")

for i in $(seq 1 $PPP_NUM)
do
let count=$i-1
ppp_ip=$(ifconfig ppp"$count" | grep "inet addr" | cut -d":" -f3 | cut -d" " -f1)

if echo $ppp_ip | grep -E "^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$" >/dev/null; then
echo " PPP"$count" IP:$ppp_ip "
iptables -t mangle -A PREROUTING -s "$lanip"/24 -m conntrack --ctstate NEW -m statistic --mode nth --every $PPP_NUM --packet "$count" -j CONNMARK --set-mark "$i"
iptables -t mangle -A PREROUTING -i br0 -m connmark --mark "$i" -j MARK --set-mark "$i"
iptables -t nat -A POSTROUTING -s "$lanip"/24 -o ppp"$count" -j MASQUERADE
ip rule add fwmark $i table "$i"0
ip route add default via $ppp_ip dev ppp"$count" table "$i"0
pppindex=" ppp"$count" "
CMD=' '$CMD' nexthop via '$ppp_ip' dev '$pppindex' weight 1 '
fi
done

${CMD}
ip route flush cache


