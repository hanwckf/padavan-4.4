/*
 * token.h -- tokenize strings, a la strtok(3)
 *
 * Copyright (C) 2007 Oracle.  All rights reserved.
 * Copyright (C) 2007 Chuck Lever <chuck.lever@oracle.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 0211-1301 USA
 *
 */

#ifndef _NFS_UTILS_MOUNT_TOKEN_H
#define _NFS_UTILS_MOUNT_TOKEN_H

struct tokenizer_state;

char *next_token(struct tokenizer_state *);
struct tokenizer_state *init_tokenizer(char *, char);
int tokenizer_error(struct tokenizer_state *);
void end_tokenizer(struct tokenizer_state *);

#endif	/* _NFS_UTILS_MOUNT_TOKEN_H */
