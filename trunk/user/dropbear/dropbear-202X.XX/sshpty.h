/*	$OpenBSD: sshpty.h,v 1.4 2002/03/04 17:27:39 stevesk Exp $	*/

/*
 * Copied from openssh-3.5p1 source by Matt Johnston 2003
 *
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * Functions for allocating a pseudo-terminal and making it the controlling
 * tty.
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 */

#ifndef SSHPTY_H
#define SSHPTY_H

int	 pty_allocate(int *, int *, char *, int);
void	 pty_release(const char *);
void	 pty_make_controlling_tty(int *, const char *);
void	 pty_change_window_size(int, int, int, int, int);
void	 pty_setowner(struct passwd *, const char *);

#endif /* SSHPTY_H */
