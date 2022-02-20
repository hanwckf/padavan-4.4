################################################################################
# wg-quick script from wireguard tools ash shell busybox modification
# license: MIT
# author: Isaev Ruslan (legale.legale_in_gmail.com)
################################################################################

#!/bin/sh
#set -x

WG_CONFIG=""
INTERFACE=""
ADDRESSES=""
MTU=""
DNS=""
DNS_SEARCH=""
TABLE=""
PRE_UP=""
POST_UP=""
PRE_DOWN=""
POST_DOWN=""
SAVE_CONFIG=1
CONFIG_FILE=""
PROGRAM="${0##*/}"
ARGS="$*"
RED='\033[0;31m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
NC='\033[0m' # No Color

undo_changes(){
#  log "abort()"
  log_red "Error! Undoing changes..."
  cmd_down force
  log "Changes undone."
}

parse_options() {
#  log "parse_options()"
  local VAL
  local NAMES
  local NAME
  local BLOCK="global"
  CONFIG_FILE=$1
  local CONFIG_FILE_=$1
  [ -e $CONFIG_FILE ] || CONFIG_FILE="/etc/wireguard/$CONFIG_FILE.conf"
  [ -e $CONFIG_FILE ] || die "'$CONFIG_FILE_' does not exist\n'$CONFIG_FILE' does not exist"
  INTERFACE=$(basename "$CONFIG_FILE_" | cut -d'.' -f1)
  [ $(echo $CONFIG_FILE | grep -o "^.\+\.conf$") ] || die "The config file must be a valid interface name, followed by .conf"
  
  #log_start "Reading configuration file..."
  while read -r LINE_; do
    CONTINUE=1
    if [ ! -z "$LINE_" ] && [ $(echo "$LINE_" | cut -c 1) != "#" ]; then #check if not empty and not comment line
      #block name parse
      BLOCK_=$(echo "$LINE_" | grep -o "\[.\+\]" | tr -d "[]" | tr "[a-z]" "[A-Z]") #find pattern [.+] get BLOCK name without quotes
      [ ! -z "$BLOCK_" ] && BLOCK="$BLOCK_" # set BLOCK name
      #echo "BLOCK:$BLOCK line:$LINE"
      
      #param, value parse
      if [ -z "$BLOCK_" ]; then  #if this line is not a BLOCK
        NAME=$(echo -n $LINE_ | cut -f1 -d " " | tr "[a-z]" "[A-Z]") #split first part before space and make uppercase
        VAL=$(echo -n $LINE_ | grep -o "=.\+" | cut -c 3-) #cut from 3 char after first =
        BNAME="${BLOCK}_${NAME}"
        eval "$BNAME"="\"$VAL\""
        NAMES="$NAMES$BNAME#"
      fi
      
      #interface block special params parse
      if [ "$BLOCK" == "INTERFACE" ]; then
        case "$NAME" in
        ADDRESS)
          VAL=$(echo -e $VAL | tr -d " " | tr -s "," "#")
          if [ -z "$ADDRESSES" ]; then ADDRESSES="$VAL"; else ADDRESSES="$ADDRESSES#$VAL"; fi; CONTINUE=0 ;;          
        MTU)
          if [ -z "$MTU" ]; then  MTU="$VAL"; else MTU="$MTU#$VAL"; fi; CONTINUE=0 ;;
        DNS)
          VAL=$(echo -e $VAL | tr -d " " | tr -s "," "#")
          echo $VAL | grep "^[0-9.]\+$" >/dev/null 2>&1
          if [ $? ]; then
            if [ -z "$DNS" ]; then DNS="$VAL"; else DNS="$DNS#$VAL"; fi
          else
            if [ -z "$DNS_SEARCH" ]; then DNS_SEARCH="$VAL"; else DNS_SEARCH="$DNS_SEARCH#$VAL"; fi
          fi 
          CONTINUE=0 ;;
        TABLE) if [ -z "$TABLE" ]; then TABLE="$VAL"; else TABLE="$TABLE#$VAL"; fi; CONTINUE=0 ;;
        PREUP) if [ -z "$PRE_UP" ]; then PRE_UP="$VAL"; else PRE_UP="$PRE_UP#$VAL"; fi; CONTINUE=0 ;;
        PREDOWN) if [ -z "$PRE_DOWN" ]; then PRE_DOWN="$VAL"; else PRE_DOWN="$PRE_DOWN#$VAL"; fi; CONTINUE=0 ;;
        POSTUP) if [ -z "$POST_UP" ]; then POST_UP="$VAL"; else POST_UP="$POST_UP#$VAL"; fi; CONTINUE=0 ;;
        POSTDOWN) if [ -z "$POST_DOWN" ]; then POST_DOWN="$VAL"; else POST_DOWN="$POST_DOWN#$VAL"; fi; CONTINUE=0 ;;
        SAVECONFIG) read_bool SAVE_CONFIG "$VAL"; CONTINUE=0 ;;
        esac
        
      fi
    [ "$CONTINUE" == 0 ] && continue #trick to skip loop from case construction
  fi
  #wg only configuration params (stripped config)
  WG_CONFIG="$WG_CONFIG$LINE_\n"
  done < $CONFIG_FILE
# log_end "ok"
  
# log "Your configuration is:"
  local IFS="#"
  for N in $NAMES; do
    local VALUE=$(eval echo "\${$N}") #this will expand $N variable and get it value 
    # printf "${YELLOW}param:${NC} %-40.40s ${YELLOW}value:${NC} %s\n" $N $VALUE 
  done
  
}

read_bool() {
# log "read_bool()"
  case "$2" in
  true) eval "$1"=1 ;;
  false) eval "$1"=0 ;;
  *) die "'\$2' is neither true nor false"
  esac
}

auto_su() {
#  log "auto_su()"
  #log_start "Checking root access..."
	if [ ! -z $UID ] && [ $UID != "0" ]; then #if UID is not unset and not 0
    [ ! $(which sudo >/dev/null) ] && die "$PROGRAM must be run as root but sudo is not installed!"
    [ exec sudo -p "$PROGRAM must be run as root. Please enter the password for %u to continue: " -- /bin/sh -- "$0" "$*" ]
  fi
  #log_end "ok"
}

add_if() {
  #log "add_if()"
  local RET
  # log "Adding interface $INTERFACE..."
  cmd "ip link add $INTERFACE type wireguard"
	if [ $? != 0 ]; then
		local RET=$?
		[ -e /sys/module/wireguard ] || [ ! command -v "${WG_QUICK_USERSPACE_IMPLEMENTATION:-wireguard-go}" >/dev/null ] && exit $RET
		echo "[!] Missing WireGuard kernel module. Falling back to slow userspace implementation." >&2
		cmd "${WG_QUICK_USERSPACE_IMPLEMENTATION:-wireguard-go}" "$INTERFACE"
	fi
}

del_if() {
#  log "del_if()"
  # log "Removing interface $INTERFACE..."
  cmd "ip link delete dev $INTERFACE"
}

add_addr() {
#  log "add_addr()"
	local PROTO=-4
	echo -n "$1" | grep ":" >/dev/null && PROTO=-6
	cmd "ip $PROTO address add $1 dev $INTERFACE" || die "Unable to set ip address!"
  return $?
}

set_mtu_up() {
#  log "mtu_set()"
	local ENDPOINT ENDPOINTS OUTPUT
	if [ $MTU ]; then
		cmd "ip link set mtu $MTU up dev $INTERFACE"
  else 
    cmd "ip link set mtu $(( 1500 - 80 )) up dev $INTERFACE"
  fi
  return $?
}

set_dns() {
#  log "set_dns()"
  local SERVER
  local IFS="#"
  for SERVER in $DNS; do
    cmd "echo \"nameserver $SERVER\" >> /etc/resolv.conf"
	done
  for SERVER in $DNS_SEARCH; do
    cmd "echo \"search $SERVER\" >> /etc/resolv.conf"
	done
  return $?
}

unset_dns() {
#  log "unset_dns()"
  local SERVER
  local TMP="/tmp/resolv.tmp.conf"
  local IFS='#'
  for SERVER in $DNS; do
    cmd "cat /etc/resolv.conf | grep -v \"nameserver $SERVER\" > $TMP"
    cmd "cat $TMP > /etc/resolv.conf"
	done
  for SERVER in $DNS_SEARCH; do
    cmd "cat /etc/resolv.conf | grep -v \"search $SERVER\" > /tmp/resolv.tmp.conf"
    cmd "cat $TMP > /etc/resolv.conf"
	done
}

add_route() {
#  log "add_route()"
	local PROTO=-4
	echo -n "$1" | grep ":" >/dev/null && PROTO=-6
	[ "$TABLE" == "off" ] && return 0
	if [ -n "$TABLE" ] && [ "$TABLE" != "auto" ]; then
		cmd "ip $PROTO route add $1 dev $INTERFACE table $TABLE"
	elif [ $(echo -n "$1" | grep "/0") ]; then
		add_default "$1" 
	else
		cmd "ip $PROTO route add $1 dev $INTERFACE"
	fi
}

get_min_rule_pref(){
  local PROTO=$2
  WG_PREF=$(ip $PROTO rule show | cut -d ':' -f 1 | grep "[^0]" | head -n 1)
  eval "$1"=$((WG_PREF-1))
}

bypass_route(){
  log_green "Trying to create bypass route"
  local PROTO=$1
  local TABLE=$2
  local BYPASS_TABLE=$((TABLE+1))
  get_min_rule_pref WG_PREF
  local MAIN_PREF=$((WG_PREF-1))

  log_green "Copying 'ip route' main table to the new bypass table $BYPASS_TABLE..."
  ip_ro_copy_tab_skip_def $PROTO main $BYPASS_TABLE
  
  cmd "ip $PROTO rule add from all table $BYPASS_TABLE pref $MAIN_PREF"
  cmd "ip $PROTO rule add not fwmark 51820 table 51820 pref $WG_PREF"
}

ip_ro_copy_tab_skip_def(){
  local IFS=$'\n'
  local PROTO=$1; local SRC=$2; local DST=$3
  for LINE in $(ip $PROTO route show table $SRC); do 
    [ $(echo $LINE | grep "^default") ] && continue
    ip_ro_line $LINE
    ip_ro_add "$PROTO" "$NET" "$VIA" "$DEV" "$METRIC" "$DST"
  done
}

ip_ro_line(){
  NET=$(echo $LINE | cut -d ' ' -f 1)
  VIA=""; DEV=""; METRIC="";

  local LINE=$1
  local ELSE=$(echo $LINE | cut -d ' ' -f 2-)
  local IFS=' '
  local i=2
  for E in $ELSE; do
    eval "E$i"="$E"
    i=$(($i+1))
  done
  local i=3
  for E in $ELSE; do
    case $E in
    "via") eval "VIA"="\$E$i" ;;
    "dev") eval "DEV"="\$E$i" ;;
    "metric") eval "METRIC"="\$E$i" ;;
    esac
    i=$(($i+1))
  done
}

ip_ro_add(){
  local PROTO; local NET; local VIA; local DEV; local METRIC; local TAB;
  ( [ -z "$1" ] || [ -z "$2" ] ) && log "ip_ro_add args error '$1' '$2'" && return 1
  PROTO=$1
  NET=$2
  [ -n "$3" ] && VIA="via $3"
  [ -n "$4" ] && DEV="dev $4"
  [ -n "$5" ] && METRIC="metric $5"
  [ -n "$6" ] && TAB="table $6"
  cmd "ip route add $NET $VIA $DEV $METRIC $TAB"
}

add_default() {
#  log "add_default()"
  get_ro_table_fwmark "TABLE"
	[ $? ] && cmd "wg set $INTERFACE fwmark $TABLE"
  
	local PROTO=-4
	echo -n "$1" | grep ":" >/dev/null && PROTO=-6
  local IPTABLES="iptables"; 
	[ "$PROTO" == "-6" ] && IPTABLES="ip6tables"
  
  #ip routes and rules
	cmd "ip $PROTO route add $1 dev $INTERFACE table $TABLE" || ( undo_changes && die "Unable to add route! Program aborted." )
	cmd "ip $PROTO rule add not fwmark $TABLE table $TABLE" || ( undo_changes && die "Unable to add rule! Program aborted." )
	cmd "ip $PROTO rule add table main suppress_prefixlength 0" || bypass_route "$PROTO" "$TABLE"

  #iptables
  get_dev_ip IP $PROTO $INTERFACE
  if [ -n "$IP"  ]; then
    cmd "$IPTABLES -t raw -A PREROUTING ! -i $INTERFACE -d $IP -m addrtype ! --src-type LOCAL -j DROP"
    cmd "$IPTABLES -t mangle -A POSTROUTING -m mark --mark $TABLE -p udp -j CONNMARK --save-mark"
    cmd "$IPTABLES -t mangle -A PREROUTING -p udp -j CONNMARK --restore-mark"  
  fi
  [ "$PROTO" == "-4" ] && cmd "sysctl -q -w net.ipv4.conf.all.src_valid_mark=1"

	return 0
}

get_dev_ip(){
  IP=""
  local LINE A
  local PROTO=$2
  local INTERFACE=$3
  log_yellow "[#] ip -o $PROTO address show dev $INTERFACE"
  LINE=$(ip -o $PROTO address show dev $INTERFACE 2>/dev/null)
  A=$(echo $LINE | grep -io "inet[6]\? [a-z0-9.:]\+" | cut -d " " -f 2)
  [ -n "$A" ] || return 1
  eval "$1=$A"
  return 0
}

remove_firewall() {
#  log "remove_firewall()"
  get_ro_table_fwmark "TABLE"
  local BYPASS_TABLE=$((TABLE+1))
  for PROTO in -4 -6; do
    local MAIN_PREF=$((WG_PREF-1))

    while ip $PROTO rule show | grep "lookup $TABLE" >/dev/null 2>&1; do
      cmd "ip $PROTO rule delete table $TABLE"
    done
    while ip $PROTO rule show | grep "from all lookup main suppress_prefixlength 0" >/dev/null 2>&1; do
      cmd "ip $PROTO rule delete table main suppress_prefixlength 0"
    done
    #legacy ip routes and rules related to bypass table
    while ip $PROTO rule show | grep "from all lookup $BYPASS_TABLE" >/dev/null 2>&1; do
      cmd "ip $PROTO rule delete from all table $BYPASS_TABLE"
    done
    while [ -n "$(ip $PROTO route show table $BYPASS_TABLE 2>/dev/null)" ]; do
      cmd "ip $PROTO route flush table $BYPASS_TABLE"
    done
  done

  local IPTABLES="iptables"
	if type -p iptables >/dev/null; then
    for PROTO in -4 -6; do
      get_dev_ip IP $PROTO $INTERFACE
      if [ -n "$IP"  ]; then
        [ "$PROTO" == -6 ] && IPTABLES="ip6tables"
        cmd "$IPTABLES -t raw -D PREROUTING ! -i $INTERFACE -d $IP -m addrtype ! --src-type LOCAL -j DROP"
        cmd "$IPTABLES -t mangle -D POSTROUTING -m mark --mark $TABLE -p udp -j CONNMARK --save-mark"
        cmd "$IPTABLES -t mangle -D PREROUTING -p udp -j CONNMARK --restore-mark"  
      fi
    done
	fi
}

get_ro_table_fwmark() {
#  log "get_ro_table_fwmark()"
	local FWMARK
	FWMARK=$(wg show $INTERFACE fwmark) 
  
  if [ "$FWMARK" != "off" ]; then 
    FWMARK=$(printf "%d" $FWMARK)
    eval "$1"=$(printf "%d" $FWMARK)
    return 0
  fi
  FWMARK=51820
  while [ -n "$(ip -4 route show table $FWMARK 2>/dev/null)" ] || \
        [ -n "$(ip -6 route show table $FWMARK 2>/dev/null)" ]; do
    FWMARK=$((FWMARK+1))
  done
  eval "$1=$FWMARK"
  return 1
}

set_config() {
#  log "set_config()"
  local TMP="/tmp/${INTERFACE}.tmp.conf"
  
  echo -e "$WG_CONFIG" > "$TMP"
  #log "Configuring interface $INTERFACE..."
  cmd "wg setconf "$INTERFACE" $TMP" || die "Unable to set configuration $TMP. Program Terminated."
  rm -f "$TMP"
}

save_config() {
#  log "save_config()"
  local i=0
  local VAL
  local NEW_ADDR
  local CONFIG_FILE="/etc/wireguard/$INTERFACE.conf"
  local TMP="$CONFIG_FILE.tmp"

	local OLD_UMASK=$(umask)
	umask 077
  local WG_CONF_FILE="$CONFIG_FILE.wg.tmp"
  wg showconf "$INTERFACE" >$WG_CONF_FILE 
  
  printf "[Interface]\n" >$TMP
  
  for VAL in $(ip address show dev optzona | grep -io "inet[6]\? [a-z0-9.:]\+" | cut -d " " -f 2); do
    printf "Address = %s\n" $VAL >>$TMP 
  done
  
  [ -n "$DNS"  ] && for VAL in $DNS; do echo "DNS = $VAL">>$TMP; done
  [ -n "$DNS_SEARCH" ] && for VAL in $DNS_SEARCH; do echo "DNS = $VAL">>$TMP; done
  [ -n "$MTU" ] && MTU=$(ip link show dev $INTERFACE | grep -o "mtu [0-9]*" | grep -o '[0-9]*') \
    && echo "MTU = $MTU" >>$TMP
  [ -n "$TABLE" ] && echo "Table = $TABLE" >>$TMP
  [ "$SAVE_CONFIG" == 0 ] && echo "SaveConfig = true" >>$TMP
  
  
  local IFS="#"
  [ -n "$PRE_UP" ] &&	for VAL in $PRE_UP; do echo "PreUp = $VAL" >>$TMP; done
  [ -n "$POST_UP" ] && for VAL in $POST_UP; do echo "PostUp = $VAL" >>$TMP; done
  [ -n "$PRE_DOWN" ] && for VAL in $PRE_DOWN; do echo "PreDown = $VAL" >>$TMP; done
  [ -n "$POST_DOWN" ] && for VAL in $POST_DOWN; do echo "PostDown = $VAL" >>$TMP; done  

  cat "$WG_CONF_FILE" | grep -iv "\[interface\]" >>$TMP
  rm -f "$WG_CONF_FILE"
	mv "$TMP" "$CONFIG_FILE" || die "Could not move configuration file $TMP to $CONFIG_FILE"
	umask "$OLD_UMASK"
}

execute_hooks() {
#  log "execute_hooks()"
	local HOOK
  local IFS="#"
  [ -z "$*" ] && return 0
	for HOOK in $*; do
    HOOK="${HOOK//%i/$INTERFACE}"
		cmd "$HOOK"
	done
}

cmd_usage() {
	cat >&2 <<-_EOF
	Usage: $PROGRAM [ up | down | save | strip ] [ CONFIG_FILE | INTERFACE ]

	  CONFIG_FILE is a configuration file, whose filename is the interface name
	  followed by \`.conf'. Otherwise, INTERFACE is an interface name, with
	  configuration found at /etc/wireguard/INTERFACE.conf. It is to be readable
	  by wg(8)'s \`setconf' sub-command, with the exception of the following additions
	  to the [Interface] section, which are handled by $PROGRAM:

	  - Address: may be specified one or more times and contains one or more
	    IP addresses (with an optional CIDR mask) to be set for the interface.
	  - DNS: an optional DNS server to use while the device is up.
	  - MTU: an optional MTU for the interface; if unspecified, auto-calculated.
	  - Table: an optional routing table to which routes will be added; if
	    unspecified or \`auto', the default table is used. If \`off', no routes
	    are added.
	  - PreUp, PostUp, PreDown, PostDown: script snippets which will be executed
	    by sh(1) at the corresponding phases of the link, most commonly used
	    to configure DNS. The string \`%i' is expanded to INTERFACE.
	  - SaveConfig: if set to \`true', the configuration is saved from the current
	    state of the interface upon shutdown.

	See wg-quick(8) for more info and examples.
	_EOF
  return $?
}

cmd_usage_short() {
	cat >&2 <<-_EOF
	Usage: $PROGRAM [ up | down | save | strip ] [ CONFIG_FILE | INTERFACE ]
  Try '$PROGRAM --help' for more information.
	_EOF
  return $?
}

is_interface() {
  ip link show dev $1 >/dev/null 2>&1
  return $?
}

is_wg_interface () { 
  wg show interfaces | grep $1 >/dev/null 2>&1 
  return $?
}

cmd_up() {
#  log "cmd_up()"
  if is_interface "$INTERFACE"; then
    die "Interface $INTERFACE already exist"
  fi
	execute_hooks "$PRE_UP"
	add_if
	set_config  
  local i IP
  local IFS="#"
	for i in $ADDRESSES; do
		add_addr "$i"
	done
	
  set_mtu_up
	set_dns
  
  IFS=$'\n\t '
  for i in $(wg show "$INTERFACE" allowed-ips); do
    IP=$(echo "$i" | grep "^[0-9a-z.:/]\+$")
    [ $IP ] && add_route "$IP"
  done
  
	execute_hooks "$POST_UP"
	trap - INT TERM EXIT
  return $?
}

cmd_down() {
#  log "cmd_down()"
  
  [ "$1" != "force" ] && ! is_wg_interface $INTERFACE && \
  die "interface '$INTERFACE' does not exist, or not a WireGuard interface"
  
  execute_hooks "$PRE_DOWN"
	[ $SAVE_CONFIG == 0 ] && save_config
  
  remove_firewall
  unset_dns 
	del_if
	execute_hooks "$POST_DOWN"
  return $?
}

cmd_save() {
#  log "cmd_save()"
	if ! is_wg_interface $INTERFACE; then
    die "interface '$INTERFACE' does not exist, or not a WireGuard interface"
	fi
  save_config
  return $?
}

cmd_strip() {
#  log "cmd_strip()"
	echo -e "$WG_CONFIG"
  return $?
}

die() {
#  log "die()"
	echo -e "${RED}$PROGRAM: $*${NC}" >&2
	exit 1
}

cmd() {
  log_yellow "[#] $1"
	eval "$1" >&2
}

log() {
	echo -e "$1$2" >&2
}

log_yellow() {
	echo -e "${YELLOW}${1}${NC}$2" >&2
}

log_red() {
	echo -e "${RED}${1}${NC}$2" >&2
}

log_green() {
	echo -e "${GREEN}${1}${NC}$2" >&2
}

log_start() {
	printf "${YELLOW}%-50.50s${NC}$2" "$1" >&2
}

log_end() {
	printf "%s$2\n" "$1" >&2
}

# ~~ main routing point ~~
[ "$#" == 1 ] && case $1 in
  "--help") cmd_usage ;;
  "help") cmd_usage ;; 
  *) cmd_usage_short; exit 1 ;;
esac && exit 0
 
[ "$#" == 2 ] && case $1 in
  "up") 	auto_su; parse_options "$2"; cmd_up ;;
  "down") auto_su; parse_options "$2"; cmd_down ;; 
  "save") auto_su; parse_options "$2"; cmd_save ;;
  "strip") auto_su; parse_options "$2"; cmd_strip ;;
  *) cmd_usage_short; exit 1 ;;
esac && exit 0

cmd_usage_short
exit 1

