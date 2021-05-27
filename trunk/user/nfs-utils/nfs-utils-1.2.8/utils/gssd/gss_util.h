/*
  Copyright (c) 2004 The Regents of the University of Michigan.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
  3. Neither the name of the University nor the names of its
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _GSS_UTIL_H_
#define _GSS_UTIL_H_

#include <stdlib.h>
#include <rpc/rpc.h>
#include "write_bytes.h"

extern gss_cred_id_t	gssd_creds;

int gssd_acquire_cred(char *server_name, const gss_OID oid);
void pgsserr(char *msg, u_int32_t maj_stat, u_int32_t min_stat,
	const gss_OID mech);
int gssd_check_mechs(void);

#ifndef HAVE_LIBGSSGLUE
#include <gssapi/gssapi_krb5.h>
#define gss_free_lucid_sec_context(min, ctx, ret) \
		gss_krb5_free_lucid_sec_context(min, ret)

#define gss_export_lucid_sec_context gss_krb5_export_lucid_sec_context
#define gss_set_allowable_enctypes(min, cred, oid, num, types) \
		gss_krb5_set_allowable_enctypes(min, cred, num, types)
#endif

extern int avoid_dns;

#endif /* _GSS_UTIL_H_ */
