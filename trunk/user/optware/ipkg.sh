#!/bin/sh

# ipkg - the itsy package management system
#
# Copyright (C) 2001 Carl D. Worth
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

set -e

if [ -f /opt/bin/opkg ] ; then
	echo "WARNING! Entware detected. Please remove Entware first!"
	exit 1
fi

PATH=/usr/sbin:/usr/bin:/sbin:/bin
export PATH

# By default do not do globbing. Any command wanting globbing should
# explicitly enable it first and disable it afterwards.
set -o noglob

ipkg_is_upgrade () {
  local A B a b
  A=$(echo $1 | sed "s/[0-9]*/ & /g")
  B=$(echo $2 | sed "s/[0-9]*/ & /g")
  while [ \! -z "$A" ] && [ \! -z "$B" ]; do {
    set $A; a=$1; shift; A=$*
    set $B; b=$1; shift; B=$*
    { [ "$a" -gt "$b" ] 2>&- || [ "$a" ">" "$b" ]; } && { return 0; }
  }; done
  return 1;
}

ipkg_srcs() {
	local srcre="$1"
	sed -ne "s/^src[[:space:]]\+$srcre[[:space:]]\+//p" < $IPKG_CONF
}

ipkg_src_names() {
	sed -ne "s/^src[[:space:]]\+\([^[:space:]]\+\).*/\1/p" < $IPKG_CONF
}

ipkg_src_byname() {
	local src="$1"
	ipkg_srcs $src | head -1
}

ipkg_dests() {
	local destre="`echo $1 | ipkg_protect_slashes`"
	sed -ne "/^dest[[:space:]]\+$destre/{
s/^dest[[:space:]]\+[^[:space:]]\+[[:space:]]\+//
s/^/`echo $IPKG_OFFLINE_ROOT | ipkg_protect_slashes`/
p
}" < $IPKG_CONF
}

ipkg_dest_names() {
	sed -ne "s/^dest[[:space:]]\+\([^[:space:]]\+\).*/\1/p" < $IPKG_CONF
}

ipkg_dests_all() {
	ipkg_dests '.*'
}

ipkg_state_dirs() {
	ipkg_dests_all | sed "s|\$|/$IPKG_DIR_PREFIX|"
}

ipkg_dest_default() {
	ipkg_dests_all | head -1
}

ipkg_dest_default_name() {
	ipkg_dest_names | head -1
}

ipkg_dest_byname() {
	local dest="$1"
	ipkg_dests $dest | head -1
}

ipkg_option() {
	local option="$1"
	sed -ne "s/^option[[:space:]]\+$option[[:space:]]\+//p" < $IPKG_CONF
}

ipkg_load_configuration() {
	if [ -z "$IPKG_CONF_DIR" ]; then
		IPKG_CONF_DIR=/etc_ro
	fi

	IPKG_CONF="$IPKG_CONF_DIR/ipkg.conf"

	if [ -z "$IPKG_OFFLINE_ROOT" ]; then
	    IPKG_OFFLINE_ROOT="`ipkg_option offline_root`"
	fi
	# Export IPKG_OFFLINE_ROOT for use by update-alternatives
	export IPKG_OFFLINE_ROOT
	if [ -n "$DEST_NAME" ]; then
		IPKG_ROOT="`ipkg_dest_byname $DEST_NAME`"
		if [ -z "$IPKG_ROOT" ]; then
			if [ -d "$IPKG_OFFLINE_ROOT$DEST_NAME" ]; then
				IPKG_ROOT="$IPKG_OFFLINE_ROOT$DEST_NAME";
			else
				echo "ipkg: invalid destination specification: $DEST_NAME
Valid destinations are directories or one of the dest names from $IPKG_CONF:" >&2
				ipkg_dest_names >&2
				return 1
			fi
		fi
	else
		IPKG_ROOT="`ipkg_dest_default`"
	fi

	# Global ipkg state directories
	IPKG_DIR_PREFIX=opt/lib/ipkg
	IPKG_LISTS_DIR=$IPKG_OFFLINE_ROOT/$IPKG_DIR_PREFIX/lists
	IPKG_PENDING_DIR=$IPKG_OFFLINE_ROOT/$IPKG_DIR_PREFIX/pending
	IPKG_TMP=$IPKG_ROOT/opt/tmp/ipkg

	# Destination specific ipkg meta-data directory
	IPKG_STATE_DIR=$IPKG_ROOT/$IPKG_DIR_PREFIX

	# Proxy Support
	IPKG_PROXY_USERNAME="`ipkg_option proxy_username`"
	IPKG_PROXY_PASSWORD="`ipkg_option proxy_password`"
	IPKG_HTTP_PROXY="`ipkg_option http_proxy`"
	IPKG_FTP_PROXY="`ipkg_option ftp_proxy`"
	IPKG_NO_PROXY="`ipkg_option no_proxy`"
	if [ -n "$IPKG_HTTP_PROXY" ]; then 
		export http_proxy="$IPKG_HTTP_PROXY"
	fi

	if [ -n "$IPKG_FTP_PROXY" ]; then 
		export ftp_proxy="$IPKG_FTP_PROXY"
	fi

	if [ -n "$IPKG_NO_PROXY" ]; then 
		export no_proxy="$IPKG_NO_PROXY"
	fi

	IPKG_STATUS_FIELDS='\(Package\|Status\|Essential\|Version\|Conffiles\|Root\)'
}

ipkg_usage() {
	[ $# -gt 0 ] && echo "ipkg: $*"
	echo "
usage: ipkg [options...] sub-command [arguments...]
where sub-command is one of:

Package Manipulation:
	update  		Update list of available packages
	upgrade			Upgrade all installed packages to latest version
	install <pkg>		Download and install <pkg> (and dependencies)
	install <file.ipk>	Install package <file.ipk>
	install <file.deb>	Install package <file.deb>
	remove <pkg>		Remove package <pkg>

Informational Commands:
	list    		List available packages and descriptions
	files <pkg>		List all files belonging to <pkg>
	search <file>		Search for a packaging providing <file>
	info [pkg [<field>]]	Display all/some info fields for <pkg> or all
	status [pkg [<field>]]	Display all/some status fields for <pkg> or all
	depends <pkg>		Print uninstalled package dependencies for <pkg>

Options:
	-d <dest_name>          Use <dest_name> as the the root directory for
	-dest <dest_name>	package installation, removal, upgrading.
				<dest_name> should be a defined dest name from the
				configuration file, (but can also be a directory
				name in a pinch).
        -o <offline_root>       Use <offline_root> as the root for offline installation.
        -offline <offline_root> 				

Force Options (use when ipkg is too smart for its own good):
	-force-depends          Make dependency checks warnings instead of errors
	-force-defaults         Use default options for questions asked by ipkg.
                                (no prompts). Note that this will not prevent
                                package installation scripts from prompting.
" >&2
	exit 1
}

ipkg_dir_part() {
	local dir="`echo $1 | sed -ne 's/\(.*\/\).*/\1/p'`"
	if [ -z "$dir" ]; then
		dir="./"
	fi
	echo $dir
}

ipkg_file_part() {
	echo $1 | sed 's/.*\///'
}

ipkg_protect_slashes() {
	sed -e 's/\//\\\//g'
}

ipkg_download() {
	local src="$1"
	local dest="$2"

	local src_file="`ipkg_file_part $src`"
	local dest_dir="`ipkg_dir_part $dest`"
	if [ -z "$dest_dir" ]; then
		dest_dir="$IPKG_TMP"
	fi

	local dest_file="`ipkg_file_part $dest`"
	if [ -z "$dest_file" ]; then
		dest_file="$src_file"
	fi

	# Proxy support
	local proxyuser=""
	local proxypassword=""
	local proxyoption=""
		
	if [ -n "$IPKG_PROXY_USERNAME" ]; then
		proxyuser="--proxy-user=\"$IPKG_PROXY_USERNAME\""
		proxypassword="--proxy-passwd=\"$IPKG_PROXY_PASSWORD\""
	fi

	if [ -n "$IPKG_PROXY_HTTP" -o -n "$IPKG_PROXY_FTP" ]; then
		proxyoption="--proxy=on"
	fi

	echo "Downloading $src ..."
	rm -f $IPKG_TMP/$src_file
	case "$src" in
	http://* | ftp://*)
		if ! wget --passive-ftp $proxyoption $proxyuser $proxypassword -P $IPKG_TMP $src; then
			echo "ipkg_download: ERROR: Failed to retrieve $src, returning $err"
			return 1
		fi
		mv $IPKG_TMP/$src_file $dest_dir/$dest_file 2>/dev/null
		;;
	file:/* )
		ln -s `echo $src | sed 's/^file://'` $dest_dir/$dest_file 2>/dev/null
		;;
	*)
	echo "DEBUG: $src"
		;;
	esac

	echo "Done."
	return 0
}

ipkg_update() {
	if [ ! -e "$IPKG_LISTS_DIR" ]; then
		mkdir -p $IPKG_LISTS_DIR
	fi

	local err=
	for src_name in `ipkg_src_names`; do
		local src="`ipkg_src_byname $src_name`"
		if ! ipkg_download $src/Packages $IPKG_LISTS_DIR/$src_name; then
			echo "ipkg_update: Error downloading $src/Packages to $IPKG_LISTS_DIR/$src_name" >&2
			err=t
		else
			echo "Updated list of available packages in $IPKG_LISTS_DIR/$src_name"
		fi
	done

	[ -n "$err" ] && return 1

	return 0
}

ipkg_list() {
	for src in `ipkg_src_names`; do
		if ipkg_require_list $src; then 
# black magic...
sed -ne "
/^Package:/{
s/^Package:[[:space:]]*\<\([a-z0-9.+-]*$1[a-z0-9.+-]*\).*/\1/
h
}
/^Description:/{
s/^Description:[[:space:]]*\(.*\)/\1/
H
g
s/\\
/ - /
p
}
" $IPKG_LISTS_DIR/$src
		fi
	done
}

ipkg_extract_paragraph() {
	local pkg="$1"
	sed -ne "/Package:[[:space:]]*$pkg[[:space:]]*\$/,/^\$/p"
}

ipkg_extract_field() {
	local field="$1"
# blacker magic...
	sed -ne "
: TOP
/^$field:/{
p
n
b FIELD
}
d
: FIELD
/^$/b TOP
/^[^[:space:]]/b TOP
p
n
b FIELD
"
}

ipkg_extract_value() {
	sed -e "s/^[^:]*:[[:space:]]*//"
}

ipkg_require_list() {
	[ $# -lt 1 ] && return 1
	local src="$1"
	if [ ! -f "$IPKG_LISTS_DIR/$src" ]; then
		echo "ERROR: File not found: $IPKG_LISTS_DIR/$src" >&2
		echo "       You probably want to run \`ipkg update'" >&2
		return 1
	fi
	return 0
}

ipkg_info() {
	for src in `ipkg_src_names`; do
		if ipkg_require_list $src; then
			case $# in
			0)
				cat $IPKG_LISTS_DIR/$src
				;;	
			1)
				ipkg_extract_paragraph $1 < $IPKG_LISTS_DIR/$src
				;;
			*)
				ipkg_extract_paragraph $1 < $IPKG_LISTS_DIR/$src | ipkg_extract_field $2
				;;
			esac
		fi
	done
}

ipkg_status_sd() {
	[ $# -lt 1 ] && return 0
	sd="$1"
	shift
	if [ -f $sd/status ]; then
		case $# in
		0)
			cat $sd/status
			;;
		1)
			ipkg_extract_paragraph $1 < $sd/status
			;;
		*)
			ipkg_extract_paragraph $1 < $sd/status | ipkg_extract_field $2
			;;
		esac
	fi
	return 0
}

ipkg_status_all() {
	for sd in `ipkg_state_dirs`; do
		ipkg_status_sd $sd $*
	done
}

ipkg_status() {
	if [ -n "$DEST_NAME" ]; then
		ipkg_status_sd $IPKG_STATE_DIR $*
	else
		ipkg_status_all $*
	fi
}

ipkg_status_matching_sd() {
	local sd="$1"
	local re="$2"
	if [ -f $sd/status ]; then
		sed -ne "
: TOP
/^Package:/{
s/^Package:[[:space:]]*//
s/[[:space:]]*$//
h
}
/$re/{
g
p
b NEXT
}
d
: NEXT
/^$/b TOP
n
b NEXT
" < $sd/status
	fi
	return 0
}

ipkg_status_matching_all() {
	for sd in `ipkg_state_dirs`; do
		ipkg_status_matching_sd $sd $*
	done
}

ipkg_status_matching() {
	if [ -n "$DEST_NAME" ]; then
		ipkg_status_matching_sd $IPKG_STATE_DIR $*
	else
		ipkg_status_matching_all $*
	fi
}

ipkg_status_installed_sd() {
	local sd="$1"
	local pkg="$2"
	ipkg_status_sd $sd $pkg Status | grep -q "Status: install ok installed"
}

ipkg_status_installed_all() {
	local ret=1
	for sd in `ipkg_state_dirs`; do
		if `ipkg_status_installed_sd $sd $*`; then
			ret=0
		fi
	done
	return $ret
}

ipkg_status_mentioned_sd() {
	local sd="$1"
	local pkg="$2"
	[ -n "`ipkg_status_sd $sd $pkg Status`" ]
}

ipkg_files() {
	local pkg="$1"
	if [ -n "$DEST_NAME" ]; then
		dests=$IPKG_ROOT
	else
		dests="`ipkg_dests_all`"
	fi
	for dest in $dests; do
		if [ -f $dest/$IPKG_DIR_PREFIX/info/$pkg.list ]; then
			dest_sed="`echo $dest | ipkg_protect_slashes`"
			sed -e "s/^/$dest_sed/" < $dest/$IPKG_DIR_PREFIX/info/$pkg.list
		fi
	done
}

ipkg_search() {
	local pattern="$1"

	for dest_name in `ipkg_dest_names`; do
		dest="`ipkg_dest_byname $dest_name`"
		dest_sed="`echo $dest | ipkg_protect_slashes`"

		set +o noglob
		local list_files="`ls -1 $dest/$IPKG_DIR_PREFIX/info/*.list 2>/dev/null`"
		set -o noglob
		for file in $list_files; do
			if sed "s/^/$dest_sed/" $file | grep -q $pattern; then
				local pkg="`echo $file | sed "s/^.*\/\(.*\)\.list/\1/"`"
				[ "$dest_name" != `ipkg_dest_default_name` ] && pkg="$pkg ($dest_name)"
				sed "s/^/$dest_sed/" $file | grep $pattern | sed "s/^/$pkg: /"
			fi
		done
	done
}

ipkg_status_remove_sd() {
	local sd="$1"
	local pkg="$2"

	if [ ! -f $sd/status ]; then
		mkdir -p $sd
		touch $sd/status
	fi
	sed -ne "/Package:[[:space:]]*$pkg[[:space:]]*\$/,/^\$/!p" < $sd/status > $sd/status.new
	mv $sd/status.new $sd/status
}

ipkg_status_remove_all() {
	for sd in `ipkg_state_dirs`; do
		ipkg_status_remove_sd $sd $*
	done
}

ipkg_status_remove() {
	if [ -n "$DEST_NAME" ]; then
		ipkg_status_remove_sd $IPKG_STATE_DIR $*
	else
		ipkg_status_remove_all $*
	fi
}

ipkg_status_update_sd() {
	local sd="$1"
	local pkg="$2"

	ipkg_status_remove_sd $sd $pkg
	ipkg_extract_field "$IPKG_STATUS_FIELDS" >> $sd/status
	echo "" >> $sd/status
}

ipkg_status_update() {
	ipkg_status_update_sd $IPKG_STATE_DIR $*
}

ipkg_unsatisfied_dependences() {
    local pkg=$1
    local deps="`ipkg_get_depends $pkg`"
    local remaining_deps=
    for dep in $deps; do
	local installed="`ipkg_get_installed $dep`"
	if [ "$installed" != "installed" ] ; then
	    remaining_deps="$remaining_deps $dep"
	fi
    done
    ## echo "ipkg_unsatisfied_dependences pkg=$pkg $remaining_deps" > /dev/console
    echo $remaining_deps
}

ipkg_safe_pkg_name() {
	local pkg=$1
	local spkg="`echo pkg_$pkg | sed -e y/-+./___/`"
	echo $spkg
}

ipkg_set_depends() {
	local pkg=$1; shift 
	local new_deps="$*"
	pkg="`ipkg_safe_pkg_name $pkg`"
	## setvar ${pkg}_depends "$new_deps"
	echo $new_deps > /tmp/ipkg/${pkg}.depends
}

ipkg_get_depends() {
	local pkg=$1
	pkg="`ipkg_safe_pkg_name $pkg`"
	cat /tmp/ipkg/${pkg}.depends
	## eval "echo \$${pkg}_depends"
}

ipkg_set_installed() {
	local pkg=$1
	pkg="`ipkg_safe_pkg_name $pkg`"
	echo installed > /tmp/ipkg/${pkg}.installed
	## setvar ${pkg}_installed "installed"
}

ipkg_set_uninstalled() {
	local pkg=$1
	pkg="`ipkg_safe_pkg_name $pkg`"
	### echo ipkg_set_uninstalled $pkg > /dev/console
	echo uninstalled > /tmp/ipkg/${pkg}.installed
	## setvar ${pkg}_installed "uninstalled"
}

ipkg_get_installed() {
	local pkg=$1
	pkg="`ipkg_safe_pkg_name $pkg`"
	if [ -f /tmp/ipkg/${pkg}.installed ]; then
		cat /tmp/ipkg/${pkg}.installed
	fi
	## eval "echo \$${pkg}_installed"
}

ipkg_depends() {
	local new_pkgs="$*"
	local all_deps=
	local installed_pkgs="`ipkg_status_matching_all 'Status:.*[[:space:]]installed'`"
	for pkg in $installed_pkgs; do
	    ipkg_set_installed $pkg
	done
	while [ -n "$new_pkgs" ]; do
		all_deps="$all_deps $new_pkgs"
		local new_deps=
		for pkg in $new_pkgs; do
			if echo $pkg | grep -q '[^a-z0-9.+-]'; then
				echo "ipkg_depends: ERROR: Package name $pkg contains illegal characters (should be [a-z0-9.+-])" >&2
				return 1
			fi
			# TODO: Fix this. For now I am ignoring versions and alternations in dependencies.
			new_deps="$new_deps "`ipkg_info $pkg '\(Pre-\)\?Depends' | ipkg_extract_value | sed -e 's/([^)]*)//g
s/\(|[[:space:]]*[a-z0-9.+-]\+[[:space:]]*\)\+//g
s/,/ /g
s/ \+/ /g'`
			ipkg_set_depends $pkg $new_deps
		done

		new_deps=`echo $new_deps | sed -e 's/[[:space:]]\+/\\
/g' | sort | uniq`

		local maybe_new_pkgs=
		for pkg in $new_deps; do
			if ! echo $installed_pkgs | grep -q "\<$pkg\>"; then
				maybe_new_pkgs="$maybe_new_pkgs $pkg"
			fi
		done

		new_pkgs=
		for pkg in $maybe_new_pkgs; do
			if ! echo $all_deps | grep -q "\<$pkg\>"; then
				if [ -z "`ipkg_info $pkg`" ]; then
					echo "ipkg_depends: Warning: $pkg mentioned in dependency but no package found in $IPKG_LISTS_DIR" >&2
					ipkg_set_installed $pkg
				else
					new_pkgs="$new_pkgs $pkg"
					ipkg_set_uninstalled $pkg
				fi
			else
				ipkg_set_uninstalled $pkg
			fi
		done
	done

	echo $all_deps
}

ipkg_get_install_dest() {
	local dest="$1"
	shift
	local sd=$dest/$IPKG_DIR_PREFIX
	local info_dir=$sd/info

        local requested_pkgs="$*"
	local pkgs="`ipkg_depends $*`"

	mkdir -p $info_dir
	for pkg in $pkgs; do
		if ! ipkg_status_mentioned_sd $sd $pkg; then
			echo "Package: $pkg
Status: install ok not-installed" | ipkg_status_update_sd $sd $pkg
		fi
	done
        ## mark the packages that we were directly requested to install as uninstalled
        for pkg in $requested_pkgs; do ipkg_set_uninstalled $pkg; done

	local new_pkgs=
	local pkgs_installed=0
	while [ -n "pkgs" ]; do
		curcheck=0
		## echo "pkgs to install: {$pkgs}" > /dev/console
		for pkg in $pkgs; do
			curcheck="`expr $curcheck + 1`"
			local is_installed="`ipkg_get_installed $pkg`"
			if [ "$is_installed" = "installed" ]; then
				echo "$pkg is installed" > /dev/console
				continue
			fi

			local remaining_deps="`ipkg_unsatisfied_dependences $pkg`"
			if [ -n "$remaining_deps" ]; then
				new_pkgs="$new_pkgs $pkg"
				### echo "Dependences not satisfied for $pkg: $remaining_deps"
				if [ $curcheck -ne `echo  $pkgs|wc -w` ]; then
			        	continue
				fi
			fi

			local filename=
			for src in `ipkg_src_names`; do
				if ipkg_require_list $src; then
					filename="`ipkg_extract_paragraph $pkg < $IPKG_LISTS_DIR/$src | ipkg_extract_field Filename | ipkg_extract_value`"
					[ -n "$filename" ] && break
				fi
			done

			if [ -z "$filename" ]; then
				echo "ipkg_get_install: ERROR: Cannot find package $pkg in $IPKG_LISTS_DIR"
				echo "ipkg_get_install:        Check the spelling and maybe run \`ipkg update'."
				ipkg_status_remove_sd $sd $pkg
				return 1;
			fi

			[ -e "$IPKG_TMP" ] || mkdir -p $IPKG_TMP

			echo ""
			local tmp_pkg_file="$IPKG_TMP/"`ipkg_file_part $filename`
			if ! ipkg_download `ipkg_src_byname $src`/$filename $tmp_pkg_file; then
				echo "ipkg_get_install: Perhaps you need to run \`ipkg update'?"
				return 1
			fi

			if ! ipkg_install_file_dest $dest $tmp_pkg_file; then
				echo "ipkg_get_install: ERROR: Failed to install $tmp_pkg_file"
				echo "ipkg_get_install: I'll leave it there for you to try a manual installation"
				return 1
			fi

			ipkg_set_installed $pkg
			pkgs_installed="`expr $pkgs_installed + 1`"
			rm $tmp_pkg_file
		done
		### echo "Installed $pkgs_installed package(s) this round"
		if [ $pkgs_installed -eq 0 ]; then
			if [ -z "$new_pkgs" ]; then
			    break
			fi
		fi
		pkgs_installed=0
		pkgs="$new_pkgs"
		new_pkgs=
		curcheck=0
        done
}

ipkg_get_install() {
	ipkg_get_install_dest $IPKG_ROOT $*
}

ipkg_install_file_dest() {
	local dest="$1"
	local filename="$2"
	local sd=$dest/$IPKG_DIR_PREFIX
	local info_dir=$sd/info

	if [ ! -f "$filename" ]; then
		echo "ipkg_install_file: ERROR: File $filename not found"
		return 1
	fi

	local pkg="`ipkg_file_part $filename | sed 's/\([a-z0-9.+-]\+\)_.*/\1/'`"
	local ext="`echo $filename | sed 's/.*\.//'`"
	local pkg_extract_stdout
	if [ "$ext" = "ipk" ]; then
		pkg_extract_stdout="tar -xzOf"
	elif [ "$ext" = "deb" ]; then
		pkg_extract_stdout="ar p"
	else
		echo "ipkg_install_file: ERROR: File $filename has unknown extension $ext (not .ipk or .deb)"
		return 1
	fi

	# Check dependencies
	local depends="`ipkg_depends $pkg | sed -e "s/\<$pkg\>//"`"

	# Don't worry about deps that are scheduled for installation
	local missing_deps=
	for dep in $depends; do
		if ! ipkg_status_all $dep | grep -q 'Status:[[:space:]]install'; then
			missing_deps="$missing_deps $dep"
		fi
	done

	if [ ! -z "$missing_deps" ]; then
		if [ -n "$FORCE_DEPENDS" ]; then
			echo "ipkg_install_file: Warning: $pkg depends on the following uninstalled programs: $missing_deps"
		else
			echo "ipkg_install_file: ERROR: $pkg depends on the following uninstalled programs:
	$missing_deps"
			echo "ipkg_install_file: You may want to use \`ipkg install' to install these."
			return 1
		fi
	fi

	mkdir -p $IPKG_TMP/$pkg/control
	mkdir -p $IPKG_TMP/$pkg/data
	mkdir -p $info_dir

	if ! $pkg_extract_stdout $filename ./control.tar.gz | (cd $IPKG_TMP/$pkg/control; tar -xzf - ) ; then
		echo "ipkg_install_file: ERROR unpacking control.tar.gz from $filename"
		return 1
	fi

	if [ -n "$IPKG_OFFLINE_ROOT" ]; then
		if grep -q '^InstallsOffline:[[:space:]]*no' $IPKG_TMP/$pkg/control/control; then
			echo "*** Warning: Package $pkg may not be installed in offline mode"
			echo "*** Warning: Scheduling $filename for pending installation (installing into $IPKG_PENDING_DIR)"
			echo "Package: $pkg
Status: install ok pending" | ipkg_status_update_sd $sd $pkg
			mkdir -p $IPKG_PENDING_DIR
			cp $filename $IPKG_PENDING_DIR
			rm -r $IPKG_TMP/$pkg/control
			rm -r $IPKG_TMP/$pkg/data
			rmdir $IPKG_TMP/$pkg
			return 0
		fi
	fi


	echo -n "Unpacking $pkg..."
	set +o noglob
	for file in $IPKG_TMP/$pkg/control/*; do
		local base_file="`ipkg_file_part $file`"
		mv $file $info_dir/$pkg.$base_file
	done
	set -o noglob
	rm -r $IPKG_TMP/$pkg/control

	if ! $pkg_extract_stdout $filename ./data.tar.gz | (cd $IPKG_TMP/$pkg/data; tar -xzf - ) ; then
		echo "ipkg_install_file: ERROR unpacking data.tar.gz from $filename"
		return 1
	fi
	echo "Done."

	echo -n "Configuring $pkg..."
	export PKG_ROOT=$dest
	if [ -x "$info_dir/$pkg.preinst" ]; then
		if ! $info_dir/$pkg.preinst install; then
			echo "$info_dir/$pkg.preinst failed. Aborting installation of $pkg"
			rm -rf $IPKG_TMP/$pkg/data
			rmdir $IPKG_TMP/$pkg
			return 1
		fi
	fi

	local old_conffiles="`ipkg_status_sd $sd $pkg Conffiles | ipkg_extract_value`"
	local new_conffiles=
	if [ -f "$info_dir/$pkg.conffiles" ]; then
		for conffile in `cat $info_dir/$pkg.conffiles`; do
			if [ -f "$dest/$conffile" ] && ! echo " $old_conffiles " | grep -q " $conffile "`md5sum $dest/$conffile | sed 's/ .*//'`; then
				local use_maintainers_conffile=
				if [ -z "$FORCE_DEFAULTS" ]; then
					while true; do
						echo -n "Configuration file \`$conffile'
 ==> File on system created by you or by a script.
 ==> File also in package provided by package maintainer.
   What would you like to do about it ?  Your options are:
    Y or I  : install the package maintainer's version
    N or O  : keep your currently-installed version
      D     : show the differences between the versions (if diff is installed)
 The default action is to keep your current version.
*** `ipkg_file_part $conffile` (Y/I/N/O/D) [default=N] ? "
						read response
						case "$response" in
						[YyIi] | [Yy][Ee][Ss])
							use_maintainers_conffile=t
							break
						;;
						[Dd])
							echo "
diff -u $dest/$conffile $IPKG_TMP/$pkg/data/$conffile"
							diff -u $dest/$conffile $IPKG_TMP/$pkg/data/$conffile || true
							echo "[Press ENTER to continue]"
							read junk
						;;
						*)
							break
						;;
						esac
					done
				fi
				if [ -n "$use_maintainers_conffile" ]; then
					local md5sum="`md5sum $IPKG_TMP/$pkg/data/$conffile | sed 's/ .*//'`"
					new_conffiles="$new_conffiles $conffile $md5sum"
				else
					new_conffiles="$new_conffiles $conffile <custom>"
					rm $IPKG_TMP/$pkg/data/$conffile
				fi
			else
				md5sum="`md5sum $IPKG_TMP/$pkg/data/$conffile | sed 's/ .*//'`"
				new_conffiles="$new_conffiles $conffile $md5sum"
			fi
		done
	fi

	local owd="`pwd`"
	(cd $IPKG_TMP/$pkg/data/; tar cf - . | (cd $owd; cd $dest; tar xf -))
	rm -rf $IPKG_TMP/$pkg/data
	rmdir $IPKG_TMP/$pkg
	$pkg_extract_stdout $filename ./data.tar.gz | tar tzf - | sed -e 's/^\.//' > $info_dir/$pkg.list

	if [ -x "$info_dir/$pkg.postinst" ]; then
		$info_dir/$pkg.postinst configure
	fi

	if [ -n "$new_conffiles" ]; then
		new_conffiles='Conffiles: '`echo $new_conffiles | ipkg_protect_slashes`
	fi
	local sed_safe_root="`echo $dest | sed -e "s/^${IPKG_OFFLINE_ROOT}//" | ipkg_protect_slashes`"
	sed -e "s/\(Package:.*\)/\1\\
Status: install ok installed\\
Root: ${sed_safe_root}\\
${new_conffiles}/" $info_dir/$pkg.control | ipkg_status_update_sd $sd $pkg

	rm -f $info_dir/$pkg.control
	rm -f $info_dir/$pkg.conffiles
	rm -f $info_dir/$pkg.preinst
	rm -f $info_dir/$pkg.postinst

	echo "Done."
}

ipkg_install_file() {
	ipkg_install_file_dest $IPKG_ROOT $*
}

ipkg_install() {

	while [ $# -gt 0 ]; do
		local pkg="$1"
		shift
	
		case "$pkg" in
		http://* | ftp://*)
			local tmp_pkg_file="$IPKG_TMP/"`ipkg_file_part $pkg`
			if ipkg_download $pkg $tmp_pkg_file; then
				ipkg_install_file $tmp_pkg_file
				rm $tmp_pkg_file
			fi
			;;
		file:/*.ipk  | file://*.deb)
				local ipkg_filename="`echo $pkg|sed 's/^file://'`"
				ipkg_install_file $ipkg_filename
			;;
		*.ipk  | *.deb)
			if [ -f "$pkg" ]; then
				ipkg_install_file $pkg
			else
				echo "File not found $pkg" >&2
			fi
			;;
		*)
			ipkg_get_install $pkg || true
			;;
		esac
	done
}

ipkg_install_pending() {
	[ -n "$IPKG_OFFLINE_ROOT" ] && return 0

	if [ -d "$IPKG_PENDING_DIR" ]; then
		set +o noglob
		local pending="`ls -1d $IPKG_PENDING_DIR/*.ipk 2> /dev/null`" || true
		set -o noglob
		if [ -n "$pending" ]; then
			echo "The following packages in $IPKG_PENDING_DIR will now be installed:"
			echo $pending
			for filename in $pending; do
				if ipkg_install_file $filename; then
					rm $filename
				fi
			done
		fi
	fi
	return 0
}

ipkg_install_wanted() {
	local wanted="`ipkg_status_matching 'Status:[[:space:]]*install.*not-installed'`"

	if [ -n "$wanted" ]; then
		echo "The following package were previously requested but have not been installed:"
		echo $wanted

		if [ -n "$FORCE_DEFAULTS" ]; then
			echo "Installing them now."
		else
			echo -n "Install them now [Y/n] ? "
			read response
			case "$response" in
			[Nn] | [Nn][Oo])
				return 0
				;;
			esac
		fi

		ipkg_install $wanted
	fi

	return 0
}

ipkg_upgrade_pkg() {
	local pkg="$1"
	local avail_ver="`ipkg_info $pkg Version | ipkg_extract_value | head -1`"

	is_installed=
	for dest_name in `ipkg_dest_names`; do
		local dest="`ipkg_dest_byname $dest_name`"
		local sd=$dest/$IPKG_DIR_PREFIX
		local inst_ver="`ipkg_status_sd $sd $pkg Version | ipkg_extract_value`"
		if [ -n "$inst_ver" ]; then
			is_installed=t

			if [ -z "$avail_ver" ]; then
				echo "Assuming locally installed package $pkg ($inst_ver) is up to date"
				return 0
			fi

			if [ "$avail_ver" = "$inst_ver" ]; then 
				echo "Package $pkg ($inst_ver) installed in $dest_name is up to date"
			elif ipkg_is_upgrade "$avail_ver" "$inst_ver"; then
				echo "Upgrading $pkg ($dest_name) from $inst_ver to $avail_ver"
				ipkg_get_install_dest $dest $pkg
			else
				echo "Not downgrading package $pkg from $inst_ver to $avail_ver"
			fi
		fi
	done

	if [ -z "$is_installed" ]; then
		echo "Package $pkg does not appear to be installed"
		return 0
	fi

}

ipkg_upgrade() {
	if [ $# -lt 1 ]; then
		local pkgs="`ipkg_status_matching 'Status:.*[[:space:]]installed'`"
	else
		pkgs="$*"
	fi
	
	for pkg in $pkgs; do
		ipkg_upgrade_pkg $pkg
	done
}

ipkg_remove_pkg_dest() {
	local dest="$1"
	local pkg="$2"
	local sd=$dest/$IPKG_DIR_PREFIX
	local info_dir=$sd/info

	if ! ipkg_status_installed_sd $sd $pkg; then
		echo "ipkg_remove: Package $pkg does not appear to be installed in $dest"
		if ipkg_status_mentioned_sd $sd $pkg; then
			echo "Purging mention of $pkg from the ipkg database"
			ipkg_status_remove_sd $sd $pkg
		fi
		return 1
	fi

	echo "ipkg_remove: Removing $pkg... "

	local files="`cat $info_dir/$pkg.list`"

	export PKG_ROOT=$dest
	if [ -x "$info_dir/$pkg.prerm" ]; then
		$info_dir/$pkg.prerm remove
	fi

	local conffiles="`ipkg_status_sd $sd $pkg Conffiles | ipkg_extract_value`"

	local dirs_to_remove=
	for file in $files; do
		if [ -d "$dest/$file" ]; then
			dirs_to_remove="$dirs_to_remove $dest/$file"
		else
			if echo " $conffiles " | grep -q " $file "; then
				if echo " $conffiles " | grep -q " $file "`md5sum $dest/$file | sed 's/ .*//'`; then
					rm -f $dest/$file
				fi
			else
				rm -f $dest/$file
			fi
		fi
	done

	local removed_a_dir=t
	while [ -n "$removed_a_dir" ]; do
		removed_a_dir=
		local new_dirs_to_remove=
		for dir in $dirs_to_remove; do
			if rmdir $dir >/dev/null 2>&1; then
				removed_a_dir=t
			else
				new_dirs_to_remove="$new_dirs_to_remove $dir"
			fi
		done
		dirs_to_remove="$new_dirs_to_remove"
	done

	if [ -n "$dirs_to_remove" ]; then
		echo "ipkg_remove: Warning: Not removing the following directories since they are not empty:" >&2
		echo "$dirs_to_remove" | sed -e 's/\/[/]\+/\//g' >&2
	fi

	if [ -x "$info_dir/$pkg.postrm" ]; then
		$info_dir/$pkg.postrm remove
	fi

	ipkg_status_remove_sd $sd $pkg
	set +o noglob
	rm -f $info_dir/$pkg.*
	set -o noglob

	echo "Done."
}

ipkg_remove_pkg() {
	local pkg="$1"
	for dest in `ipkg_dests_all`; do
		local sd=$dest/$IPKG_DIR_PREFIX
		if ipkg_status_mentioned_sd $sd $pkg; then
			ipkg_remove_pkg_dest $dest $pkg
		fi
	done
}

ipkg_remove() {
	while [ $# -gt 0 ]; do
		local pkg="$1"
		shift
		if [ -n "$DEST_NAME" ]; then
			ipkg_remove_pkg_dest $IPKG_ROOT $pkg
		else
			ipkg_remove_pkg $pkg
		fi
	done
}

###########
# ipkg main
###########

# Parse options
while [ $# -gt 0 ]; do
	arg="$1"
	case $arg in
	-d | -dest)
		[ $# -gt 1 ] || ipkg_usage "option $arg requires an argument"
		DEST_NAME="$2"
		shift
		;;
	-o | -offline)
		[ $# -gt 1 ] || ipkg_usage "option $arg requires an argument"
		IPKG_OFFLINE_ROOT="$2"
		shift
		;;
	-force-depends)
		FORCE_DEPENDS=t
		;;
	-force-defaults)
		FORCE_DEFAULTS=t
		;;
	-*)
		ipkg_usage "unknown option $arg"
		;;
	*)
		break
		;;
	esac
	shift
done

[ $# -lt 1 ] && ipkg_usage "ipkg must have one sub-command argument"
cmd="$1"
shift

ipkg_load_configuration
mkdir -p /tmp/ipkg && mkdir -p $IPKG_TMP

case "$cmd" in
update|upgrade|list|info|status|install_pending)
	;;
install|depends|remove|files|search)
	[ $# -lt 1 ] && ipkg_usage "ERROR: the \`\`$cmd'' command requires an argument"
	;;
*)
	echo "ERROR: unknown sub-command \`$cmd'"
	ipkg_usage
	;;
esac

# Only install pending if we have an interactive sub-command
case "$cmd" in
upgrade|install)
	ipkg_install_pending
	ipkg_install_wanted
	;;
esac

ipkg_$cmd $*
for a in `ls $IPKG_TMP`; do
	rm -rf $IPKG_TMP/$a
done
