#!/bin/sh

action="$1"
storage_Path="/etc/storage"
smartdns_Bin="/usr/bin/smartdns"
smartdns_Ini="$storage_Path/smartdns_conf.ini"
smartdns_Conf="$storage_Path/smartdns.conf"
smartdns_tmp_Conf="$storage_Path/smartdns_tmp.conf"
smartdns_address_Conf="$storage_Path/smartdns_address.conf"
smartdns_blacklist_Conf="$storage_Path/smartdns_blacklist-ip.conf"
smartdns_whitelist_Conf="$storage_Path/smartdns_whitelist-ip.conf"
smartdns_custom_Conf="$storage_Path/smartdns_custom.conf"
dnsmasq_Conf="$storage_Path/dnsmasq/dnsmasq.conf"
chn_Route="$storage_Path/chinadns/chnroute.txt"

sdns_enable=$(nvram get sdns_enable)
snds_name=$(nvram get snds_name)
sdns_port=$(nvram get sdns_port)
sdns_tcp_server=$(nvram get sdns_tcp_server)
sdns_ipv6_server=$(nvram get sdns_ipv6_server)
snds_ip_change=$(nvram get snds_ip_change)
sdns_ipv6=$(nvram get sdns_ipv6)
sdns_www=$(nvram get sdns_www)
sdns_exp=$(nvram get sdns_exp)
snds_redirect=$(nvram get snds_redirect)
sdns_cache_persist=$(nvram get sdns_cache_persist)
snds_cache=$(nvram get snds_cache)
sdns_ttl=$(nvram get sdns_ttl)
sdns_ttl_min=$(nvram get sdns_ttl_min)
sdns_ttl_max=$(nvram get sdns_ttl_max)
sdnse_enable=$(nvram get sdnse_enable)
sdnse_port=$(nvram get sdnse_port)
sdnse_tcp=$(nvram get sdnse_tcp)
sdnse_speed=$(nvram get sdnse_speed)
sdns_speed=$(nvram get sdns_speed)
sdnse_name=$(nvram get sdnse_name)
sdnse_address=$(nvram get sdnse_address)
sdns_address=$(nvram get sdns_address)
sdnse_ns=$(nvram get sdnse_ns)
sdns_ns=$(nvram get sdns_ns)
sdnse_ipset=$(nvram get sdnse_ipset)
sdns_ipset=$(nvram get sdns_ipset)
sdnse_as=$(nvram get sdnse_as)
sdns_as=$(nvram get sdns_as)
sdnse_ipc=$(nvram get sdnse_ipc)
sdnse_cache=$(nvram get sdnse_cache)
ss_white=$(nvram get ss_white)
ss_black=$(nvram get ss_black)
sdns_coredump=$(nvram get sdns_coredump)

adbyby_process=$(pidof adbyby | awk '{ print $1 }')
smartdns_process=$(pidof smartdns | awk '{ print $1 }')
IPS4="$(ifconfig br0 | grep "inet addr" | grep -v ":127" | grep "Bcast" | awk '{print $2}' | awk -F : '{print $2}')"
IPS6="$(ifconfig br0 | grep "inet6 addr" | grep -v "fe80::" | grep -v "::1" | grep "Global" | awk '{print $3}')"
dnsmasq_md5=$(md5sum  "$dnsmasq_Conf" | awk '{ print $1 }')


# 函数

Read_ini () {
# 【读取上次成功启动时的端口等】
    if [ -s "$smartdns_Ini" ] ; then
        hosts_type=$(sed -n '1p' $smartdns_Ini)
        snds_redirected=$(sed -n '2p' $smartdns_Ini)
        sdns_ported=$(sed -n '3p' $smartdns_Ini)
    else
        hosts_type=0
        snds_redirected=0
        sdns_ported="$sdns_port"
    fi
}


Check_md5 () {
    # 【检测某些文件是否变动】
    echo "smartdns：" "Enter Check_md5"
    
    local files="$storage_Path/smartdns_*.sh"
    local md5="$storage_Path/smartdns.md5"
    local new_md5="/tmp/smartdns.md5"
    local status=0
    
    md5sum -b "$files" > $new_md5
    if [ -s "$md5" ] ; then    # 存在，则比较
        diff $md5 $new_md5 >/dev/null 2>&1
        [ $? -eq 1 ] && status=1    # 不同，则存储并更新
    else
        status=1    # 不存在，则存储并更新
    fi
    [ "$status" = 1 ] && cat $new_md5 > $md5  \
    && mtd_storage.sh save >/dev/null 2>&1
    
    echo "smartdns：" "Leave Check_md5"
}


Check_ss(){
    if [ -s /etc_ro/ss_ip.sh ] ; then
        if [ $(nvram get ss_enable) = 1 ] && [ $(nvram get ss_run_mode) = "router" ] && [ $(nvram get pdnsd_enable) = 0 ] ; then
            logger -t "SmartDNS" "系统检测到SS模式为绕过大陆模式，并且启用了pdnsd,请先调整SS解析使用SmartDNS+手动配置模式！程序将退出。"
            nvram set sdns_enable=0
            exit 0
        fi
    fi
}


Get_sdns_conf () {
    # 【】
    :>"$smartdns_tmp_Conf"
    echo "server-name $snds_name" >> "$smartdns_tmp_Conf"
    ARGS_1=""
    if [ "$sdns_address" = "1" ] ; then
     ARGS_1="$ARGS_1 -no-rule-addr"
    fi
    if [ "$sdns_ns" = "1" ] ; then
        ARGS_1="$ARGS_1 -no-rule-nameserver"
    fi
    if [ "$sdns_ipset" = "1" ] ; then
        ARGS_1="$ARGS_1 -no-rule-ipset"
    fi
    if [ "$sdns_speed" = "1" ] ; then
        ARGS_1="$ARGS_1 -no-speed-check"
    fi
    if [ "$sdns_as" = "1" ] ; then
        ARGS_1="$ARGS_1 -no-rule-soa"
    fi
    if [ "$sdns_ipv6_server" = "1" ] ; then
        echo "bind" "[::]:$sdns_port $ARGS_1" >> "$smartdns_tmp_Conf"
    else
        echo "bind" ":$sdns_port $ARGS_1" >> "$smartdns_tmp_Conf"
    fi
    if [ "$sdns_tcp_server" = "1" ] ; then
        if [ "$sdns_ipv6_server" = "1" ] ; then
            echo "bind-tcp" "[::]:$sdns_port $ARGS_1" >> "$smartdns_tmp_Conf"
        else
            echo "bind-tcp" ":$sdns_port $ARGS_1" >> "$smartdns_tmp_Conf"
        fi
    fi
    # 读取 第二服务器 配置
    Get_sdnse_conf
    echo "cache-size $snds_cache" >> "$smartdns_tmp_Conf"
    echo "rr-ttl $sdns_ttl" >> "$smartdns_tmp_Conf"
    echo "rr-ttl-min $sdns_ttl_min" >> "$smartdns_tmp_Conf"
    echo "rr-ttl-max $sdns_ttl_max" >> "$smartdns_tmp_Conf"
    echo "tcp-idle-time 120" >> "$smartdns_tmp_Conf"
    if [ "$snds_ip_change" -eq 1 ] ;then
        echo "dualstack-ip-selection yes" >> "$smartdns_tmp_Conf"
        echo "dualstack-ip-selection-threshold $(nvram get snds_ip_change_time)" >> "$smartdns_tmp_Conf"
    elif [ "$sdns_ipv6" -eq 1 ] ;then
        echo "force-AAAA-SOA yes" >> "$smartdns_tmp_Conf"
    fi
    if [ "$sdns_cache_persist" -eq 1 ] && [ "$snds_cache" -gt 0 ] ;then
        echo "cache-persist yes" >> "$smartdns_tmp_Conf"
        echo "cache-file /etc/storage/smartdns.cache" >> "$smartdns_tmp_Conf"    
    else
        echo "cache-persist no" >> "$smartdns_tmp_Conf"
    fi
    if [ "$sdns_www" -eq 1 ] && [ " $snds_cache" -gt 0 ] ;then
        echo "prefetch-domain yes" >> "$smartdns_tmp_Conf"
    else
        echo "prefetch-domain no" >> "$smartdns_tmp_Conf"
    fi
    if [ "$sdns_exp" -eq 1 ] && [ "$snds_cache" -gt 0 ] ;then
        echo "serve-expired yes" >> "$smartdns_tmp_Conf"
    else
        echo "serve-expired no" >> "$smartdns_tmp_Conf"
    fi
    echo "log-level warn" >> "$smartdns_tmp_Conf"
    listnum=$(nvram get sdnss_staticnum_x)
    for i in $(seq 1 "$listnum")
    do
        j=$(expr "$i" - 1)
        sdnss_enable=$(nvram get sdnss_enable_x"$j")
        if  [ "$sdnss_enable" -eq 1 ] ; then
            sdnss_name=$(nvram get sdnss_name_x"$j")
            sdnss_ip=$(nvram get sdnss_ip_x"$j")
            sdnss_port=$(nvram get sdnss_port_x"$j")
            sdnss_type=$(nvram get sdnss_type_x"$j")
            sdnss_ipc=$(nvram get sdnss_ipc_x"$j")
            sdnss_named=$(nvram get sdnss_named_x"$j")
            sdnss_non=$(nvram get sdnss_non_x"$j")
            sdnss_ipset=$(nvram get sdnss_ipset_x"$j")
            ipc=""
            named=""
            non=""
            sipset=""
            if [ "$sdnss_ipc" = "whitelist" ] ; then
                ipc="-whitelist-ip"
            elif [ "$sdnss_ipc" = "blacklist" ] ; then
                ipc="-blacklist-ip"
            fi
            if [ "$sdnss_named"x != x ] ; then
                named="-group $sdnss_named"
            fi
            if [ "$sdnss_non" = "1" ] ; then
                non="-exclude-default-group"
            fi
            if [ "$sdnss_type" = "tcp" ] ; then
                if [ "$sdnss_port" = "default" ] ; then
                    echo "server-tcp $sdnss_ip:53 $ipc $named $non" >> "$smartdns_tmp_Conf"
                else
                    echo "server-tcp $sdnss_ip:$sdnss_port $ipc $named $non" >> "$smartdns_tmp_Conf"
                fi
            elif [ "$sdnss_type" = "udp" ] ; then
                if [ "$sdnss_port" = "default" ] ; then
                    echo "server $sdnss_ip:53 $ipc $named $non" >> "$smartdns_tmp_Conf"
                else
                    echo "server $sdnss_ip:$sdnss_port $ipc $named $non" >> "$smartdns_tmp_Conf"
                fi
            elif [ "$sdnss_type" = "tls" ] ; then
                if [ "$sdnss_port" = "default" ] ; then
                    echo "server-tls $sdnss_ip:853 $ipc $named $non" >> "$smartdns_tmp_Conf"
                else
                    echo "server-tls $sdnss_ip:$sdnss_port $ipc $named $non" >> "$smartdns_tmp_Conf"
                fi
            elif [ "$sdnss_type" = "https" ] ; then
                if [ "$sdnss_port" = "default" ] ; then
                    echo "server-https $sdnss_ip:443 $ipc $named $non" >> "$smartdns_tmp_Conf"
                else
                    echo "server-https $sdnss_ip:$sdnss_port $ipc $named $non" >> "$smartdns_tmp_Conf"
                fi    
            fi
            if [ "$sdnss_ipset"x != x ] ; then
                # 调用 check_ip_Addr 函数，检测 ip 是否合规
                Check_ip_addr "$sdnss_ipset"
                if [ "$?" = "1" ] ;then
                    echo "ipset /$sdnss_ipset/smartdns" >> "$smartdns_tmp_Conf"
                else
                    ipset add smartdns "$sdnss_ipset" 2>/dev/null
                fi
            fi
        fi
    done
    if [ "$ss_white" = "1" ] && [ -f "$chn_Route" ] ; then
        :>/tmp/whitelist.conf
        logger -t "SmartDNS" "开始处理白名单IP"
        awk '{printf("whitelist-ip %s\n", $1, $1 )}' "$chn_Route" >> /tmp/whitelist.conf
        echo "conf-file /tmp/whitelist.conf" >> "$smartdns_tmp_Conf"
    fi
    if [ "$ss_black" = "1" ] && [ -f "$chn_Route" ] ; then
        :>/tmp/blacklist.conf
        logger -t "SmartDNS" "开始处理黑名单IP"
        awk '{printf("blacklist-ip %s\n", $1, $1 )}' "$chn_Route" >> /tmp/blacklist.conf
        echo "conf-file /tmp/blacklist.conf" >> "$smartdns_tmp_Conf"
    fi
}


Get_sdnse_conf () {
    # 【】
    if [ "$sdnse_enable" -eq 1 ] ; then
    ARGS_2=""
    ADDR=""
    if [ "$sdnse_speed" = "1" ] ; then
        ARGS_2="$ARGS_2 -no-speed-check"
    fi
    if [ -n "$sdnse_name" ] ; then
        ARGS_2="$ARGS_2-group $sdnse_name"
    fi
    if [ "$sdnse_address" = "1" ] ; then
        ARGS_2="$ARGS_2-no-rule-addr"
    fi
    if [ "$sdnse_ns" = "1" ] ; then
        ARGS_2="$ARGS_2-no-rule-nameserver"
    fi
    if [ "$sdnse_ipset" = "1" ] ; then
        ARGS_2="$ARGS_2-no-rule-ipset"
    fi
    if [ "$sdnse_as" = "1" ] ; then
        ARGS_2="$ARGS_2-no-rule-soa"
    fi
    if [ "$sdnse_ipc" = "1" ] ; then
        ARGS_2="$ARGS_2-no-dualstack-selection"
    fi
    if [ "$sdnse_cache" = "1" ] ; then
        ARGS_2="$ARGS_2-no-cache"
    fi
    if [ "$sdns_ipv6_server" = "1" ] ; then
        ADDR="[::]"
    else
        ADDR=""
    fi
    echo "bind" "$ADDR:$sdnse_port $ARGS_2" >> "$smartdns_tmp_Conf"
     if [ "$sdnse_tcp" = "1" ] ; then
        echo "bind-tcp" "$ADDR:$sdnse_port $ARGS_2" >> "$smartdns_tmp_Conf"
    fi
fi
}


Check_ip_addr () {
    echo "$1"|grep "^[0-9]\{1,3\}\.\([0-9]\{1,3\}\.\)\{2\}[0-9]\{1,3\}$" >/dev/null
    # IP地址必须为全数字
    if [ $? -ne 0 ] ; then
        return 1
    fi
    ipaddr=$1
    a=$(echo "$ipaddr"|awk -F . '{ print $1 }')  # 以"."分隔，取出每个列的值
    b=$(echo "$ipaddr"|awk -F . '{ print $2 }')
    c=$(echo "$ipaddr"|awk -F . '{ print $3 }')
    d=$(echo "$ipaddr"|awk -F . '{ print $4 }')
    for num in $a $b $c $d
    do
        if [ "$num" -gt 255 ] || [ "$num" -lt 0 ] ; then   # 每个数值必须在0-255之间
            return 1
        fi
    done
    return 0
}


Change_adbyby () {
# 【】
    if [ "$adbyby_process"x != x ] && [ $(nvram get adbyby_enable) = 1 ] ; then
    case $sdns_enable in
    0)
        if [ $(nvram get adbyby_add) = 1 ] && [ "$hosts_type" != "Dnsmasq" ]; then
            nvram set adbyby_add=0
            /usr/bin/adbyby.sh switch
            logger -t "SmartDNS" "DNS 去广告规则: SmartDNS ⇒ Dnsmasq"
            hosts_type="Dnsmasq"
        fi
        ;;
    1)
        if [ "$hosts_type" != "SmartDNS" ] && [ "$action" = "start" ] ; then
            if [ "$sdns_port" = "53" ] || [ $(nvram get adbyby_add) = 1 ] || [ "$snds_redirect" = "2" ] ; then
                nvram set adbyby_add=1
                /usr/bin/adbyby.sh switch
                logger -t "SmartDNS" "DNS 去广告规则: Dnsmasq ⇒ SmartDNS"
                hosts_type="SmartDNS"
            fi
        fi
        ;;
    esac
    fi
}


Change_dnsmasq () {
    # 删除 dnsmasq 配置文件中的相关项，避免重复
    case $action in
    stop)
        sed -i '/no-resolv/d' "$dnsmasq_Conf"
        sed -i '/server=127.0.0.1#'"$sdns_portd"'/d' "$dnsmasq_Conf"
        sed -i '/port=0/d' "$dnsmasq_Conf"
        if [ "$sdns_enable" = 0 ] ; then
            [ "$sdns_ported" = "53" ] && logger -t "SmartDNS" "已启用 dnsmasq 域名解析（DNS）功能" 
            [ "$snds_redirected" = "1" ] && logger -t "SmartDNS" "删除 dnsmasq 上游服务器：127.0.0.1:$sdns_ported" 
        fi
        ;;
    start)
        # 启动 SmartDNS 时
        if [ "$sdns_port" = "53" ] ; then
            echo "port=0" >> "$dnsmasq_Conf"
            logger -t "SmartDNS" "已关闭 dnsmasq 域名解析（DNS）功能"
            if [ "$snds_redirect" = "1" ] ; then
                nvram set snds_redirect=0
                snds_redirect=0
                logger -t "SmartDNS" "因此，自动修改 重定向为：无" 
            fi
        fi
        if [ "$snds_redirect" = "1" ] ; then
            echo "no-resolv" >> "$dnsmasq_Conf"
            echo "server=127.0.0.1#$sdns_port" >> "$dnsmasq_Conf"
            logger -t "SmartDNS" "作为 dnsmasq 上游服务器：127.0.0.1:$sdns_port"
        fi
        ;;
    esac
}



Change_iptable () {
    # 【】
    local statu=0
    case $action in
    stop)
        if [ "$snds_redirected" = 2 ] ; then
            iptables -t nat -D PREROUTING -p tcp --dport 53 -j REDIRECT --to-ports "$sdns_ported" >/dev/null 2>&1
            iptables -t nat -D PREROUTING -p udp --dport 53 -j REDIRECT --to-ports "$sdns_ported" >/dev/null 2>&1
            ip6tables -t nat -D PREROUTING -p tdp --dport 53 -j REDIRECT --to-ports "$sdns_ported" >/dev/null 2>&1
            ip6tables -t nat -D PREROUTING -p udp --dport 53 -j REDIRECT --to-ports "$sdns_ported" >/dev/null 2>&1
            [ "$sdns_enable" = 0 ] && logger -t "SmartDNS" "恢复重定向 $IPS4:$sdns_ported 至 xxx.xxx.xxx:53"
        fi
        if [ "$snds_redirected" = 1 ] ; then
            iptables -t nat -D PREROUTING -p udp --dport 53 -j REDIRECT --to-ports 53 >/dev/null 2>&1
        fi
        ;;
    start)
        if [ "$snds_redirected" != 2 ] && [ "$snds_redirect" = 2 ] ; then
            statu=1
            logger -t "SmartDNS" "重定向 xxx.xxx.xxx:53 至 $IPS4:$sdns_port"
        fi
        ;;
    reset)
        if [ "$snds_redirect" = 2 ] ; then
            statu=1
        fi
        if [ "$snds_redirect" = 1 ] ; then
            iptables -t nat -A PREROUTING -p udp --dport 53 -j REDIRECT --to-ports 53 >/dev/null 2>&1
        fi
        ;;
    esac
    if [ "$statu" = 1 ] ; then
        [ "$sdns_tcp_server" = "1" ] && iptables -t nat -A PREROUTING -p tcp --dport 53 -j REDIRECT --to-ports "$sdns_port" >/dev/null 2>&1
        iptables -t nat -A PREROUTING -p udp --dport 53 -j REDIRECT --to-ports "$sdns_port" >/dev/null 2>&1
        if [ "$sdns_ipv6_server" = "1" ] ; then
            [ "$sdns_tcp_server" = "1" ] && ip6tables -t nat -A PREROUTING -p tcp --dport 53 -j REDIRECT --to-ports "$sdns_port" >/dev/null 2>&1
            ip6tables -t nat -A PREROUTING -p udp --dport 53 -j REDIRECT --to-ports "$sdns_port" >/dev/null 2>&1
        fi
    fi
}

Start_smartdns () {
    # 【】
    :>"$smartdns_Ini"
    [ "$sdns_enable" -eq 0 ] && nvram set sdns_enable=1 && sdns_enable=1
    [ $(pidof smartdns | awk '{ print $1 }')x != x ] && killall -9 smartdns >/dev/null 2>&1
    Change_dnsmasq
    Change_adbyby
    echo "$hosts_type" >> "$smartdns_Ini"
    [ "$snds_redirect" = 0 ] && logger -t "SmartDNS" "SmartDNS 使用 $sdns_port 端口"
    Change_iptable
    snds_redirected="$snds_redirect"
    echo "$snds_redirected" >> "$smartdns_Ini"
    echo "$sdns_port" >> "$smartdns_Ini"
    #存疑
    rm -f /tmp/sdnsipset.conf
    args=""
    logger -t "SmartDNS" "创建配置文件..."
    ipset -N smartdns hash:net >/dev/null
    Get_sdns_conf
    grep -v '^#' $smartdns_address_Conf | grep -v "^$" >> "$smartdns_tmp_Conf"
    grep -v '^#' $smartdns_blacklist_Conf | grep -v "^$" >> "$smartdns_tmp_Conf"
    grep -v '^#' $smartdns_whitelist_Conf | grep -v "^$" >> "$smartdns_tmp_Conf"
    grep -v '^#' $smartdns_custom_Conf | grep -v "^$" >> "$smartdns_tmp_Conf"
    sed -i '/my.router/d' "$smartdns_tmp_Conf"
    echo "domain-rules " "/my.router/ -c none -a $IPS4 -d no" >> "$smartdns_tmp_Conf"
    # 配置文件去重
    awk '!x[$0]++' "$smartdns_tmp_Conf" > "$smartdns_Conf"
    rm -f "$smartdns_tmp_Conf"
    if [ "$sdns_coredump" = "1" ] ; then
        args="$args -S"
    fi
    # 通过检测配置文件是否变化，确定是否重启 dnsmasq 进程
    if [ "$dnsmasq_md5" != $(md5sum  "$dnsmasq_Conf" | awk '{ print $1 }') ] ; then
         /sbin/restart_dhcpd >/dev/null 2>&1
    fi
    # 启动 smartdns 进程
    "$smartdns_Bin" -f -c "$smartdns_Conf" "$args"  &>/dev/null &
    sleep 1
    smartdns_process=$(pidof smartdns | awk '{ print $1 }')
    if [ "$smartdns_process"x = x ] ; then
        if [ "$hosts_type" = "SmartDNS" ] ; then
            logger -t "SmartDNS" "启动失败．．．"
            logger -t "SmartDNS" "删除"$smartdns_Conf"中conf-file附加去广告设置，再次启动......"
            logger -t "SmartDNS" "若启动成功，则请检查相关去广告规则格式是否符合SmartDNS要求"
            sed -i '/conf-file /d' "$smartdns_Conf"
            "$smartdns_Bin" -f -c "$smartdns_Conf" "$args"  &>/dev/null &
        fi
    fi
    sleep 1
    smartdns_process=$(pidof smartdns | awk '{ print $1 }')
    if [ "$smartdns_process"x = x ] ; then
        logger -t "SmartDNS" "启动失败．．．"
        logger -t "SmartDNS" "停用SmartDNS！请检查其端口配置及自定义设置！"
        logger -t "SmartDNS" "恢复Dnsmasq提供dns解析"
        nvram set sdns_enable=0
        sdns_enable=0
        action="stop"
        Stop_smartdns
        if [ "$dnsmasq_md5" != $(md5sum  "$dnsmasq_Conf" | awk '{ print $1 }') ] ; then
             /sbin/restart_dhcpd >/dev/null 2>&1
        fi
        exit
    else
        logger -t "SmartDNS" "smartdns 进程已启动"
    fi
}


Stop_smartdns () {
    # 【】
    killall -9 smartdns >/dev/null 2>&1
    logger -t "SmartDNS" "结束smartdns进程 ．．．"
    Change_adbyby
    Change_dnsmasq
    Change_iptable
    if [ "$dnsmasq_md5" != $(md5sum  "$dnsmasq_Conf" | awk '{ print $1 }') ] && [ "$sdns_enable" = 0 ] ; then
         /sbin/restart_dhcpd >/dev/null 2>&1
    fi
    smartdns_process=$(pidof smartdns | awk '{ print $1 }')
    if [ "$smartdns_process"x = x ] && [ "$sdns_enable" = 0 ] ; then 
        rm  -f "$smartdns_Ini"
        logger -t "SmartDNS" "已停用"
    fi
}


Main () {
# 【调用各子函数】
    case $action in
    start)
        if [ ! -s "$smartdns_Ini" ] ; then
            logger -t "SmartDNS" "启动．．．"
        fi
        Check_ss
        Start_smartdns
        logger -t "SmartDNS" "已成功启动"
        ;;
    stop)
        if [ "$smartdns_process"x != x ] ; then
            case $sdns_enable in
            0)
                logger -t "SmartDNS" "停用 ．．．"
                ;;
            1)
                logger -t "SmartDNS" "重启 ．．．"
                ;;
            esac
        fi
        Stop_smartdns
        ;;
    restart)
        if [ $(nvram get adbyby_enable) = 1 ] ; then
            [ $(nvram get adbyby_add) = 1 ] && hosts_type="SmartDNS"
            [ $(nvram get adbyby_add) = 0 ] && hosts_type="Dnsmasq"
        else
            hosts_type="0"
        fi
        Check_ss
        Start_smartdns
        logger -t "SmartDNS" "已完成重启"
        ;;
    reset)
        [ "$sdns_enable" = "1" ] && Change_iptable
        ;;
    *)
        echo "check"
        ;;
    esac
    }

Read_ini
Main
