/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __NAMESPACE_H__
#define __NAMESPACE_H__ 1

#include <sched.h>
#include <sys/mount.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

#ifndef NETNS_RUN_DIR
#define NETNS_RUN_DIR "/var/run/netns"
#endif

#ifndef NETNS_ETC_DIR
#define NETNS_ETC_DIR "/etc/netns"
#endif

#ifndef CLONE_NEWNET
#define CLONE_NEWNET 0x40000000	/* New network namespace (lo, device, names sockets, etc) */
#endif

#ifndef MNT_DETACH
#define MNT_DETACH	0x00000002	/* Just detach from the tree */
#endif /* MNT_DETACH */

/* sys/mount.h may be out too old to have these */
#ifndef MS_REC
#define MS_REC		16384
#endif

#ifndef MS_SLAVE
#define MS_SLAVE	(1 << 19)
#endif

#ifndef MS_SHARED
#define MS_SHARED	(1 << 20)
#endif

#ifndef HAVE_SETNS
static inline int setns(int fd, int nstype)
{
#ifdef __NR_setns
	return syscall(__NR_setns, fd, nstype);
#else
	errno = ENOSYS;
	return -1;
#endif
}
#endif /* HAVE_SETNS */

int netns_switch(char *netns);
int netns_get_fd(const char *netns);
int netns_foreach(int (*func)(char *nsname, void *arg), void *arg);

struct netns_func {
	int (*func)(char *nsname, void *arg);
	void *arg;
};

#endif /* __NAMESPACE_H__ */
