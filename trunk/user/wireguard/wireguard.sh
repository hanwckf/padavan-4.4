#!/bin/sh

start_wg() {
	localip="$(nvram get wireguard_localip)"
	privatekey="$(nvram get wireguard_localkey)"
	peerkey="$(nvram get wireguard_peerkey)"
	peerip="$(nvram get wireguard_peerip)"
	logger -t "WIREGUARD" "正在启动wireguard"
	ifconfig wg0 down
	ip link del dev wg0
	ip link add dev wg0 type wireguard
	ip link set dev wg0 mtu 1420
	ip addr add $localip dev wg0
	echo "$privatekey" > /tmp/privatekey
	wg set wg0 private-key /tmp/privatekey
	wg set wg0 peer $peerkey persistent-keepalive 25 allowed-ips 0.0.0.0/0 endpoint $peerip
	iptables -t nat -A POSTROUTING -o wg0 -j MASQUERADE
	ifconfig wg0 up
}


stop_wg() {
	ifconfig wg0 down
	ip link del dev wg0
	logger -t "WIREGUARD" "正在关闭wireguard"
	}



case $1 in
start)
	start_wg
	;;
stop)
	stop_wg
	;;
*)
	echo "check"
	#exit 0
	;;
esac
