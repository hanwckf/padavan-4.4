/* vi: set sw=4 ts=4: */
/*
 * Mini insmod implementation for busybox
 *
 * Copyright (C) 2008 Timo Teras <timo.teras@iki.fi>
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 */

//applet:IF_INSMOD(APPLET(insmod, BB_DIR_SBIN, BB_SUID_DROP))

#include "libbb.h"
#include "modutils.h"
#include <sys/utsname.h>
#include <fnmatch.h>

/* 2.6 style insmod has no options and required filename
 * (not module name - .ko can't be omitted) */

//usage:#if !ENABLE_MODPROBE_SMALL
//usage:#define insmod_trivial_usage
//usage:	IF_FEATURE_2_4_MODULES("[OPTIONS] MODULE ")
//usage:	IF_NOT_FEATURE_2_4_MODULES("FILE ")
//usage:	"[SYMBOL=VALUE]..."
//usage:#define insmod_full_usage "\n\n"
//usage:       "Load kernel module"
//usage:	IF_FEATURE_2_4_MODULES( "\n"
//usage:     "\n	-f	Force module to load into the wrong kernel version"
//usage:     "\n	-k	Make module autoclean-able"
//usage:     "\n	-v	Verbose"
//usage:     "\n	-q	Quiet"
//usage:     "\n	-L	Lock: prevent simultaneous loads"
//usage:	IF_FEATURE_INSMOD_LOAD_MAP(
//usage:     "\n	-m	Output load map to stdout"
//usage:	)
//usage:     "\n	-x	Don't export externs"
//usage:	)
//usage:#endif

int insmod_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int insmod_main(int argc UNUSED_PARAM, char **argv)
{
	struct utsname uts;
	struct stat st;
	char *filename;
	FILE *fp = NULL;
	int rc, pos;

	/* Compat note:
	 * 2.6 style insmod has no options and required filename
	 * (not module name - .ko can't be omitted).
	 * 2.4 style insmod can take module name without .o
	 * and performs module search in default directories
	 * or in $MODPATH.
	 */

	IF_FEATURE_2_4_MODULES(
		getopt32(argv, INSMOD_OPTS INSMOD_ARGS);
		argv += optind - 1;
	);

	filename = *++argv;
	if (!filename)
		bb_show_usage();

	/* Get a filedesc for the module.  Check we we have a complete path */
	if (stat(filename, &st) < 0 || !S_ISREG(st.st_mode) ||
		(fp = fopen(filename, "r")) == NULL) {
                /* Hmm.  Could not open it.  First search under /lib/modules/`uname -r` */
    		xchdir("/lib/modules");
	        uname(&uts);
    		xchdir(uts.release);

		pos = strlen(filename) - 2;
		if (get_linux_version_code() < KERNEL_VERSION(2,6,0)) {
			if (pos < 0) pos = 0;
			if (strncmp(&filename[pos], ".o", 2) !=0 )
				filename = xasprintf("%s.o", filename);
		} else {
			if (--pos < 0) pos = 0;
			if (strncmp(&filename[pos], ".ko", 3) !=0 )
				filename = xasprintf("%s.ko", filename);
		}
	}
	if (fp != NULL)
		fclose(fp);

	rc = bb_init_module(filename, parse_cmdline_module_options(argv, /*quote_spaces:*/ 0));
	if (rc)
		bb_error_msg("can't insert '%s': %s", filename, moderror(rc));

	return rc;
}
