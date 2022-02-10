################################################################################
# (sqm) functions.sh
#
# These are all helper functions for various parts of SQM scripts. If you want
# to play around with your own shaper-qdisc-filter configuration look here for
# ready made tools, or examples start of on your own.
#
# Please note the SQM logger function is broken down into levels of logging.
# Use only levels appropriate to touch points in your script and realize the
# potential to overflow SYSLOG.
#
################################################################################
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
#   Copyright (C) 2012-6
#       Michael D. Taht, Toke Høiland-Jørgensen, Sebastian Moeller
#       Eric Luehrsen
#
################################################################################

sqm_logger() {
    case $1 in
        ''|*[!0-9]*) LEVEL=$VERBOSITY_INFO ;; # empty or non-numbers
        *) LEVEL=$1; shift ;;
    esac

    if [ "$SQM_VERBOSITY_MAX" -ge "$LEVEL" ] && [ "$SQM_VERBOSITY_MIN" -le "$LEVEL" ] ; then
        if [ "$SQM_SYSLOG" -eq "1" ]; then
            logger -t SQM -s "$*"
        else
            echo "$@" >&2
        fi
    fi
    # slightly dangerous as this will keep adding to the log file
    if [ -n "${SQM_DEBUG}" -a "${SQM_DEBUG}" -eq "1" ]; then
        if [ "$SQM_VERBOSITY_MAX" -ge "$LEVEL" -o "$LEVEL" -eq "$VERBOSITY_TRACE" ]; then
            echo "$@" >> ${SQM_DEBUG_LOG}
        fi
    fi
}

sqm_error() { sqm_logger $VERBOSITY_ERROR ERROR: "$@"; }
sqm_warn() { sqm_logger $VERBOSITY_WARNING WARNING: "$@"; }
sqm_log() { sqm_logger $VERBOSITY_INFO "$@"; }
sqm_debug() { sqm_logger $VERBOSITY_DEBUG "$@"; }
sqm_trace() { sqm_logger $VERBOSITY_TRACE "$@"; }

# ipt needs a toggle to show the outputs for debugging (as do all users of >
# /dev/null 2>&1 and friends)
ipt() {
    d=$(echo $* | sed s/-A/-D/g)
    [ "$d" != "$*" ] && {
        sqm_trace "iptables ${d}"
        iptables $d >> ${OUTPUT_TARGET} 2>&1
        sqm_trace "ip6tables ${d}"
        ip6tables $d >> ${OUTPUT_TARGET} 2>&1
    }
    d=$(echo $* | sed s/-I/-D/g)
    [ "$d" != "$*" ] && {
        sqm_trace "iptables ${d}"
        iptables $d >> ${OUTPUT_TARGET} 2>&1
        sqm_trace "ip6tables ${d}"
        ip6tables $d >> ${OUTPUT_TARGET} 2>&1
    }
    sqm_trace "iptables $*"
    iptables $* >> ${OUTPUT_TARGET} 2>&1
    sqm_trace "ip6tables $*"
    ip6tables $* >> ${OUTPUT_TARGET} 2>&1
}

# wrapper to call tc to allow debug logging
tc_wrapper() {
    tc_args=$*
    sqm_trace "${TC_BINARY} $*"
    ${TC_BINARY} $* >> ${OUTPUT_TARGET} 2>&1
}

# wrapper to call tc to allow debug logging
ip_wrapper() {
    ip_args=$*
    sqm_trace "${IP_BINARY} $*"
    ${IP_BINARY} $* >> ${OUTPUT_TARGET} 2>&1
}


do_modules() {
    for m in $ALL_MODULES; do
        [ -d /sys/module/${m} ] || ${INSMOD} $m 2>>${OUTPUT_TARGET}
    done
}

# Write a state file to the filename given as $1. The remaining arguments are
# variable names that should be written to the state file.
write_state_file() {
    local filename=$1
    shift
    for var in "$@"; do
        val=$(eval echo '$'$var)
        echo "$var=\"$val\""
    done > $filename
}


# find the ifb device associated with a specific interface, return nothing of no
# ifb is associated with IF
get_ifb_associated_with_if() {
    local CUR_IF=$1
    # Stray ' in the comment is a fix for broken editor syntax highlighting
    local CUR_IFB=$( $TC_BINARY -p filter show parent ffff: dev ${CUR_IF} | grep -o -E ifb'[^)\ ]+' )    # '
    sqm_debug "ifb associated with interface ${CUR_IF}: ${CUR_IFB}"

    # we could not detect an associated IFB for CUR_IF
    if [ -z "${CUR_IFB}" ]; then
        local TMP=$( $TC_BINARY -p filter show parent ffff: dev ${CUR_IF} )
        if [ ! -z "${TMP}" ]; then
            # oops, there is output but we failed to properly parse it? Ask for a user report
            sqm_error "#---- CUT HERE ----#"
            sqm_error "get_ifb_associated_with_if failed to extrect the ifb name from:"
            sqm_error $( $TC_BINARY -p filter show parent ffff: dev ${CUR_IF} )
            sqm_error "Please report this as an issue at https://github.com/tohojo/sqm-scripts"
            sqm_error "Please copy and paste everything below the cut-here line into your issue report, thanks."
        else
            sqm_debug "Currently no ifb is associated with ${CUR_IF}, this is normal during starting of the sqm system."
        fi
    fi
    echo ${CUR_IFB}
}

ifb_name() {
    local CUR_IF=$1
    local MAX_IF_NAME_LENGTH=15
    local IFB_PREFIX="ifb4"
    local NEW_IFB="${IFB_PREFIX}${CUR_IF}"
    local IFB_NAME_LENGTH=${#NEW_IFB}
    # IFB names can only be 15 chararcters, so we chop of excessive characters
    # at the start of the interface name
    if [ ${IFB_NAME_LENGTH} -gt ${MAX_IF_NAME_LENGTH} ]; then
        local OVERLIMIT=$(( ${#NEW_IFB} - ${MAX_IF_NAME_LENGTH} ))
        NEW_IFB=${IFB_PREFIX}${CUR_IF:${OVERLIMIT}:$(( ${MAX_IF_NAME_LENGTH} - ${#IFB_PREFIX} ))}
    fi
    echo ${NEW_IFB}
}

# if required
create_new_ifb_for_if() {
    local NEW_IFB=$(ifb_name $1)
    create_ifb ${NEW_IFB}
    echo $NEW_IFB
    return $?
}


# TODO: report failures
create_ifb() {
    local CUR_IFB=${1}
    $IP link add name ${CUR_IFB} type ifb
    ret=$?
    return $?
}

delete_ifb() {
    local CUR_IFB=${1}
    $IP link set dev ${CUR_IFB} down
    $IP link delete ${CUR_IFB} type ifb
    return $?
}


# the best match is either the IFB already associated with the current interface
# or a new named IFB
get_ifb_for_if() {
    local CUR_IF=$1
    # if an ifb is already associated return that
    local CUR_IFB=$( get_ifb_associated_with_if ${CUR_IF} )
    [ -z "$CUR_IFB" ] && CUR_IFB=$( create_new_ifb_for_if ${CUR_IF} )
    [ -z "$CUR_IFB" ] && sqm_warn "Could not find existing IFB for ${CUR_IF}, nor create a new IFB instead..."
    echo ${CUR_IFB}
}


# Verify that a qdisc works, and optionally that it is part of a set of
# supported qdiscs. If passed a $2, this function will first check if $1 is in
# that (space-separated) list and return an error if it's not.
#
# note the ingress qdisc is different in that it requires tc qdisc replace dev
# tmp_ifb ingress instead of "root ingress"
verify_qdisc() {
    local qdisc=$1
    local supported="$2"
    local ifb=TMP_IFB_4_SQM
    local root_string="root" # this works for most qdiscs
#    local root_string=`nvram get http_username`
    local args=""

    if [ -n "$supported" ]; then
        local found=0
        for q in $supported; do
            [ "$qdisc" = "$q" ] && found=1
        done
        [ "$found" -eq "1" ] || return 1
    fi
    create_ifb $ifb || return 1
    case $qdisc in
        #ingress is special
        ingress) root_string="" ;;
        #cannot instantiate tbf without args
        tbf) args="limit 1 burst 1 rate 1kbps" ;;
    esac

    $TC qdisc replace dev $ifb $root_string $qdisc $args
    res=$?
    if [ "$res" = "0" ] ; then
        sqm_debug "QDISC $qdisc is useable."
    else
        sqm_error "QDISC $qdisc is NOT useable."
    fi
    delete_ifb $ifb
    return $res
}


get_htb_adsll_string() {
    ADSLL=""
    if [ "$LLAM" = "htb_private" -a "$LINKLAYER" != "none" ]; then
        # HTB defaults to MTU 1600 and an implicit fixed TSIZE of 256, but HTB
        # as of around 3.10.0 does not actually use a table in the kernel
        ADSLL="mpu ${STAB_MPU} linklayer ${LINKLAYER} overhead ${OVERHEAD} mtu ${STAB_MTU}"
        sqm_debug "ADSLL: ${ADSLL}"
    fi
    echo ${ADSLL}
}

get_stab_string() {
    STABSTRING=""
    local TMP_LLAM=${LLAM}
    if [ "${LLAM}" = "default" -a "$QDISC" != "cake" ]; then
	sqm_debug "LLA: default link layer adjustment method for !cake is tc_stab"
	TMP_LLAM="tc_stab"
    fi

    if [ "${TMP_LLAM}" = "tc_stab" -a "$LINKLAYER" != "none" ]; then
        STABSTRING="stab mtu ${STAB_MTU} tsize ${STAB_TSIZE} mpu ${STAB_MPU} overhead ${OVERHEAD} linklayer ${LINKLAYER}"
        sqm_debug "STAB: ${STABSTRING}"
    fi
    echo ${STABSTRING}
}

# cake knows how to handle ATM and per packet overhead, so expose and use this...
get_cake_lla_string() {
    STABSTRING=""
    local TMP_LLAM=${LLAM}
    if [ "${LLAM}" = "default" -a "$QDISC" = "cake" ]; then
	sqm_debug "LLA: default link layer adjustment method for cake is cake"
	TMP_LLAM="cake"
    fi

    if [ "${TMP_LLAM}" = "cake" -a "${LINKLAYER}" != "none" ]; then
        if [ "${LINKLAYER}" = "atm" ]; then
            STABSTRING="atm"
        fi

        STABSTRING="${STABSTRING} overhead ${OVERHEAD}"
        sqm_debug "cake link layer adjustments: ${STABSTRING}"
    fi
    echo ${STABSTRING}
}


sqm_stop() {
    $TC qdisc del dev $IFACE ingress #2>> ${OUTPUT_TARGET}
    $TC qdisc del dev $IFACE root #2>> ${OUTPUT_TARGET}
    [ -n "$CUR_IFB" ] && $TC qdisc del dev $CUR_IFB root #2>> ${OUTPUT_TARGET}
    [ -n "$CUR_IFB" ] && sqm_debug "${0}: ${CUR_IFB} shaper deleted"

    [ -n "$CUR_IFB" ] && ipt -t mangle -D POSTROUTING -o $CUR_IFB -m mark --mark 0x00 -g QOS_MARK_${IFACE}
    ipt -t mangle -D POSTROUTING -o $IFACE -m mark --mark 0x00/${IPT_MASK} -g QOS_MARK_${IFACE}
    ipt -t mangle -D PREROUTING -i vtun+ -p tcp -j MARK --set-mark 0x2/${IPT_MASK}
    # not sure whether we need to make this conditional or whether they are
    # silent if the deletion does not work out
    ipt -t mangle -D PREROUTING -i $IFACE -m dscp ! --dscp 0 -j DSCP --set-dscp-class be
    ipt -t mangle -D PREROUTING -i $IFACE -m mark --mark 0x00/${IPT_MASK} -g QOS_MARK_${IFACE}

    ipt -t mangle -D OUTPUT -p udp -m multiport --ports 123,53 -j DSCP --set-dscp-class AF42
    ipt -t mangle -F QOS_MARK_${IFACE}
    ipt -t mangle -X QOS_MARK_${IFACE}


    [ -n "$CUR_IFB" ] && $IP link set dev ${CUR_IFB} down
    [ -n "$CUR_IFB" ] && $IP link delete ${CUR_IFB} type ifb
    [ -n "$CUR_IFB" ] && sqm_debug "${0}: ${CUR_IFB} interface deleted"
}
# Note this has side effects on the prio variable
# and depends on the interface global too

fc() {
    $TC filter add dev $interface protocol ip parent $1 prio $prio u32 match ip tos $2 0xfc classid $3
    prio=$(($prio + 1))
    $TC filter add dev $interface protocol ipv6 parent $1 prio $prio u32 match ip6 priority $2 0xfc classid $3
    prio=$(($prio + 1))
}

# Scale quantum with bandwidth to lower computation cost.
get_htb_quantum() {
    case "$HTB_QUANTUM_FUNCTION" in
        linear|step)
            eval "htb_quantum_${HTB_QUANTUM_FUNCTION} $1 $2" ;;
        *)
            sqm_warn "unknown HTB quantum function: '$HTB_QUANTUM_FUNCTION'."
            echo 1500 ;; # fallback
    esac
}

# Linear scaling within bounds
htb_quantum_linear() {
    HTB_MTU=$( get_mtu $1 )
    BANDWIDTH=$2

    if [ -z "${HTB_MTU}" ] ; then
        HTB_MTU=1500
    fi

    # Because $BANDWIDTH is in kbps, bytes/1ms is simply factor-8
    # Stop at some huge quantum, and don't start until 2 MTU
    BANDWIDTH_L=$(( ${HTB_MTU} *  2 * 8 ))
    BANDWIDTH_H=$(( ${HTB_MTU} * 64 * 8 ))

    if [ ${BANDWIDTH} -gt ${BANDWIDTH_H} ] ; then
        HTB_QUANTUM=$(( ${HTB_MTU} * 64 ))

    elif [ ${BANDWIDTH} -gt ${BANDWIDTH_L} ] ; then
        # Take no chances with order o' exec rounding
        HTB_QUANTUM=$(( ${BANDWIDTH}   / 8 ))
        HTB_QUANTUM=$(( ${HTB_QUANTUM} / ${HTB_MTU} ))
        HTB_QUANTUM=$(( ${HTB_QUANTUM} * ${HTB_MTU} ))

    else
        HTB_QUANTUM=${HTB_MTU}
    fi

    sqm_debug "HTB_QUANTUM (linear): ${HTB_QUANTUM}, BANDWIDTH: ${BANDWIDTH}"

    echo $HTB_QUANTUM
}

# Fixed step scaling
htb_quantum_step() {
    HTB_QUANTUM=$( get_mtu $1 )
    BANDWIDTH=$2

    if [ -z "${HTB_QUANTUM}" ]; then
        HTB_QUANTUM=1500
    fi
    if [ ${BANDWIDTH} -gt 20000 ]; then
        HTB_QUANTUM=$((${HTB_QUANTUM} * 2))
    fi
    if [ ${BANDWIDTH} -gt 30000 ]; then
        HTB_QUANTUM=$((${HTB_QUANTUM} * 2))
    fi
    if [ ${BANDWIDTH} -gt 40000 ]; then
        HTB_QUANTUM=$((${HTB_QUANTUM} * 2))
    fi
    if [ ${BANDWIDTH} -gt 50000 ]; then
        HTB_QUANTUM=$((${HTB_QUANTUM} * 2))
    fi
    if [ ${BANDWIDTH} -gt 60000 ]; then
        HTB_QUANTUM=$((${HTB_QUANTUM} * 2))
    fi
    if [ ${BANDWIDTH} -gt 80000 ]; then
        HTB_QUANTUM=$((${HTB_QUANTUM} * 2))
    fi

    sqm_debug "HTB_QUANTUM (step): ${HTB_QUANTUM}, BANDWIDTH: ${BANDWIDTH}"

    echo $HTB_QUANTUM
}


get_burst() {
    MTU=$1
    BANDWIDTH=$2
    BURST=

    # 10 MTU burst can itself create delay under CPU load.
    # It will need to all wait for a hardware commit.
    # Note the lean mixture at high bandwidths for upper limit.
    BANDWIDTH_L=$(( ${MTU} *  2 * 8 ))
    BANDWIDTH_H=$(( ${MTU} * 18 * 8 ))


    if [ ${BANDWIDTH} -gt ${BANDWIDTH_H} ] ; then
        BURST=$(( ${HTB_MTU} * 10 ))

    elif [ ${BANDWIDTH} -gt ${BANDWIDTH_L} ] ; then
            # Start with 1ms buffer 2x MTU, and lean out the mixture at higher rates
            BURST=$(( ${BANDWIDTH} - ${BANDWIDTH_L} ))
            BURST=$(( ${BURST} / 16 ))
            BURST=$(( ${BURST} / ${MTU} ))
            BURST=$(( ${BURST} * ${MTU} ))
            BURST=$(( ${BURST} + ${MTU} * 2 ))
    fi

    sqm_debug "BURST: ${BURST}, BANDWIDTH: ${BANDWIDTH}"

    echo $BURST
}

# Create optional burst parameters to leap over CPU interupts when the CPU is
# severly loaded. We need to be conservative though.
get_htb_burst() {
    HTB_MTU=$( get_mtu $1 )
    BANDWIDTH=$2

    if [ -n "${HTB_MTU}" -a "${SHAPER_BURST}" -eq "1" ] ; then
        BURST=$( get_burst $HTB_MTU $BANDWIDTH )
        if [ -n "$BURST" ]; then
            echo burst $BURST cburst $BURST
        else
            sqm_debug "Default Burst, HTB will use MTU plus shipping and handling"
        fi
    fi
}

# For a default PPPoE link this returns 1492 just as expected but I fear we
# actually need the wire size of the whole thing not so much the MTU
get_mtu() {
    CUR_MTU=$(cat /sys/class/net/$1/mtu)
    sqm_debug "IFACE: ${1} MTU: ${CUR_MTU}"
    echo ${CUR_MTU}
}

# Set the autoflow variable to 1 if you want to limit the number of flows
# otherwise the default of 1024 will be used for all Xfq_codel qdiscs.

get_flows() {
    case $QDISC in
        codel|ns2_codel|pie|*fifo|pfifo_fast) ;;
        fq_codel|*fq_codel|sfq) echo flows $( get_flows_count ${1} ) ;;
    esac
}

get_flows_count() {
    if [ "${AUTOFLOW}" -eq "1" ]; then
        FLOWS=8
        [ $1 -gt 999 ] && FLOWS=16
        [ $1 -gt 2999 ] && FLOWS=32
        [ $1 -gt 7999 ] && FLOWS=48
        [ $1 -gt 9999 ] && FLOWS=64
        [ $1 -gt 19999 ] && FLOWS=128
        [ $1 -gt 39999 ] && FLOWS=256
        [ $1 -gt 69999 ] && FLOWS=512
        [ $1 -gt 99999 ] && FLOWS=1024
        case $QDISC in
          codel|ns2_codel|pie|*fifo|pfifo_fast) ;;
          fq_codel|*fq_codel|sfq) echo $FLOWS ;;
        esac
    else
        case $QDISC in
          codel|ns2_codel|pie|*fifo|pfifo_fast) ;;
          fq_codel|*fq_codel|sfq) echo 1024 ;;
        esac
    fi
}

# set the target parameter, also try to only take well formed inputs
# Note, the link bandwidth in the current direction (ingress or egress)
# is required to adjust the target for slow links
get_target() {
    local CUR_TARGET=${1}
    local CUR_LINK_KBPS=${2}
    [ ! -z "$CUR_TARGET" ] && sqm_debug "cur_target: ${CUR_TARGET} cur_bandwidth: ${CUR_LINK_KBPS}"
    CUR_TARGET_STRING=
    # either e.g. 100ms or auto
    CUR_TARGET_VALUE=$( echo ${CUR_TARGET} | grep -o -e \^'[[:digit:]]\+' )
    CUR_TARGET_UNIT=$( echo ${CUR_TARGET} | grep -o -e '[[:alpha:]]\+'\$ )

    AUTO_TARGET=
    UNIT_VALID=

    case $QDISC in
        *codel|*pie)
            if [ ! -z "${CUR_TARGET_VALUE}" -a ! -z "${CUR_TARGET_UNIT}" ];
            then
                case ${CUR_TARGET_UNIT} in
                    # permissible units taken from: tc_util.c get_time()
                    s|sec|secs|ms|msec|msecs|us|usec|usecs)
                        CUR_TARGET_STRING="target ${CUR_TARGET_VALUE}${CUR_TARGET_UNIT}"
                        UNIT_VALID="1"
                        ;;
                esac
            fi
            # empty field in GUI or undefined GUI variable now defaults to auto
            if [ -z "${CUR_TARGET_VALUE}" -a -z "${CUR_TARGET_UNIT}" ];
            then
                if [ ! -z "${CUR_LINK_KBPS}" ]; then
                    TMP_TARGET_US=$( adapt_target_to_slow_link $CUR_LINK_KBPS )
                    TMP_INTERVAL_STRING=$( adapt_interval_to_slow_link $TMP_TARGET_US )
                    CUR_TARGET_STRING="target ${TMP_TARGET_US}us ${TMP_INTERVAL_STRING}"
                    AUTO_TARGET="1"
                    sqm_debug "get_target defaulting to auto."
                else
                    sqm_warn "required link bandwidth in kbps not passed to get_target()."
                fi
            fi
            # but still allow explicit use of the keyword auto for backward compatibility
            case ${CUR_TARGET_UNIT} in
                auto|Auto|AUTO)
                    if [ ! -z "${CUR_LINK_KBPS}" ]; then
                        TMP_TARGET_US=$( adapt_target_to_slow_link $CUR_LINK_KBPS )
                        TMP_INTERVAL_STRING=$( adapt_interval_to_slow_link $TMP_TARGET_US )
                        CUR_TARGET_STRING="target ${TMP_TARGET_US}us ${TMP_INTERVAL_STRING}"
                        AUTO_TARGET="1"
                    else
                        sqm_warn "required link bandwidth in kbps not passed to get_target()."
                    fi
                    ;;
            esac

            case ${CUR_TARGET_UNIT} in
                default|Default|DEFAULT)
                    if [ ! -z "${CUR_LINK_KBPS}" ]; then
                        CUR_TARGET_STRING=""    # return nothing so the default target is not over-ridden...
                        AUTO_TARGET="1"
                        sqm_debug "get_target using qdisc default, no explicit target string passed."
                    else
                        sqm_warn "required link bandwidth in kbps not passed to get_target()."
                    fi
                    ;;
            esac
            if [ ! -z "${CUR_TARGET}" ]; then
                if [ -z "${CUR_TARGET_VALUE}" -o -z "${UNIT_VALID}" ]; then
                    [ -z "$AUTO_TARGET" ] && sqm_warn "${CUR_TARGET} is not a well formed tc target specifier; e.g.: 5ms (or s, us), or one of the strings auto or default."
                fi
            fi
            ;;
    esac
    echo $CUR_TARGET_STRING
}

# for low bandwidth links fq_codels default target of 5ms does not work too well
# so increase target for slow links (note below roughly 2500kbps a single packet
# will take more than 5 ms to be tansfered over the wire)
adapt_target_to_slow_link() {
    LINK_BW=$1
    # for ATM the worst case expansion including overhead seems to be 33 clls of
    # 53 bytes each
    MAX_DELAY=$(( 1000 * 1000 * 33 * 53 * 8 / 1000 )) # Max delay in us at 1kbps
    TARGET=$(( ${MAX_DELAY} / ${LINK_BW} ))  # note this truncates the decimals

    # do not change anything for fast links
    [ "$TARGET" -lt 5000 ] && TARGET=5000
    case ${QDISC} in
        *codel|pie)
            echo "${TARGET}"
            ;;
    esac
}

# codel looks at a whole interval to figure out wether observed latency stayed
# below target if target >= interval that will not work well, so increase
# interval by the same amonut that target got increased
adapt_interval_to_slow_link() {
    TARGET=$1
    case ${QDISC} in
        *codel)
            # Note this is not following codel theory to well as target should
            # be 5-10% of interval and the simple addition does not conserve
            # that relationship
            INTERVAL=$(( (100 - 5) * 1000 + ${TARGET} ))
            echo "interval ${INTERVAL}us"
            ;;
        pie)
            ## not sure if pie needs this, probably not
            #TUPDATE=$(( (30 - 20) * 1000 + ${TARGET} ))
            #echo "tupdate ${TUPDATE}us"
            ;;
    esac
}


# set quantum parameter if available for this qdisc
get_quantum() {
    case $QDISC in
        *fq_codel|fq_pie|drr) echo quantum $1 ;;
        *) ;;
    esac
}

# only show limits to qdiscs that can handle them...
# Note that $LIMIT contains the default limit
get_limit() {
    CURLIMIT=$1
    case $QDISC in
        *codel|*pie|pfifo_fast|sfq|pfifo) [ -z ${CURLIMIT} ] && CURLIMIT=${LIMIT}  # global default limit
                                          ;;
        bfifo) [ -z "$CURLIMIT" ] && [ ! -z "$LIMIT" ] && CURLIMIT=$(( ${LIMIT} * $( cat /sys/class/net/${IFACE}/mtu ) ))    # bfifo defaults to txquelength * MTU,
               ;;
        *) sqm_warn "qdisc ${QDISC} does not support a limit"
           ;;
    esac
    sqm_debug "get_limit: $1 CURLIMIT: ${CURLIMIT}"

    if [ ! -z "$CURLIMIT" ]; then
        echo "limit ${CURLIMIT}"
    fi
}

get_ecn() {
    CURECN=$1
    case ${CURECN} in
        ECN)
            case $QDISC in
                *codel|*pie|*red)
                    CURECN=ecn
                    ;;
                *)
                    CURECN=""
                    ;;
            esac
            ;;
        NOECN)
            case $QDISC in
                *codel|*pie|*red)
                    CURECN=noecn
                    ;;
                *)
                    CURECN=""
                    ;;
            esac
            ;;
        *)
            sqm_warn "ecn value $1 not handled"
            ;;
    esac
    sqm_debug "get_ECN: $1 CURECN: ${CURECN} IECN: ${IECN} EECN: ${EECN}"
    echo ${CURECN}

}

# This could be a complete diffserv implementation

diffserv() {

    interface=$1
    prio=1

    # Catchall

    $TC filter add dev $interface parent 1:0 protocol all prio 999 u32 \
        match ip protocol 0 0x00 flowid 1:12

    # Find the most common matches fast

    fc 1:0 0x00 1:12 # BE
    fc 1:0 0x20 1:13 # CS1
    fc 1:0 0x10 1:11 # IMM
    fc 1:0 0xb8 1:11 # EF
    fc 1:0 0xc0 1:11 # CS3
    fc 1:0 0xe0 1:11 # CS6
    fc 1:0 0x90 1:11 # AF42 (mosh)

    # Arp traffic
    $TC filter add dev $interface protocol arp parent 1:0 prio $prio handle 500 fw flowid 1:11

    prio=$(($prio + 1))
}

eth_setup() {
    ethtool -K $IFACE gso off
    ethtool -K $IFACE tso off
    ethtool -K $IFACE ufo off
    ethtool -K $IFACE gro off

    if [ -e /sys/class/net/$IFACE/queues/tx-0/byte_queue_limits ]; then
       for i in /sys/class/net/$IFACE/queues/tx-*/byte_queue_limits
       do
          echo $(( 4 * $( get_mtu ${IFACE} ) )) > $i/limit_max
       done
    fi
}
