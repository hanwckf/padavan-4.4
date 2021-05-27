/*
  svc_in_gssd_proc.c

  Copyright (c) 2000 The Regents of the University of Michigan.
  All rights reserved.

  Copyright (c) 2002 Bruce Fields <bfields@UMICH.EDU>

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif	/* HAVE_CONFIG_H */

#include <sys/param.h>
#include <sys/stat.h>
#include <rpc/rpc.h>

#include <pwd.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <nfsidmap.h>
#include <nfslib.h>
#include <time.h>

#include "svcgssd.h"
#include "gss_util.h"
#include "err_util.h"
#include "context.h"
#include "misc.h"
#include "gss_oids.h"
#include "svcgssd_krb5.h"

extern char * mech2file(gss_OID mech);
#define SVCGSSD_CONTEXT_CHANNEL "/proc/net/rpc/auth.rpcsec.context/channel"
#define SVCGSSD_INIT_CHANNEL    "/proc/net/rpc/auth.rpcsec.init/channel"

#define TOKEN_BUF_SIZE		8192

struct svc_cred {
	uid_t	cr_uid;
	gid_t	cr_gid;
	int	cr_ngroups;
	gid_t	cr_groups[NGROUPS];
};
static char vbuf[RPC_CHAN_BUF_SIZE];

static int
do_svc_downcall(gss_buffer_desc *out_handle, struct svc_cred *cred,
		gss_OID mech, gss_buffer_desc *context_token,
		int32_t endtime, char *client_name)
{
	FILE *f;
	int i;
	char *fname = NULL;
	int err;

	printerr(1, "doing downcall\n");
	if ((fname = mech2file(mech)) == NULL)
		goto out_err;
	f = fopen(SVCGSSD_CONTEXT_CHANNEL, "w");
	if (f == NULL) {
		printerr(0, "WARNING: unable to open downcall channel "
			     "%s: %s\n",
			     SVCGSSD_CONTEXT_CHANNEL, strerror(errno));
		goto out_err;
	}
	setvbuf(f, vbuf, _IOLBF, RPC_CHAN_BUF_SIZE);
	qword_printhex(f, out_handle->value, out_handle->length);
	/* XXX are types OK for the rest of this? */
	/* For context cache, use the actual context endtime */
	qword_printint(f, endtime);
	qword_printint(f, cred->cr_uid);
	qword_printint(f, cred->cr_gid);
	qword_printint(f, cred->cr_ngroups);
	printerr(2, "mech: %s, hndl len: %d, ctx len %d, timeout: %d (%d from now), "
		 "clnt: %s, uid: %d, gid: %d, num aux grps: %d:\n",
		 fname, out_handle->length, context_token->length,
		 endtime, endtime - time(0),
		 client_name ? client_name : "<null>",
		 cred->cr_uid, cred->cr_gid, cred->cr_ngroups);
	for (i=0; i < cred->cr_ngroups; i++) {
		qword_printint(f, cred->cr_groups[i]);
		printerr(2, "  (%4d) %d\n", i+1, cred->cr_groups[i]);
	}
	qword_print(f, fname);
	qword_printhex(f, context_token->value, context_token->length);
	if (client_name)
		qword_print(f, client_name);
	err = qword_eol(f);
	if (err) {
		printerr(1, "WARNING: error writing to downcall channel "
			 "%s: %s\n", SVCGSSD_CONTEXT_CHANNEL, strerror(errno));
	}
	fclose(f);
	return err;
out_err:
	printerr(1, "WARNING: downcall failed\n");
	return -1;
}

struct gss_verifier {
	u_int32_t	flav;
	gss_buffer_desc	body;
};

#define RPCSEC_GSS_SEQ_WIN	5

static int
send_response(gss_buffer_desc *in_handle, gss_buffer_desc *in_token,
	      u_int32_t maj_stat, u_int32_t min_stat,
	      gss_buffer_desc *out_handle, gss_buffer_desc *out_token)
{
	char buf[2 * TOKEN_BUF_SIZE];
	char *bp = buf;
	int blen = sizeof(buf);
	/* XXXARG: */
	int g;

	printerr(1, "sending null reply\n");

	qword_addhex(&bp, &blen, in_handle->value, in_handle->length);
	qword_addhex(&bp, &blen, in_token->value, in_token->length);
	/* For init cache, only needed for a short time */
	qword_addint(&bp, &blen, time(0) + 60);
	qword_adduint(&bp, &blen, maj_stat);
	qword_adduint(&bp, &blen, min_stat);
	qword_addhex(&bp, &blen, out_handle->value, out_handle->length);
	qword_addhex(&bp, &blen, out_token->value, out_token->length);
	qword_addeol(&bp, &blen);
	if (blen <= 0) {
		printerr(0, "WARNING: send_respsonse: message too long\n");
		return -1;
	}
	g = open(SVCGSSD_INIT_CHANNEL, O_WRONLY);
	if (g == -1) {
		printerr(0, "WARNING: open %s failed: %s\n",
				SVCGSSD_INIT_CHANNEL, strerror(errno));
		return -1;
	}
	*bp = '\0';
	printerr(3, "writing message: %s", buf);
	if (write(g, buf, bp - buf) == -1) {
		printerr(0, "WARNING: failed to write message\n");
		close(g);
		return -1;
	}
	close(g);
	return 0;
}

#define rpc_auth_ok			0
#define rpc_autherr_badcred		1
#define rpc_autherr_rejectedcred	2
#define rpc_autherr_badverf		3
#define rpc_autherr_rejectedverf	4
#define rpc_autherr_tooweak		5
#define rpcsec_gsserr_credproblem	13
#define rpcsec_gsserr_ctxproblem	14

static void
add_supplementary_groups(char *secname, char *name, struct svc_cred *cred)
{
	int ret;
	static gid_t *groups = NULL;

	cred->cr_ngroups = NGROUPS;
	ret = nfs4_gss_princ_to_grouplist(secname, name,
			cred->cr_groups, &cred->cr_ngroups);
	if (ret < 0) {
		groups = realloc(groups, cred->cr_ngroups*sizeof(gid_t));
		ret = nfs4_gss_princ_to_grouplist(secname, name,
				groups, &cred->cr_ngroups);
		if (ret < 0)
			cred->cr_ngroups = 0;
		else {
			if (cred->cr_ngroups > NGROUPS)
				cred->cr_ngroups = NGROUPS;
			memcpy(cred->cr_groups, groups,
					cred->cr_ngroups*sizeof(gid_t));
		}
	}
}

static int
get_ids(gss_name_t client_name, gss_OID mech, struct svc_cred *cred)
{
	u_int32_t	maj_stat, min_stat;
	gss_buffer_desc	name;
	char		*sname;
	int		res = -1;
	uid_t		uid, gid;
	gss_OID		name_type = GSS_C_NO_OID;
	char		*secname;

	maj_stat = gss_display_name(&min_stat, client_name, &name, &name_type);
	if (maj_stat != GSS_S_COMPLETE) {
		pgsserr("get_ids: gss_display_name",
			maj_stat, min_stat, mech);
		goto out;
	}
	if (name.length >= 0xffff || /* be certain name.length+1 doesn't overflow */
	    !(sname = calloc(name.length + 1, 1))) {
		printerr(0, "WARNING: get_ids: error allocating %d bytes "
			"for sname\n", name.length + 1);
		gss_release_buffer(&min_stat, &name);
		goto out;
	}
	memcpy(sname, name.value, name.length);
	printerr(1, "sname = %s\n", sname);
	gss_release_buffer(&min_stat, &name);

	res = -EINVAL;
	if ((secname = mech2file(mech)) == NULL) {
		printerr(0, "WARNING: get_ids: error mapping mech to "
			"file for name '%s'\n", sname);
		goto out_free;
	}

	res = nfs4_gss_princ_to_ids(secname, sname, &uid, &gid);
	if (res < 0) {
		/*
		 * -ENOENT means there was no mapping, any other error
		 * value means there was an error trying to do the
		 * mapping.
		 * If there was no mapping, we send down the value -1
		 * to indicate that the anonuid/anongid for the export
		 * should be used.
		 */
		if (res == -ENOENT) {
			cred->cr_uid = -1;
			cred->cr_gid = -1;
			cred->cr_ngroups = 0;
			res = 0;
			goto out_free;
		}
		printerr(1, "WARNING: get_ids: failed to map name '%s' "
			"to uid/gid: %s\n", sname, strerror(-res));
		goto out_free;
	}
	cred->cr_uid = uid;
	cred->cr_gid = gid;
	add_supplementary_groups(secname, sname, cred);
	res = 0;
out_free:
	free(sname);
out:
	return res;
}

#ifdef DEBUG
void
print_hexl(const char *description, unsigned char *cp, int length)
{
	int i, j, jm;
	unsigned char c;

	printf("%s (length %d)\n", description, length);

	for (i = 0; i < length; i += 0x10) {
		printf("  %04x: ", (u_int)i);
		jm = length - i;
		jm = jm > 16 ? 16 : jm;

		for (j = 0; j < jm; j++) {
			if ((j % 2) == 1)
				printf("%02x ", (u_int)cp[i+j]);
			else
				printf("%02x", (u_int)cp[i+j]);
		}
		for (; j < 16; j++) {
			if ((j % 2) == 1)
				printf("   ");
			else
				printf("  ");
		}
		printf(" ");

		for (j = 0; j < jm; j++) {
			c = cp[i+j];
			c = isprint(c) ? c : '.';
			printf("%c", c);
		}
		printf("\n");
	}
}
#endif

static int
get_krb5_hostbased_name (gss_buffer_desc *name, char **hostbased_name)
{
	char *p, *sname = NULL;
	if (strchr(name->value, '@') && strchr(name->value, '/')) {
		if ((sname = calloc(name->length, 1)) == NULL) {
			printerr(0, "ERROR: get_krb5_hostbased_name failed "
				 "to allocate %d bytes\n", name->length);
			return -1;
		}
		/* read in name and instance and replace '/' with '@' */
		sscanf(name->value, "%[^@]", sname);
		p = strrchr(sname, '/');
		if (p == NULL) {    /* The '@' preceeded the '/' */
			free(sname);
			return -1;
		}
		*p = '@';
	}
	*hostbased_name = sname;
	return 0;
}

static int
get_hostbased_client_name(gss_name_t client_name, gss_OID mech,
			  char **hostbased_name)
{
	u_int32_t	maj_stat, min_stat;
	gss_buffer_desc	name;
	gss_OID		name_type = GSS_C_NO_OID;
	char		*cname;
	int		res = -1;

	*hostbased_name = NULL;	    /* preset in case we fail */

	/* Get the client's gss authenticated name */
	maj_stat = gss_display_name(&min_stat, client_name, &name, &name_type);
	if (maj_stat != GSS_S_COMPLETE) {
		pgsserr("get_hostbased_client_name: gss_display_name",
			maj_stat, min_stat, mech);
		goto out_err;
	}
	if (name.length >= 0xffff) {	    /* don't overflow */
		printerr(0, "ERROR: get_hostbased_client_name: "
			 "received gss_name is too long (%d bytes)\n",
			 name.length);
		goto out_rel_buf;
	}

	/* For Kerberos, transform the NT_KRB5_PRINCIPAL name to
	 * an NT_HOSTBASED_SERVICE name */
	if (g_OID_equal(&krb5oid, mech)) {
		if (get_krb5_hostbased_name(&name, &cname) == 0)
			*hostbased_name = cname;
	} else {
		printerr(1, "WARNING: unknown/unsupport mech OID\n");
	}

	res = 0;
out_rel_buf:
	gss_release_buffer(&min_stat, &name);
out_err:
	return res;
}

void
handle_nullreq(FILE *f) {
	/* XXX initialize to a random integer to reduce chances of unnecessary
	 * invalidation of existing ctx's on restarting svcgssd. */
	static u_int32_t	handle_seq = 0;
	char			in_tok_buf[TOKEN_BUF_SIZE];
	char			in_handle_buf[15];
	char			out_handle_buf[15];
	gss_buffer_desc		in_tok = {.value = in_tok_buf},
				out_tok = {.value = NULL},
				in_handle = {.value = in_handle_buf},
				out_handle = {.value = out_handle_buf},
				ctx_token = {.value = NULL},
				ignore_out_tok = {.value = NULL},
	/* XXX isn't there a define for this?: */
				null_token = {.value = NULL};
	u_int32_t		ret_flags;
	gss_ctx_id_t		ctx = GSS_C_NO_CONTEXT;
	gss_name_t		client_name = NULL;
	gss_OID			mech = GSS_C_NO_OID;
	u_int32_t		maj_stat = GSS_S_FAILURE, min_stat = 0;
	u_int32_t		ignore_min_stat;
	struct svc_cred		cred;
	static char		*lbuf = NULL;
	static int		lbuflen = 0;
	static char		*cp;
	int32_t			ctx_endtime;
	char			*hostbased_name = NULL;

	printerr(1, "handling null request\n");

	if (readline(fileno(f), &lbuf, &lbuflen) != 1) {
		printerr(0, "WARNING: handle_nullreq: "
			    "failed reading request\n");
		return;
	}

	cp = lbuf;

	in_handle.length = (size_t) qword_get(&cp, in_handle.value,
					      sizeof(in_handle_buf));
#ifdef DEBUG
	print_hexl("in_handle", in_handle.value, in_handle.length);
#endif

	in_tok.length = (size_t) qword_get(&cp, in_tok.value,
					   sizeof(in_tok_buf));
#ifdef DEBUG
	print_hexl("in_tok", in_tok.value, in_tok.length);
#endif

	if (in_handle.length != 0) { /* CONTINUE_INIT case */
		if (in_handle.length != sizeof(ctx)) {
			printerr(0, "WARNING: handle_nullreq: "
				    "input handle has unexpected length %d\n",
				    in_handle.length);
			goto out_err;
		}
		/* in_handle is the context id stored in the out_handle
		 * for the GSS_S_CONTINUE_NEEDED case below.  */
		memcpy(&ctx, in_handle.value, in_handle.length);
	}

	if (svcgssd_limit_krb5_enctypes()) {
		goto out_err;
	}

	maj_stat = gss_accept_sec_context(&min_stat, &ctx, gssd_creds,
			&in_tok, GSS_C_NO_CHANNEL_BINDINGS, &client_name,
			&mech, &out_tok, &ret_flags, NULL, NULL);

	if (maj_stat == GSS_S_CONTINUE_NEEDED) {
		printerr(1, "gss_accept_sec_context GSS_S_CONTINUE_NEEDED\n");

		/* Save the context handle for future calls */
		out_handle.length = sizeof(ctx);
		memcpy(out_handle.value, &ctx, sizeof(ctx));
		goto continue_needed;
	}
	else if (maj_stat != GSS_S_COMPLETE) {
		printerr(1, "WARNING: gss_accept_sec_context failed\n");
		pgsserr("handle_nullreq: gss_accept_sec_context",
			maj_stat, min_stat, mech);
		goto out_err;
	}
	if (get_ids(client_name, mech, &cred)) {
		/* get_ids() prints error msg */
		maj_stat = GSS_S_BAD_NAME; /* XXX ? */
		goto out_err;
	}
	if (get_hostbased_client_name(client_name, mech, &hostbased_name)) {
		/* get_hostbased_client_name() prints error msg */
		maj_stat = GSS_S_BAD_NAME; /* XXX ? */
		goto out_err;
	}

	/* Context complete. Pass handle_seq in out_handle to use
	 * for context lookup in the kernel. */
	handle_seq++;
	out_handle.length = sizeof(handle_seq);
	memcpy(out_handle.value, &handle_seq, sizeof(handle_seq));

	/* kernel needs ctx to calculate verifier on null response, so
	 * must give it context before doing null call: */
	if (serialize_context_for_kernel(&ctx, &ctx_token, mech, &ctx_endtime)) {
		printerr(0, "WARNING: handle_nullreq: "
			    "serialize_context_for_kernel failed\n");
		maj_stat = GSS_S_FAILURE;
		goto out_err;
	}
	/* We no longer need the gss context */
	gss_delete_sec_context(&ignore_min_stat, &ctx, &ignore_out_tok);

	do_svc_downcall(&out_handle, &cred, mech, &ctx_token, ctx_endtime,
			hostbased_name);
continue_needed:
	send_response(&in_handle, &in_tok, maj_stat, min_stat,
			&out_handle, &out_tok);
out:
	if (ctx_token.value != NULL)
		free(ctx_token.value);
	if (out_tok.value != NULL)
		gss_release_buffer(&ignore_min_stat, &out_tok);
	if (client_name)
		gss_release_name(&ignore_min_stat, &client_name);
	free(hostbased_name);
	printerr(1, "finished handling null request\n");
	return;

out_err:
	if (ctx != GSS_C_NO_CONTEXT)
		gss_delete_sec_context(&ignore_min_stat, &ctx, &ignore_out_tok);
	send_response(&in_handle, &in_tok, maj_stat, min_stat,
			&null_token, &null_token);
	goto out;
}
