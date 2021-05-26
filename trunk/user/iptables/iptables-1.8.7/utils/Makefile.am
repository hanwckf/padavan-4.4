# -*- Makefile -*-

AM_CFLAGS = ${regular_CFLAGS}
AM_CPPFLAGS = ${regular_CPPFLAGS} -I${top_builddir}/include \
              -I${top_srcdir}/include ${libnfnetlink_CFLAGS}

sbin_PROGRAMS =
pkgdata_DATA =
man_MANS =

if HAVE_LIBNFNETLINK
man_MANS += nfnl_osf.8
sbin_PROGRAMS += nfnl_osf
pkgdata_DATA += pf.os

nfnl_osf_LDADD = ${libnfnetlink_LIBS}

uninstall-hook:
	dir=${DESTDIR}${pkgdatadir}; { \
		test ! -d "$$dir" && test ! -f "$$dir" && test ! -r "$$dir"; \
	} || rmdir -p --ignore-fail-on-non-empty "$$dir"
endif

if ENABLE_BPFC
man_MANS += nfbpf_compile.8
sbin_PROGRAMS += nfbpf_compile
nfbpf_compile_LDADD = -lpcap
endif

if ENABLE_SYNCONF
sbin_PROGRAMS += nfsynproxy
nfsynproxy_LDADD = -lpcap
endif

CLEANFILES = nfnl_osf.8 nfbpf_compile.8
