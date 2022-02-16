#!/bin/sh

start_ald() {
   logger -t "阿里云盘" "正在启动..."
   NAME=aliyundrive-webdav
   enable=$(nvram get aliyundrive_enable)
   case "$enable" in
    1|on|true|yes|enabled)
      refresh_token=$(nvram get ald_refresh_token)
      auth_user=$(nvram get ald_auth_user)
      auth_password=$(nvram get ald_auth_password)
      read_buf_size=$(nvram get ald_read_buffer_size)
      cache_size=$(nvram get ald_cache_size)
      cache_ttl=$(nvram get ald_cache_ttl)
      host=$(nvram get ald_host)
      port=$(nvram get ald_port)
      root=$(nvram get ald_root)
      domain_id=$(nvram get ald_domain_id)

      extra_options="-I"

      if [ "$domain_id" = "99999" ]; then
        extra_options="$extra_options --domain-id $domain_id"
      else
        case "$(nvram get ald_no_trash)" in
          1|on|true|yes|enabled)
            extra_options="$extra_options --no-trash"
            ;;
          *) ;;
        esac

        case "$(nvram get ald_read_only)" in
          1|on|true|yes|enabled)
            extra_options="$extra_options --read-only"
            ;;
          *) ;;
        esac
      fi
	/usr/bin/$NAME $extra_options --host $host --root $root -p $port  -r $refresh_token  -U $auth_user -W $auth_password -S $read_buf_size --cache-size $cache_size --cache-ttl $cache_ttl --workdir /tmp/$NAME >/dev/null 2>&1 &  
        ;;
    *)
      kill_ald ;;
  esac

}
kill_ald() {
	aliyundrive_process=$(pidof aliyundrive-webdav)
	if [ -n "$aliyundrive_process" ]; then
		logger -t "阿里云盘" "关闭进程..."
		killall aliyundrive-webdav >/dev/null 2>&1
		kill -9 "$aliyundrive_process" >/dev/null 2>&1
	fi
}
stop_ald() {
	kill_ald
	}



case $1 in
start)
	start_ald
	;;
stop)
	stop_ald
	;;
*)
	echo "check"
	#exit 0
	;;
esac
