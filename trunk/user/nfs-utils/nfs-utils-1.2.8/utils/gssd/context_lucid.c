/*
 * COPYRIGHT (c) 2006
 * The Regents of the University of Michigan
 * ALL RIGHTS RESERVED
 *
 * Permission is granted to use, copy, create derivative works
 * and redistribute this software and such derivative works
 * for any purpose, so long as the name of The University of
 * Michigan is not used in any advertising or publicity
 * pertaining to the use of distribution of this software
 * without specific, written prior authorization.  If the
 * above copyright notice or any other identification of the
 * University of Michigan is included in any copy of any
 * portion of this software, then the disclaimer below must
 * also be included.
 *
 * THIS SOFTWARE IS PROVIDED AS IS, WITHOUT REPRESENTATION
 * FROM THE UNIVERSITY OF MICHIGAN AS TO ITS FITNESS FOR ANY
 * PURPOSE, AND WITHOUT WARRANTY BY THE UNIVERSITY OF
 * MICHIGAN OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING
 * WITHOUT LIMITATION THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE
 * REGENTS OF THE UNIVERSITY OF MICHIGAN SHALL NOT BE LIABLE
 * FOR ANY DAMAGES, INCLUDING SPECIAL, INDIRECT, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES, WITH RESPECT TO ANY CLAIM ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OF THE SOFTWARE, EVEN
 * IF IT HAS BEEN OR IS HEREAFTER ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGES.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif	/* HAVE_CONFIG_H */

#ifdef HAVE_LUCID_CONTEXT_SUPPORT

/*
 * Newer versions of MIT and Heimdal have lucid context support.
 * We can use common code if it is supported.
 */

#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>

#include <gssapi/gssapi_krb5.h>

#include "gss_util.h"
#include "gss_oids.h"
#include "err_util.h"
#include "context.h"

#ifndef OM_uint64
typedef uint64_t OM_uint64;
#endif

static int
write_lucid_keyblock(char **p, char *end, gss_krb5_lucid_key_t *key)
{
	gss_buffer_desc tmp;

	if (WRITE_BYTES(p, end, key->type)) return -1;
	tmp.length = key->length;
	tmp.value = key->data;
	if (write_buffer(p, end, &tmp)) return -1;
	return 0;
}

static int
prepare_krb5_rfc1964_buffer(gss_krb5_lucid_context_v1_t *lctx,
	gss_buffer_desc *buf, int32_t *endtime)
{
#define FAKESEED_SIZE 16
	char *p, *end;
	static int constant_zero = 0;
	unsigned char fakeseed[FAKESEED_SIZE];
	uint32_t word_send_seq;
	gss_krb5_lucid_key_t enc_key;
	uint32_t i;
	char *skd, *dkd;
	gss_buffer_desc fakeoid;
	int err;

	/*
	 * The new Kerberos interface to get the gss context
	 * does not include the seed or seed_init fields
	 * because we never really use them.  But for now,
	 * send down a fake buffer so we can use the same
	 * interface to the kernel.
	 */
	memset(&enc_key, 0, sizeof(enc_key));
	memset(&fakeoid, 0, sizeof(fakeoid));
	memset(fakeseed, 0, FAKESEED_SIZE);

	if (!(buf->value = calloc(1, MAX_CTX_LEN)))
		goto out_err;
	p = buf->value;
	end = buf->value + MAX_CTX_LEN;

	if (WRITE_BYTES(&p, end, lctx->initiate)) goto out_err;

	/* seed_init and seed not used by kernel anyway */
	if (WRITE_BYTES(&p, end, constant_zero)) goto out_err;
	if (write_bytes(&p, end, &fakeseed, FAKESEED_SIZE)) goto out_err;

	if (WRITE_BYTES(&p, end, lctx->rfc1964_kd.sign_alg)) goto out_err;
	if (WRITE_BYTES(&p, end, lctx->rfc1964_kd.seal_alg)) goto out_err;
	if (WRITE_BYTES(&p, end, lctx->endtime)) goto out_err;
	if (endtime)
		*endtime = lctx->endtime;
	word_send_seq = lctx->send_seq;	/* XXX send_seq is 64-bit */
	if (WRITE_BYTES(&p, end, word_send_seq)) goto out_err;
	if (write_oid(&p, end, &krb5oid)) goto out_err;

#ifdef HAVE_HEIMDAL
	/*
	 * The kernel gss code expects des-cbc-raw for all flavors of des.
	 * The keytype from MIT has this type, but Heimdal does not.
	 * Force the Heimdal keytype to 4 (des-cbc-raw).
	 * Note that the rfc1964 version only supports DES enctypes.
	 */
	if (lctx->rfc1964_kd.ctx_key.type != 4) {
		printerr(2, "%s: overriding heimdal keytype (%d => %d)\n",
			 __FUNCTION__, lctx->rfc1964_kd.ctx_key.type, 4);
		lctx->rfc1964_kd.ctx_key.type = 4;
	}
#endif
	printerr(2, "%s: serializing keys with enctype %d and length %d\n",
		 __FUNCTION__, lctx->rfc1964_kd.ctx_key.type,
		 lctx->rfc1964_kd.ctx_key.length);

	/* derive the encryption key and copy it into buffer */
	enc_key.type = lctx->rfc1964_kd.ctx_key.type;
	enc_key.length = lctx->rfc1964_kd.ctx_key.length;
	if ((enc_key.data = calloc(1, enc_key.length)) == NULL)
		goto out_err;
	skd = (char *) lctx->rfc1964_kd.ctx_key.data;
	dkd = (char *) enc_key.data;
	for (i = 0; i < enc_key.length; i++)
		dkd[i] = skd[i] ^ 0xf0;
	err = write_lucid_keyblock(&p, end, &enc_key);
	free(enc_key.data);
	if (err)
		goto out_err;

	if (write_lucid_keyblock(&p, end, &lctx->rfc1964_kd.ctx_key))
		goto out_err;

	buf->length = p - (char *)buf->value;
	return 0;
out_err:
	printerr(0, "ERROR: failed serializing krb5 context for kernel\n");
	if (buf->value) free(buf->value);
	buf->length = 0;
	return -1;
}

/* Flags for version 2 context flags */
#define KRB5_CTX_FLAG_INITIATOR		0x00000001
#define KRB5_CTX_FLAG_CFX		0x00000002
#define KRB5_CTX_FLAG_ACCEPTOR_SUBKEY	0x00000004

/*
 * Prepare a new-style buffer, as defined in rfc4121 (a.k.a. cfx),
 * to send to the kernel for newer encryption types -- or for DES3.
 *
 * The new format is:
 *
 *	u32 flags;
 *	#define KRB5_CTX_FLAG_INITIATOR		0x00000001
 *	#define KRB5_CTX_FLAG_CFX		0x00000002
 *	#define KRB5_CTX_FLAG_ACCEPTOR_SUBKEY	0x00000004
 *	s32 endtime;
 *	u64 seq_send;
 *	u32  enctype;			( encrption type of key )
 *	raw key;			( raw key bytes (kernel will derive))
 *
 */
static int
prepare_krb5_rfc4121_buffer(gss_krb5_lucid_context_v1_t *lctx,
	gss_buffer_desc *buf, int32_t *endtime)
{
	char *p, *end;
	uint32_t v2_flags = 0;
	uint32_t enctype;
	uint32_t keysize;

	if (!(buf->value = calloc(1, MAX_CTX_LEN)))
		goto out_err;
	p = buf->value;
	end = buf->value + MAX_CTX_LEN;

	/* Version 2 */
	if (lctx->initiate)
		v2_flags |= KRB5_CTX_FLAG_INITIATOR;
	if (lctx->protocol != 0)
		v2_flags |= KRB5_CTX_FLAG_CFX;
	if (lctx->protocol != 0 && lctx->cfx_kd.have_acceptor_subkey == 1)
		v2_flags |= KRB5_CTX_FLAG_ACCEPTOR_SUBKEY;

	if (WRITE_BYTES(&p, end, v2_flags)) goto out_err;
	if (WRITE_BYTES(&p, end, lctx->endtime)) goto out_err;
	if (endtime)
		*endtime = lctx->endtime;
	if (WRITE_BYTES(&p, end, lctx->send_seq)) goto out_err;

	/* Protocol 0 here implies DES3 or RC4 */
	printerr(2, "%s: protocol %d\n", __FUNCTION__, lctx->protocol);
	if (lctx->protocol == 0) {
		enctype = lctx->rfc1964_kd.ctx_key.type;
		keysize = lctx->rfc1964_kd.ctx_key.length;
	} else {
		if (lctx->cfx_kd.have_acceptor_subkey) {
			enctype = lctx->cfx_kd.acceptor_subkey.type;
			keysize = lctx->cfx_kd.acceptor_subkey.length;
		} else {
			enctype = lctx->cfx_kd.ctx_key.type;
			keysize = lctx->cfx_kd.ctx_key.length;
		}
	}
	printerr(2, "%s: serializing key with enctype %d and size %d\n",
		 __FUNCTION__, enctype, keysize);

	if (WRITE_BYTES(&p, end, enctype)) goto out_err;

	if (lctx->protocol == 0) {
		if (write_bytes(&p, end, lctx->rfc1964_kd.ctx_key.data,
				lctx->rfc1964_kd.ctx_key.length))
			goto out_err;
	} else {
		if (lctx->cfx_kd.have_acceptor_subkey) {
			if (write_bytes(&p, end,
					lctx->cfx_kd.acceptor_subkey.data,
					lctx->cfx_kd.acceptor_subkey.length))
				goto out_err;
		} else {
			if (write_bytes(&p, end, lctx->cfx_kd.ctx_key.data,
					lctx->cfx_kd.ctx_key.length))
				goto out_err;
		}
	}

	buf->length = p - (char *)buf->value;
	return 0;

out_err:
	printerr(0, "ERROR: %s: failed serializing krb5 context for kernel\n",
		 __FUNCTION__);
	if (buf->value) {
		free(buf->value);
		buf->value = NULL;
	}
	buf->length = 0;
	return -1;
}


int
serialize_krb5_ctx(gss_ctx_id_t *ctx, gss_buffer_desc *buf, int32_t *endtime)
{
	OM_uint32 maj_stat, min_stat;
	void *return_ctx = 0;
	OM_uint32 vers;
	gss_krb5_lucid_context_v1_t *lctx = 0;
	int retcode = 0;

	printerr(2, "DEBUG: %s: lucid version!\n", __FUNCTION__);
	maj_stat = gss_export_lucid_sec_context(&min_stat, ctx,
						1, &return_ctx);
	if (maj_stat != GSS_S_COMPLETE) {
		pgsserr("gss_export_lucid_sec_context",
			maj_stat, min_stat, &krb5oid);
		goto out_err;
	}

	/* Check the version returned, we only support v1 right now */
	vers = ((gss_krb5_lucid_context_version_t *)return_ctx)->version;
	switch (vers) {
	case 1:
		lctx = (gss_krb5_lucid_context_v1_t *) return_ctx;
		break;
	default:
		printerr(0, "ERROR: unsupported lucid sec context version %d\n",
			vers);
		goto out_err;
		break;
	}

	/*
	 * Now lctx points to a lucid context that we can send down to kernel
	 *
	 * Note: we send down different information to the kernel depending
	 * on the protocol version and the enctyption type.
	 * For protocol version 0 with all enctypes besides DES3, we use
	 * the original format.  For protocol version != 0 or DES3, we
	 * send down the new style information.
	 */

	if (lctx->protocol == 0 && lctx->rfc1964_kd.ctx_key.type <= 4)
		retcode = prepare_krb5_rfc1964_buffer(lctx, buf, endtime);
	else
		retcode = prepare_krb5_rfc4121_buffer(lctx, buf, endtime);

	maj_stat = gss_free_lucid_sec_context(&min_stat, ctx, return_ctx);
	if (maj_stat != GSS_S_COMPLETE) {
		pgsserr("gss_free_lucid_sec_context",
			maj_stat, min_stat, &krb5oid);
		printerr(0, "WARN: failed to free lucid sec context\n");
	}

	if (retcode) {
		printerr(1, "%s: prepare_krb5_*_buffer failed (retcode = %d)\n",
			 __FUNCTION__, retcode);
		goto out_err;
	}

	return 0;

out_err:
	printerr(0, "ERROR: failed serializing krb5 context for kernel\n");
	return -1;
}



#endif /* HAVE_LUCID_CONTEXT_SUPPORT */
