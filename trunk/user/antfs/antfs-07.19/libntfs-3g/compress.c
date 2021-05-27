/**
 * compress.c - Compressed attribute handling code.  Originated from the Linux-NTFS
 *		project.
 *
 * Copyright (c) 2004-2005 Anton Altaparmakov
 * Copyright (c) 2004-2006 Szabolcs Szakacsits
 * Copyright (c)      2005 Yura Pakhuchiy
 * Copyright (c) 2009-2014 Jean-Pierre Andre
 * Copyright (c)      2014 Eric Biggers
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "antfs.h"
#include "attrib.h"
#include "debug.h"
#include "volume.h"
#include "types.h"
#include "layout.h"
#include "runlist.h"
#include "compress.h"
#include "lcnalloc.h"
#include "misc.h"

/**
 * enum ntfs_compression_constants - constants used in the compression code
 */
enum ntfs_compression_constants {
	/* Token types and access mask. */
	NTFS_SYMBOL_TOKEN = 0,
	NTFS_PHRASE_TOKEN = 1,
	NTFS_TOKEN_MASK = 1,

	/* Compression sub-block constants. */
	NTFS_SB_SIZE_MASK = 0x0fff,
	NTFS_SB_SIZE = 0x1000,
	NTFS_SB_IS_COMPRESSED = 0x8000,
};

/* Match length at or above which ntfs_best_match() will stop searching for
 * longer matches.  */
#define NICE_MATCH_LEN 18

/* Maximum number of potential matches that ntfs_best_match() will consider at
 * each position.  */
#define MAX_SEARCH_DEPTH 24

/* log base 2 of the number of entries in the hash table for match-finding.  */
#define HASH_SHIFT 14

/* Constant for the multiplicative hash function.  */
#define HASH_MULTIPLIER 0x1E35A7BD

struct COMPRESS_CONTEXT {
	const unsigned char *inbuf;
	int bufsize;
	int size;
	int rel;
	int mxsz;
	s16 head[1 << HASH_SHIFT];
	s16 prev[NTFS_SB_SIZE];
};

/*
 *		Hash the next 3-byte sequence in the input buffer
 */
static inline unsigned int ntfs_hash(const u8 *p)
{
	u32 str;
	u32 hash;

#if defined(__i386__) || defined(__x86_64__)
	/* Unaligned access allowed, and little endian CPU.
	 * Callers ensure that at least 4 (not 3) bytes are remaining.  */
	str = *(const u32 *)p & 0xFFFFFF;
#else
	str = ((u32) p[0] << 0) | ((u32) p[1] << 8) | ((u32) p[2] << 16);
#endif

	hash = str * HASH_MULTIPLIER;

	/* High bits are more random than the low bits.  */
	return hash >> (32 - HASH_SHIFT);
}

/*
 *		Search for the longest sequence matching current position
 *
 *	A hash table, each entry of which points to a chain of sequence
 *	positions sharing the corresponding hash code, is maintained to speed up
 *	searching for matches.  To maintain the hash table, either
 *	ntfs_best_match() or ntfs_skip_position() has to be called for each
 *	consecutive position.
 *
 *	This function is heavily used; it has to be optimized carefully.
 *
 *	This function sets pctx->size and pctx->rel to the length and offset,
 *	respectively, of the longest match found.
 *
 *	The minimum match length is assumed to be 3, and the maximum match
 *	length is assumed to be pctx->mxsz.  If this function produces
 *	pctx->size < 3, then no match was found.
 *
 *	Note: for the following reasons, this function is not guaranteed to find
 *	*the* longest match up to pctx->mxsz:
 *
 *	(1) If this function finds a match of NICE_MATCH_LEN bytes or greater,
 *	    it ends early because a match this long is good enough and it's not
 *	    worth spending more time searching.
 *
 *	(2) If this function considers MAX_SEARCH_DEPTH matches with a single
 *	    position, it ends early and returns the longest match found so far.
 *	    This saves a lot of time on degenerate inputs.
 */
static void ntfs_best_match(struct COMPRESS_CONTEXT *pctx, const int i,
			    int best_len)
{
	const u8 *const inbuf = pctx->inbuf;
	const u8 *const strptr = &inbuf[i]; /* String we're matching against */
	s16 *const prev = pctx->prev;
	const int max_len = min(pctx->bufsize - i, pctx->mxsz);
	const int nice_len = min(NICE_MATCH_LEN, max_len);
	int depth_remaining = MAX_SEARCH_DEPTH;
	const u8 *best_matchptr = strptr;
	unsigned int hash;
	s16 cur_match;
	const u8 *matchptr;
	int len;

	if (max_len < 4)
		goto out;

	/* Insert the current sequence into the appropriate hash chain.  */
	hash = ntfs_hash(strptr);
	cur_match = pctx->head[hash];
	prev[i] = cur_match;
	pctx->head[hash] = i;

	if (best_len >= max_len) {
		/* Lazy match is being attempted, but there aren't enough length
		 * bits remaining to code a longer match.  */
		goto out;
	}

	/* Search the appropriate hash chain for matches.  */

	for (; cur_match >= 0 && depth_remaining--;
	     cur_match = prev[cur_match]) {

		matchptr = &inbuf[cur_match];

		/* Considering the potential match at 'matchptr':  is it longer
		 * than 'best_len'?
		 *
		 * The bytes at index 'best_len' are the most likely to differ,
		 * so check them first.
		 *
		 * The bytes at indices 'best_len - 1' and '0' are less
		 * important to check separately.  But doing so still gives a
		 * slight performance improvement, at least on x86_64, probably
		 * because they create separate branches for the CPU to predict
		 * independently of the branches in the main comparison loops.
		 */
		if (matchptr[best_len] != strptr[best_len] ||
		    matchptr[best_len - 1] != strptr[best_len - 1] ||
		    matchptr[0] != strptr[0])
			goto next_match;

		for (len = 1; len < best_len - 1; len++)
			if (matchptr[len] != strptr[len])
				goto next_match;

		/* The match is the longest found so far ---
		 * at least 'best_len' + 1 bytes.  Continue extending it.  */

		best_matchptr = matchptr;

		do {
			if (++best_len >= nice_len) {
				/* 'nice_len' reached; don't waste time
				 * searching for longer matches.  Extend the
				 * match as far as possible and terminate the
				 * search.  */
				while (best_len < max_len &&
				       (best_matchptr[best_len] ==
					strptr[best_len])) {
					best_len++;
				}
				goto out;
			}
		} while (best_matchptr[best_len] == strptr[best_len]);

		/* Found a longer match, but 'nice_len' not yet reached.  */

next_match:
		/* Continue to next match in the chain.  */
		;
	}

	/* Reached end of chain, or ended early due to reaching the maximum
	 * search depth.  */

out:
	/* Return the longest match we were able to find.  */
	pctx->size = best_len;
	pctx->rel = best_matchptr - strptr;/* given as a negative number! */
}

/*
 *		Advance the match-finder, but don't search for matches.
 */
static void ntfs_skip_position(struct COMPRESS_CONTEXT *pctx, const int i)
{
	unsigned int hash;

	if (pctx->bufsize - i < 4)
		return;

	/* Insert the current sequence into the appropriate hash chain.  */
	hash = ntfs_hash(pctx->inbuf + i);
	pctx->prev[i] = pctx->head[hash];
	pctx->head[hash] = i;
}

/*
 *		Compress a 4096-byte block
 *
 *	Returns a header of two bytes followed by the compressed data.
 *	If compression is not effective, the header and an uncompressed
 *	block is returned.
 *
 *	Note : two bytes may be output before output buffer overflow
 *	is detected, so a 4100-bytes output buffer must be reserved.
 *
 *	@return size of compressed block inclusing the header
 *	        (minimal size is 2, maximum size is 4098)
 *	@retval negative error code on failure
 */

static unsigned int ntfs_compress_block(const char *inbuf, const int bufsize,
					char *outbuf)
{
	struct COMPRESS_CONTEXT *pctx;
	int i;		/* current position */
	int j;		/* end of best match from current position */
	int k;		/* end of best match from next position */
	int offs;	/* offset to best match */
	int bp;		/* bits to store offset */
	int bp_cur;	/* saved bits to store offset at current position */
	int mxoff;	/* max match offset : 1 << bp */
	unsigned int xout;
	unsigned int q;	/* aggregated offset and size */
	int have_match;	/* do we have a match at the current position? */
	char *ptag;	/* location reserved for a tag */
	int tag;	/* current value of tag */
	int ntag;	/* count of bits still undefined in tag */

	pctx = ntfs_malloc(sizeof(struct COMPRESS_CONTEXT));
	if (!pctx)
		return -ENOMEM;

	/* All hash chains start as empty.  The special value '-1' indicates the
	 * end of each hash chain.  */
	memset(pctx->head, 0xFF, sizeof(pctx->head));

	pctx->inbuf = (const unsigned char *)inbuf;
	pctx->bufsize = bufsize;
	xout = 2;
	i = 0;
	bp = 4;
	mxoff = 1 << bp;
	pctx->mxsz = (1 << (16 - bp)) + 2;
	have_match = 0;
	tag = 0;
	ntag = 8;
	ptag = &outbuf[xout++];

	while ((i < bufsize) && (xout < (NTFS_SB_SIZE + 2))) {

		/* This implementation uses "lazy" parsing: it always chooses
		 * the longest match, unless the match at the next position is
		 * longer.  This is the same strategy used by the high
		 * compression modes of zlib.  */

		if (!have_match) {
			/* Find the longest match at the current position.  But
			 * first adjust the maximum match length if needed.
			 * (This loop might need to run more than one time in
			 * the case that we just output a long match.)  */
			while (mxoff < i) {
				bp++;
				mxoff <<= 1;
				pctx->mxsz = (pctx->mxsz + 2) >> 1;
			}
			ntfs_best_match(pctx, i, 2);
		}

		if (pctx->size >= 3) {

			/* Found a match at the current position.  */

			j = i + pctx->size;
			bp_cur = bp;
			offs = pctx->rel;

			if (pctx->size >= NICE_MATCH_LEN) {

				/* Choose long matches immediately.  */

				q = (~offs << (16 - bp_cur)) + (j - i - 3);
				outbuf[xout++] = q & 255;
				outbuf[xout++] = (q >> 8) & 255;
				tag |= (1 << (8 - ntag));

				if (j == bufsize) {
					/* Shortcut if the match extends to the
					 * end of the buffer.  */
					i = j;
					--ntag;
					break;
				}
				i += 1;
				do {
					ntfs_skip_position(pctx, i);
				} while (++i != j);
				have_match = 0;
			} else {
				/* Check for a longer match at the next
				 * position.  */

				/* Doesn't need to be while() since we just
				 * adjusted the maximum match length at the
				 * previous position.  */
				if (mxoff < i + 1) {
					bp++;
					mxoff <<= 1;
					pctx->mxsz = (pctx->mxsz + 2) >> 1;
				}
				ntfs_best_match(pctx, i + 1, pctx->size);
				k = i + 1 + pctx->size;

				if (k > (j + 1)) {
					/* Next match is longer.
					 * Output a literal.  */
					outbuf[xout++] = inbuf[i++];
					have_match = 1;
				} else {
					/* Next match isn't longer.
					 * Output the current match.  */
					q = (~offs << (16 - bp_cur)) +
					    (j - i - 3);
					outbuf[xout++] = q & 255;
					outbuf[xout++] = (q >> 8) & 255;
					tag |= (1 << (8 - ntag));

					/* The minimum match length is 3, and
					 * we've run two bytes through the
					 * matchfinder already.  So the minimum
					 * number of positions we need to skip
					 * is 1.  */
					i += 2;
					do {
						ntfs_skip_position(pctx, i);
					} while (++i != j);
					have_match = 0;
				}
			}
		} else {
			/* No match at current position.  Output a literal.  */
			outbuf[xout++] = inbuf[i++];
			have_match = 0;
		}

		/* Store the tag if fully used.  */
		if (!--ntag) {
			*ptag = tag;
			ntag = 8;
			ptag = &outbuf[xout++];
			tag = 0;
		}
	}

	/* Store the last tag if partially used.  */
	if (ntag == 8)
		xout--;
	else
		*ptag = tag;

	/* Determine whether to store the data compressed or uncompressed.  */

	if ((i >= bufsize) && (xout < (NTFS_SB_SIZE + 2))) {
		/* Compressed.  */
		outbuf[0] = (xout - 3) & 255;
		outbuf[1] = 0xb0 + (((xout - 3) >> 8) & 15);
	} else {
		/* Uncompressed.  */
		memcpy(&outbuf[2], inbuf, bufsize);
		if (bufsize < NTFS_SB_SIZE)
			memset(&outbuf[bufsize + 2], 0, NTFS_SB_SIZE - bufsize);
		outbuf[0] = 0xff;
		outbuf[1] = 0x3f;
		xout = NTFS_SB_SIZE + 2;
	}

	/* Free the compression context and return the total number of bytes
	 * written to 'outbuf'.  */
	ntfs_free(pctx);
	return xout;
}

/**
 * ntfs_decompress - decompress a compression block into an array of pages
 *
 * @param dest       buffer to which to write the decompressed data
 * @param dest_size  size of buffer @dest in bytes
 * @param cb_start   compression block to decompress
 * @param cb_size    size of compression block @cb_start in bytes
 *
 * This decompresses the compression block @cb_start into the destination
 * buffer @dest.
 *
 * @cb_start is a pointer to the compression block which needs decompressing
 * and @cb_size is the size of @cb_start in bytes (8-64kiB).
 *
 * @retval 0 on success
 * @retval -EOVERFLOW on error in the compressed stream
 */
static int ntfs_decompress(u8 *dest, const u32 dest_size,
			   u8 *const cb_start, const u32 cb_size)
{
	/*
	 * Pointers into the compressed data, i.e. the compression block (cb),
	 * and the therein contained sub-blocks (sb).
	 */
	u8 *cb_end = cb_start + cb_size;	/* End of cb. */
	u8 *cb = cb_start;	/* Current position in cb. */
	u8 *cb_sb_start = cb;	/* Beginning of the current sb in the cb. */
	u8 *cb_sb_end;		/* End of current sb / beginning of next sb. */
	/* Variables for uncompressed data / destination. */
	u8 *dest_end = dest + dest_size;	/* End of dest buffer. */
	u8 *dest_sb_start;	/* Start of current sub-block in dest. */
	u8 *dest_sb_end;	/* End of current sb in dest. */
	/* Variables for tag and token parsing. */
	u8 tag;			/* Current tag. */
	int token;		/* Loop counter for the eight tokens in tag. */

	antfs_log_enter("cb_size = 0x%x.", (unsigned)cb_size);
do_next_sb:
	antfs_log_debug("Beginning sub-block at offset = %d in the cb.",
			(int)(cb - cb_start));
	/*
	 * Have we reached the end of the compression block or the end of the
	 * decompressed data?  The latter can happen for example if the current
	 * position in the compression block is one byte before its end so the
	 * first two checks do not detect it.
	 */
	if (cb == cb_end || !le16_to_cpup((le16 *) cb) || dest == dest_end) {
		antfs_log_leave("Completed. Returning success (0).");
		return 0;
	}
	/* Setup offset for the current sub-block destination. */
	dest_sb_start = dest;
	dest_sb_end = dest + NTFS_SB_SIZE;
	/* Check that we are still within allowed boundaries. */
	if (dest_sb_end > dest_end)
		goto return_overflow;
	/* Does the minimum size of a compressed sb overflow valid range? */
	if (cb + 6 > cb_end)
		goto return_overflow;
	/* Setup the current sub-block source pointers and validate range. */
	cb_sb_start = cb;
	cb_sb_end =
	    cb_sb_start + (le16_to_cpup((le16 *) cb) & NTFS_SB_SIZE_MASK)
	    + 3;
	if (cb_sb_end > cb_end)
		goto return_overflow;
	/* Now, we are ready to process the current sub-block (sb). */
	if (!(le16_to_cpup((le16 *) cb) & NTFS_SB_IS_COMPRESSED)) {
		antfs_log_debug("Found uncompressed sub-block.");
		/* This sb is not compressed, just copy it into destination. */
		/* Advance source position to first data byte. */
		cb += 2;
		/* An uncompressed sb must be full size. */
		if (cb_sb_end - cb != NTFS_SB_SIZE)
			goto return_overflow;
		/* Copy the block and advance the source position. */
		memcpy(dest, cb, NTFS_SB_SIZE);
		cb += NTFS_SB_SIZE;
		/* Advance destination position to next sub-block. */
		dest += NTFS_SB_SIZE;
		goto do_next_sb;
	}
	antfs_log_debug("Found compressed sub-block.");
	/* This sb is compressed, decompress it into destination. */
	/* Forward to the first tag in the sub-block. */
	cb += 2;
do_next_tag:
	if (cb == cb_sb_end) {
		/* Check if the decompressed sub-block was not full-length. */
		if (dest < dest_sb_end) {
			int nr_bytes = dest_sb_end - dest;

			antfs_log_debug
			    ("Filling incomplete sub-block with zeroes.");
			/* Zero remainder and update destination position. */
			memset(dest, 0, nr_bytes);
			dest += nr_bytes;
		}
		/* We have finished the current sub-block. */
		goto do_next_sb;
	}
	/* Check we are still in range. */
	if (cb > cb_sb_end || dest > dest_sb_end)
		goto return_overflow;
	/* Get the next tag and advance to first token. */
	tag = *cb++;
	/* Parse the eight tokens described by the tag. */
	for (token = 0; token < 8; token++, tag >>= 1) {
		u16 lg, pt, length, max_non_overlap;
		register u16 i;
		u8 *dest_back_addr;

		/* Check if we are done / still in range. */
		if (cb >= cb_sb_end || dest > dest_sb_end)
			break;
		/* Determine token type and parse appropriately. */
		if ((tag & NTFS_TOKEN_MASK) == NTFS_SYMBOL_TOKEN) {
			/*
			 * We have a symbol token, copy the symbol across, and
			 * advance the source and destination positions.
			 */
			*dest++ = *cb++;
			/* Continue with the next token. */
			continue;
		}
		/*
		 * We have a phrase token. Make sure it is not the first tag in
		 * the sb as this is illegal and would confuse the code below.
		 */
		if (dest == dest_sb_start)
			goto return_overflow;
		/*
		 * Determine the number of bytes to go back (p) and the number
		 * of bytes to copy (l). We use an optimized algorithm in which
		 * we first calculate log2(current destination position in sb),
		 * which allows determination of l and p in O(1) rather than
		 * O(n). We just need an arch-optimized log2() function now.
		 */
		lg = 0;
		for (i = dest - dest_sb_start - 1; i >= 0x10; i >>= 1)
			lg++;
		/* Get the phrase token into i. */
		pt = le16_to_cpup((le16 *) cb);
		/*
		 * Calculate starting position of the byte sequence in
		 * the destination using the fact that p = (pt >> (12 - lg)) + 1
		 * and make sure we don't go too far back.
		 */
		dest_back_addr = dest - (pt >> (12 - lg)) - 1;
		if (dest_back_addr < dest_sb_start)
			goto return_overflow;
		/* Now calculate the length of the byte sequence. */
		length = (pt & (0xfff >> lg)) + 3;
		/* Verify destination is in range. */
		if (dest + length > dest_sb_end)
			goto return_overflow;
		/* The number of non-overlapping bytes. */
		max_non_overlap = dest - dest_back_addr;
		if (length <= max_non_overlap) {
			/* The byte sequence doesn't overlap, just copy it. */
			memcpy(dest, dest_back_addr, length);
			/* Advance destination pointer. */
			dest += length;
		} else {
			/*
			 * The byte sequence does overlap, copy non-overlapping
			 * part and then do a slow byte by byte copy for the
			 * overlapping part. Also, advance the destination
			 * pointer.
			 */
			memcpy(dest, dest_back_addr, max_non_overlap);
			dest += max_non_overlap;
			dest_back_addr += max_non_overlap;
			length -= max_non_overlap;
			while (length--)
				*dest++ = *dest_back_addr++;
		}
		/* Advance source position and continue with the next token. */
		cb += 2;
	}
	/* No tokens left in the current tag. Continue with the next tag. */
	goto do_next_tag;
return_overflow:
	antfs_log_error("Failed to decompress file");
	antfs_log_leave("err: -EINVAL");
	return -EOVERFLOW;
}

/**
 * ntfs_is_cb_compressed - internal function, do not use
 *
 * This is a very specialised function determining if a cb is compressed or
 * uncompressed.  It is assumed that checking for a sparse cb has already been
 * performed and that the cb is not sparse.  It makes all sorts of other
 * assumptions as well and hence it is not useful anywhere other than where it
 * is used at the moment.  Please, do not make this function available for use
 * outside of compress.c as it is bound to confuse people and not do what they
 * want.
 *
 * @retval @TRUE on errors so that the error will be detected later on in the
 * code.  Might be a bit confusing to debug but there really should never be
 * errors coming from here.
 */
static bool ntfs_is_cb_compressed(struct ntfs_attr *na,
				  struct runlist_element *rl, VCN cb_start_vcn,
				  int cb_clusters)
{
	/*
	 * The simplest case: the run starting at @cb_start_vcn contains
	 * @cb_clusters clusters which are all not sparse, thus the cb is not
	 * compressed.
	 */
restart:
	cb_clusters -= rl->length - (cb_start_vcn - rl->vcn);
	while (cb_clusters > 0) {
		/* Go to the next run. */
		rl++;
		/* Map the next runlist fragment if it is not mapped. */
		if (rl->lcn < LCN_HOLE || !rl->length) {
			cb_start_vcn = rl->vcn;
			rl = ntfs_attr_find_vcn(na, rl->vcn);
			if (IS_ERR(rl) || rl->lcn < LCN_HOLE || !rl->length)
				return TRUE;
			/*
			 * If the runs were merged need to deal with the
			 * resulting partial run so simply restart.
			 */
			if (rl->vcn < cb_start_vcn)
				goto restart;
		}
		/* If the current run is sparse, the cb is compressed. */
		if (rl->lcn == LCN_HOLE)
			return TRUE;
		/* If the whole cb is not sparse, it is not compressed. */
		if (rl->length >= cb_clusters)
			return FALSE;
		cb_clusters -= rl->length;
	};
	/* All cb_clusters were not sparse thus the cb is not compressed. */
	return FALSE;
}

/**
 * ntfs_compressed_attr_pread - read from a compressed attribute
 *
 * @param na     ntfs attribute to read from
 * @param pos    byte position in the attribute to begin reading from
 * @param count  number of bytes to read
 * @param b      output data buffer
 *
 * @note You probably want to be using attrib.c::ntfs_attr_pread() instead.
 *
 * This function will read @count bytes starting at offset @pos from the
 * compressed ntfs attribute @na into the data buffer @b.
 *
 * On success, return the number of successfully read bytes.  If this number
 * is lower than @count this means that the read reached end of file or that
 * an error was encountered during the read so that the read is partial.
 * 0 means end of file or nothing was read (also return 0 when @count is 0).
 *
 * On error and nothing has been read, return negative error code
 * the return code of ntfs_pread(), or -EINVAL in case of invalid
 * arguments.
 *
 * @retval >= 0 is number of bytes read
 * @retval negative error code on failure
 */
s64 ntfs_compressed_attr_pread(struct ntfs_attr *na, s64 pos, s64 count,
			       void *b)
{
	s64 br, to_read, ofs, total, total2;
	u64 cb_size_mask;
	VCN start_vcn, vcn, end_vcn;
	struct ntfs_volume *vol;
	struct runlist_element *rl;
	u8 *dest, *cb, *cb_pos, *cb_end;
	u32 cb_size;
	int err;
	enum ATTR_FLAGS data_flags;
	enum FILE_ATTR_FLAGS compression;
	unsigned int nr_cbs, cb_clusters;

	antfs_log_enter
	    ("inode %lld, attr 0x%x, pos 0x%llx, count 0x%llx.",
	     (unsigned long long)na->ni->mft_no, le32_to_cpu(na->type),
	     (long long)pos, (long long)count);
	data_flags = na->data_flags;
	compression = na->ni->flags & FILE_ATTR_COMPRESSED;
	if (IS_ERR_OR_NULL(na) || IS_ERR_OR_NULL(na->ni)
	    || IS_ERR_OR_NULL(na->ni->vol)
	    || IS_ERR_OR_NULL(b)
	    || ((data_flags & ATTR_COMPRESSION_MASK)
		!= ATTR_IS_COMPRESSED)
	    || pos < 0 || count < 0) {
		antfs_log_leave("err: -EINVAL");
		return -EINVAL;
	}
	/*
	 * Encrypted attributes are not supported.  We return access denied,
	 * which is what Windows NT4 does, too.
	 */
	if (NAttrEncrypted(na)) {
		antfs_log_leave("err: -EACCESS");
		return -EACCES;
	}
	if (!count) {
		antfs_log_leave("!count (0)");
		return 0;
	}
	/* Truncate reads beyond end of attribute. */
	if (pos + count > na->data_size) {
		if (pos >= na->data_size) {
			antfs_log_leave("Read past EOF");
			return 0;
		}
		count = na->data_size - pos;
	}
	/* If it is a resident attribute, simply use ntfs_attr_pread(). */
	if (!NAttrNonResident(na))
		return ntfs_attr_pread(na, pos, count, b);
	total = total2 = 0;
	/* Zero out reads beyond initialized size. */
	if (pos + count > na->initialized_size) {
		if (pos >= na->initialized_size) {
			memset(b, 0, count);
			return count;
		}
		total2 = pos + count - na->initialized_size;
		count -= total2;
		memset((u8 *) b + count, 0, total2);
	}
	vol = na->ni->vol;
	cb_size = na->compression_block_size;
	cb_size_mask = cb_size - 1UL;
	cb_clusters = na->compression_block_clusters;

	/* Need a temporary buffer for each loaded compression block. */
	cb = (u8 *) ntfs_malloc(cb_size);
	if (!cb)
		return -ENOMEM;

	/* Need a temporary buffer for each uncompressed block. */
	dest = (u8 *) ntfs_malloc(cb_size);
	if (!dest) {
		ntfs_free(cb);
		return -ENOMEM;
	}
	/*
	 * The first vcn in the first compression block (cb) which we need to
	 * decompress.
	 */
	start_vcn = (pos & ~cb_size_mask) >> vol->cluster_size_bits;
	/* Offset in the uncompressed cb at which to start reading data. */
	ofs = pos & cb_size_mask;
	/*
	 * The first vcn in the cb after the last cb which we need to
	 * decompress.
	 */
	end_vcn = ((pos + count + cb_size - 1) & ~cb_size_mask) >>
	    vol->cluster_size_bits;
	/* Number of compression blocks (cbs) in the wanted vcn range. */
	nr_cbs = (end_vcn - start_vcn) << vol->cluster_size_bits >>
	    na->compression_block_size_bits;
	cb_end = cb + cb_size;
do_next_cb:
	nr_cbs--;
	cb_pos = cb;
	vcn = start_vcn;
	start_vcn += cb_clusters;

	/* Check whether the compression block is sparse. */
	rl = ntfs_attr_find_vcn(na, vcn);
	if (IS_ERR(rl) || rl->lcn < LCN_HOLE) {
		ntfs_free(cb);
		ntfs_free(dest);
		if (total)
			return total;
		/* FIXME: Do we want EIO or the error code? (AIA) */
		return -EIO;
	}
	if (rl->lcn == LCN_HOLE) {
		/* Sparse cb, zero out destination range overlapping the cb. */
		antfs_log_debug("Found sparse compression block.");
		to_read = min(count, cb_size - ofs);
		memset(b, 0, to_read);
		ofs = 0;
		total += to_read;
		count -= to_read;
		b = (u8 *) b + to_read;
	} else if (!ntfs_is_cb_compressed(na, rl, vcn, cb_clusters)) {
		s64 tdata_size, tinitialized_size;
		/*
		 * Uncompressed cb, read it straight into the destination range
		 * overlapping the cb.
		 */
		antfs_log_debug("Found uncompressed compression block.");
		/*
		 * Read the uncompressed data into the destination buffer.
		 * NOTE: We cheat a little bit here by marking the attribute as
		 * not compressed in the ntfs_attr structure so that we can
		 * read the data by simply using ntfs_attr_pread().  (-8
		 * NOTE: we have to modify data_size and initialized_size
		 * temporarily as well...
		 */
		to_read = min(count, cb_size - ofs);
		ofs += vcn << vol->cluster_size_bits;
		NAttrClearCompressed(na);
		na->data_flags &= ~ATTR_COMPRESSION_MASK;
		tdata_size = na->data_size;
		tinitialized_size = na->initialized_size;
		na->data_size = na->initialized_size = na->allocated_size;
		do {
			br = ntfs_attr_pread(na, ofs, to_read, b);
			if (br <= 0) {
				if (!br) {
					antfs_log_error("Failed to read an"
							" uncompressed cluster,"
							" inode %lld offs 0x%llx",
							(long long)na->ni->
							mft_no, (long long)ofs);
					err = -EIO;
				} else
					err = (int)br;
				na->data_size = tdata_size;
				na->initialized_size = tinitialized_size;
				na->ni->flags |= compression;
				na->data_flags = data_flags;
				ntfs_free(cb);
				ntfs_free(dest);
				if (total)
					return total;
				return err;
			}
			total += br;
			count -= br;
			b = (u8 *) b + br;
			to_read -= br;
			ofs += br;
		} while (to_read > 0);
		na->data_size = tdata_size;
		na->initialized_size = tinitialized_size;
		na->ni->flags |= compression;
		na->data_flags = data_flags;
		ofs = 0;
	} else {
		s64 tdata_size, tinitialized_size;
		u32 decompsz;

		/*
		 * Compressed cb, decompress it into the temporary buffer, then
		 * copy the data to the destination range overlapping the cb.
		 */
		antfs_log_debug("Found compressed compression block.");
		/*
		 * Read the compressed data into the temporary buffer.
		 * NOTE: We cheat a little bit here by marking the attribute as
		 * not compressed in the ntfs_attr structure so that we can
		 * read the raw, compressed data by simply using
		 * ntfs_attr_pread().  (-8
		 * NOTE: We have to modify data_size and initialized_size
		 * temporarily as well...
		 */
		to_read = cb_size;
		NAttrClearCompressed(na);
		na->data_flags &= ~ATTR_COMPRESSION_MASK;
		tdata_size = na->data_size;
		tinitialized_size = na->initialized_size;
		na->data_size = na->initialized_size = na->allocated_size;
		do {
			br = ntfs_attr_pread(na,
					     (vcn << vol->cluster_size_bits) +
					     (cb_pos - cb), to_read, cb_pos);
			if (br <= 0) {
				if (!br) {
					antfs_log_error("Failed to read a"
						" compressed cluster, "
						" inode %lld offs 0x%llx",
						(long long)na->ni->mft_no,
						(long long)(vcn <<
						vol->cluster_size_bits));
					err = -EIO;
				} else
					err = br;
				na->data_size = tdata_size;
				na->initialized_size = tinitialized_size;
				na->ni->flags |= compression;
				na->data_flags = data_flags;
				ntfs_free(cb);
				ntfs_free(dest);
				if (total)
					return total;
				return err;
			}
			cb_pos += br;
			to_read -= br;
		} while (to_read > 0);
		na->data_size = tdata_size;
		na->initialized_size = tinitialized_size;
		na->ni->flags |= compression;
		na->data_flags = data_flags;
		/* Just a precaution. */
		if (cb_pos + 2 <= cb_end)
			*(u16 *) cb_pos = 0;
		antfs_log_debug("Successfully read the compression block.");
		/* Do not decompress beyond the requested block */
		to_read = min(count, cb_size - ofs);
		decompsz = ((ofs + to_read - 1) | (NTFS_SB_SIZE - 1)) + 1;
		err = ntfs_decompress(dest, decompsz, cb, cb_size);
		if (err < 0) {
			ntfs_free(cb);
			ntfs_free(dest);
			if (total)
				return total;
			return err;
		}
		memcpy(b, dest + ofs, to_read);
		total += to_read;
		count -= to_read;
		b = (u8 *) b + to_read;
		ofs = 0;
	}
	/* Do we have more work to do? */
	if (nr_cbs)
		goto do_next_cb;
	/* We no longer need the buffers. */
	ntfs_free(cb);
	ntfs_free(dest);
	/* Return number of bytes read. */
	antfs_log_leave("Read: %lld", (long long)(total + total2));
	return total + total2;
}

/*
 *		Read data from a set of clusters
 *
 *	Returns the amount of data read (no error codes)
 */
static u32 read_clusters(struct ntfs_volume *vol,
			 const struct runlist_element *rl, s64 offs,
			 u32 to_read, char *inbuf)
{
	u32 count;
	int xgot;
	u32 got;
	s64 xpos;
	bool first;
	char *xinbuf;
	const struct runlist_element *xrl;

	got = 0;
	xrl = rl;
	xinbuf = inbuf;
	first = TRUE;
	do {
		count = xrl->length << vol->cluster_size_bits;
		xpos = xrl->lcn << vol->cluster_size_bits;
		if (first) {
			count -= offs;
			xpos += offs;
		}
		if ((to_read - got) < count)
			count = to_read - got;
		xgot = ntfs_pread(vol->dev, xpos, count, xinbuf);
		if (xgot == (int)count) {
			got += count;
			xpos += count;
			xinbuf += count;
			xrl++;
		}
		first = FALSE;
	} while ((xgot == (int)count) && (got < to_read));
	return got;
}

/*
 *		Write data to a set of clusters
 *
 *	Returns the amount of data written (no error codes)
 */
static s32 write_clusters(struct ntfs_volume *vol,
			  const struct runlist_element *rl, s64 offs,
			  s32 to_write, const char *outbuf)
{
	s32 count;
	s32 put, xput;
	s64 xpos;
	bool first;
	const char *xoutbuf;
	const struct runlist_element *xrl;

	put = 0;
	xrl = rl;
	xoutbuf = outbuf;
	first = TRUE;
	do {
		count = xrl->length << vol->cluster_size_bits;
		xpos = xrl->lcn << vol->cluster_size_bits;
		if (first) {
			count -= offs;
			xpos += offs;
		}
		if ((to_write - put) < count)
			count = to_write - put;
		xput = ntfs_pwrite(vol->dev, xpos, count, xoutbuf);
		if (xput == count) {
			put += count;
			xpos += count;
			xoutbuf += count;
			xrl++;
		}
		first = FALSE;
	} while ((xput == count) && (put < to_write));
	return put;
}

/*
 *		Compress and write a set of blocks
 *
 * @return the size actually written (rounded to a full cluster)
 * @retval 0 if all zeroes (nothing is written)
 * @retval -EDOM if could not compress (nothing is written)
 * @retval other negative error code for an irrecoverable error
 */
static s32 ntfs_comp_set(struct ntfs_attr *na, struct runlist_element *rl,
			 s64 offs, u32 insz, const char *inbuf)
{
	struct ntfs_volume *vol;
	char *outbuf;
	char *pbuf;
	u32 compsz;
	s32 written;
	s32 rounded;
	unsigned int clsz;
	u32 p;
	unsigned int sz;
	unsigned int bsz;
	bool fail;
	bool allzeroes;
	/* a single compressed zero */
	static char onezero[] = { 0x01, 0xb0, 0x00, 0x00 };
	/* a couple of compressed zeroes */
	static char twozeroes[] = { 0x02, 0xb0, 0x00, 0x00, 0x00 };
	/* more compressed zeroes, to be followed by some count */
	static char morezeroes[] = { 0x03, 0xb0, 0x02, 0x00 };

	vol = na->ni->vol;
	written = -EDOM;	/* default return */
	clsz = 1 << vol->cluster_size_bits;
	/* may need 2 extra bytes per block and 2 more bytes */
	outbuf = (char *)ntfs_malloc(na->compression_block_size
				     +
				     2 * (na->compression_block_size /
					  NTFS_SB_SIZE)
				     + 2);
	if (!outbuf)
		return -ENOMEM;

	fail = FALSE;
	compsz = 0;
	allzeroes = TRUE;
	for (p = 0; (p < insz) && !fail; p += NTFS_SB_SIZE) {
		if ((p + NTFS_SB_SIZE) < insz)
			bsz = NTFS_SB_SIZE;
		else
			bsz = insz - p;
		pbuf = &outbuf[compsz];
		sz = ntfs_compress_block(&inbuf[p], bsz, pbuf);
		/* fail if all the clusters (or more) are needed */
		if (!sz || ((compsz + sz + clsz + 2)
			    > na->compression_block_size))
			fail = TRUE;
		else {
			if (allzeroes) {
				/* check whether this is all zeroes */
				switch (sz) {
				case 4:
					allzeroes = !memcmp(pbuf, onezero, 4);
					break;
				case 5:
					allzeroes = !memcmp(pbuf, twozeroes, 5);
					break;
				case 6:
					allzeroes =
					    !memcmp(pbuf, morezeroes, 4);
					break;
				default:
					allzeroes = FALSE;
					break;
				}
			}
			compsz += sz;
		}
	}
	if (!fail && !allzeroes) {
		/* add a couple of null bytes, space has been checked */
		outbuf[compsz++] = 0;
		outbuf[compsz++] = 0;
		/* write a full cluster, to avoid partial reading */
		rounded = ((compsz - 1) | (clsz - 1)) + 1;
		memset(&outbuf[compsz], 0, rounded - compsz);
		written = write_clusters(vol, rl, offs, rounded, outbuf);
		if (written != rounded) {
			/*
			 * TODO : previously written text has been
			 * spoilt, should return a specific error
			 */
			antfs_log_error("error writing compressed data");
			written = -EIO;
		}
	} else if (!fail)
		written = 0;
	ntfs_free(outbuf);

	return written;
}

/*
 *		Check the validity of a compressed runlist
 *	The check starts at the beginning of current run and ends
 *	at the end of runlist
 *
 *	@retval 0 if valid
 *	@retval negative error code on failure
 */
static int check_valid_compressed_run(struct ntfs_attr *na,
				      struct runlist_element *rl,
				      bool fullcheck, const char *text)
{
	struct runlist_element *xrl;
	const char *err_txt;
	int err = 0;

	xrl = rl;
	while (xrl->vcn & (na->compression_block_clusters - 1))
		xrl--;
	err_txt = (const char *)NULL;
	while (xrl->length) {
		if ((xrl->vcn + xrl->length) != xrl[1].vcn)
			err_txt = "Runs not adjacent";
		if (xrl->lcn == LCN_HOLE) {
			if ((xrl->vcn + xrl->length)
			    & (na->compression_block_clusters - 1)) {
				err_txt = "Invalid hole";
			}
			if (fullcheck && (xrl[1].lcn == LCN_HOLE))
				err_txt = "Adjacent holes";
		}
		if (err_txt) {
			antfs_log_error("%s at %s index %ld inode %lld",
					err_txt, text, (long)(xrl - na->rl),
					(long long)na->ni->mft_no);
			err = -EIO;
			err_txt = (const char *)NULL;
		}
		xrl++;
	}
	return err;
}

/*
 *		Free unneeded clusters after overwriting compressed data
 *
 *	This generally requires one or two empty slots at the end of runlist,
 *	but we do not want to reallocate the runlist here because
 *	there are many pointers to it.
 *	So the empty slots have to be reserved beforehand
 *
 *	Returns zero unless some error occurred
 *
 *         +======= start of block =====+
 *      0  |A     chunk may overflow    | <-- rl         usedcnt : A + B
 *         |A     on previous block     |                        then B
 *         |A                           |
 *         +-- end of allocated chunk --+                freelength : C
 *         |B                           |                      (incl overflow)
 *         +== end of compressed data ==+
 *         |C                           | <-- freerl     freecnt : C + D
 *         |C     chunk may overflow    |
 *         |C     on next block         |
 *         +-- end of allocated chunk --+
 *         |D                           |
 *         |D     chunk may overflow    |
 *     15  |D     on next block         |
 *         +======== end of block ======+
 *
 */

static int ntfs_compress_overwr_free(struct ntfs_attr *na,
				     struct runlist_element *rl, s32 usedcnt,
				     s32 freecnt, VCN *update_from)
{
	bool beginhole;
	bool mergeholes;
	s32 oldlength;
	s32 freelength;
	s64 freelcn;
	s64 freevcn;
	struct runlist_element *freerl;
	struct ntfs_volume *vol;
	s32 carry;
	int err = 0;

	vol = na->ni->vol;
	freelcn = rl->lcn + usedcnt;
	freevcn = rl->vcn + usedcnt;
	freelength = rl->length - usedcnt;
	beginhole = !usedcnt && !rl->vcn;
	/* can merge with hole before ? */
	mergeholes = !usedcnt && rl[0].vcn && (rl[-1].lcn == LCN_HOLE);
	/* truncate current run, carry to subsequent hole */
	carry = freelength;
	oldlength = rl->length;
	if (mergeholes) {
		/* merging with a hole before */
		freerl = rl;
	} else {
		rl->length -= freelength;	/* warning : can be zero */
		freerl = ++rl;
	}
	if (!mergeholes && (usedcnt || beginhole)) {
		s32 freed;
		struct runlist_element *frl;
		struct runlist_element *erl;
		int holes = 0;
		bool threeparts;

		/* free the unneeded clusters from initial run, then freerl */
		threeparts = (freelength > freecnt);
		freed = 0;
		frl = freerl;
		if (freelength) {
			err = ntfs_cluster_free_basic(vol, freelcn,
						      (threeparts ? freecnt :
						       freelength));
			if (!err)
				freed += (threeparts ? freecnt : freelength);
			if (!usedcnt) {
				holes++;
				freerl--;
				freerl->length += (threeparts
						   ? freecnt : freelength);
				if (freerl->vcn < *update_from)
					*update_from = freerl->vcn;
			}
		}
		while (!err && frl->length && (freed < freecnt)) {
			if (frl->length <= (freecnt - freed)) {
				err = ntfs_cluster_free_basic(vol, frl->lcn,
							      frl->length);
				if (!err) {
					freed += frl->length;
					frl->lcn = LCN_HOLE;
					frl->length += carry;
					carry = 0;
					holes++;
				}
			} else {
				err = ntfs_cluster_free_basic(vol, frl->lcn,
							      freecnt - freed);
				if (!err) {
					frl->lcn += freecnt - freed;
					frl->vcn += freecnt - freed;
					frl->length -= freecnt - freed;
					freed = freecnt;
				}
			}
			frl++;
		}
		na->compressed_size -= freed << vol->cluster_size_bits;
		switch (holes) {
		case 0:
			/* there are no hole, must insert one */
			/* space for hole has been prereserved */
			if (freerl->lcn == LCN_HOLE) {
				if (threeparts) {
					erl = freerl;
					while (erl->length)
						erl++;
					do {
						erl[2] = *erl;
					} while (erl-- != freerl);

					freerl[1].length = freelength - freecnt;
					freerl->length = freecnt;
					freerl[1].lcn = freelcn + freecnt;
					freerl[1].vcn = freevcn + freecnt;
					freerl[2].lcn = LCN_HOLE;
					freerl[2].vcn = freerl[1].vcn
					    + freerl[1].length;
					freerl->vcn = freevcn;
				} else {
					freerl->vcn = freevcn;
					freerl->length += freelength;
				}
			} else {
				erl = freerl;
				while (erl->length)
					erl++;
				if (threeparts) {
					do {
						erl[2] = *erl;
					} while (erl-- != freerl);
					freerl[1].lcn = freelcn + freecnt;
					freerl[1].vcn = freevcn + freecnt;
					freerl[1].length =
					    oldlength - usedcnt - freecnt;
				} else {
					do {
						erl[1] = *erl;
					} while (erl-- != freerl);
				}
				freerl->lcn = LCN_HOLE;
				freerl->vcn = freevcn;
				freerl->length = freecnt;
			}
			break;
		case 1:
			/* there is a single hole, may have to merge */
			freerl->vcn = freevcn;
			freerl->length = freecnt;
			if (freerl[1].lcn == LCN_HOLE) {
				freerl->length += freerl[1].length;
				erl = freerl;
				do {
					erl++;
					*erl = erl[1];
				} while (erl->length);
			}
			break;
		default:
			/* there were several holes, must merge them */
			freerl->lcn = LCN_HOLE;
			freerl->vcn = freevcn;
			freerl->length = freecnt;
			if (freerl[holes].lcn == LCN_HOLE) {
				freerl->length += freerl[holes].length;
				holes++;
			}
			erl = freerl;
			do {
				erl++;
				*erl = erl[holes - 1];
			} while (erl->length);
			break;
		}
	} else {
		s32 freed;
		struct runlist_element *frl;
		struct runlist_element *xrl;

		freed = 0;
		frl = freerl--;
		if (freerl->vcn < *update_from)
			*update_from = freerl->vcn;
		while (!err && frl->length && (freed < freecnt)) {
			if (frl->length <= (freecnt - freed)) {
				freerl->length += frl->length;
				freed += frl->length;
				err = ntfs_cluster_free_basic(vol, frl->lcn,
							      frl->length);
				frl++;
			} else {
				freerl->length += freecnt - freed;
				err = ntfs_cluster_free_basic(vol, frl->lcn,
							      freecnt - freed);
				frl->lcn += freecnt - freed;
				frl->vcn += freecnt - freed;
				frl->length -= freecnt - freed;
				freed = freecnt;
			}
		}
		/* remove unneded runlist entries */
		xrl = freerl;
		/* group with next run if also a hole */
		if (frl->length && (frl->lcn == LCN_HOLE)) {
			xrl->length += frl->length;
			frl++;
		}
		while (frl->length)
			*++xrl = *frl++;
		*++xrl = *frl;	/* terminator */
		na->compressed_size -= freed << vol->cluster_size_bits;
	}
	return err;
}

/*
 *		Free unneeded clusters after compression
 *
 *	This generally requires one or two empty slots at the end of runlist,
 *	but we do not want to reallocate the runlist here because
 *	there are many pointers to it.
 *	So the empty slots have to be reserved beforehand
 *
 *	Returns zero unless some error occurred
 */
static int ntfs_compress_free(struct ntfs_attr *na, struct runlist_element *rl,
			      s64 used, s64 reserved, bool appending,
			      VCN *update_from)
{
	s32 freecnt;
	s32 usedcnt;
	int err = -EIO;		/* default return */
	s64 freelcn;
	s64 freevcn;
	s32 freelength;
	bool mergeholes;
	bool beginhole;
	struct ntfs_volume *vol;
	struct runlist_element *freerl;

	vol = na->ni->vol;
	freecnt = (reserved - used) >> vol->cluster_size_bits;
	usedcnt = (reserved >> vol->cluster_size_bits) - freecnt;
	if (rl->vcn < *update_from)
		*update_from = rl->vcn;
	/* skip entries fully used, if any */
	while (rl->length && (rl->length < usedcnt)) {
		usedcnt -= rl->length;	/* must be > 0 */
		rl++;
	}
	if (rl->length) {
		/*
		 * Splitting the current allocation block requires
		 * an extra runlist element to create the hole.
		 * The required entry has been prereserved when
		 * mapping the runlist.
		 */
		/* get the free part in initial run */
		freelcn = rl->lcn + usedcnt;
		freevcn = rl->vcn + usedcnt;
		/* new count of allocated clusters */
		if (!((freevcn + freecnt)
		      & (na->compression_block_clusters - 1))) {
			if (!appending) {
				err = ntfs_compress_overwr_free(na, rl,
					usedcnt, freecnt, update_from);
			} else {
				freelength = rl->length - usedcnt;
				beginhole = !usedcnt && !rl->vcn;
				mergeholes = !usedcnt
				    && rl[0].vcn && (rl[-1].lcn == LCN_HOLE);
				if (mergeholes) {
					s32 carry;

				/* shorten the runs which have free space */
					carry = freecnt;
					freerl = rl;
					while (freerl->length < carry) {
						carry -= freerl->length;
						freerl++;
					}
					freerl->length = carry;
					freerl = rl;
				} else {
					rl->length = usedcnt;/* can be zero ? */
					freerl = ++rl;
				}
				if ((freelength > 0)
				    && !mergeholes && (usedcnt || beginhole)) {
					/*
					 * move the unused part to the end.
					 * Doing so, the vcn will be out of
					 * order. This does not harm, the vcn
					 * are meaningless now, and only the lcn
					 * are meaningful for freeing.
					 */
					/* locate current end */
					while (rl->length)
						rl++;
					/* new terminator relocated */
					rl[1].vcn = rl->vcn;
					rl[1].lcn = LCN_ENOENT;
					rl[1].length = 0;
					/* hole, currently allocated */
					rl->vcn = freevcn;
					rl->lcn = freelcn;
					rl->length = freelength;
				} else {
			/* why is this different from the begin hole case ? */
					if ((freelength > 0)
					    && !mergeholes && !usedcnt) {
						freerl--;
						freerl->length = freelength;
						if (freerl->vcn < *update_from)
							*update_from
							    = freerl->vcn;
					}
				}
				/* free the hole */
				err = ntfs_cluster_free_from_rl(vol, freerl);
				if (!err) {
					na->compressed_size -= freecnt
					    << vol->cluster_size_bits;
					if (mergeholes) {
						/* merge with adjacent hole */
						freerl--;
						freerl->length += freecnt;
					} else {
						if (beginhole)
							freerl--;
						/* mark hole as free */
						freerl->lcn = LCN_HOLE;
						freerl->vcn = freevcn;
						freerl->length = freecnt;
					}
					if (freerl->vcn < *update_from)
						*update_from = freerl->vcn;
					/* and set up the new end */
					freerl[1].lcn = LCN_ENOENT;
					freerl[1].vcn = freevcn + freecnt;
					freerl[1].length = 0;
				}
			}
		} else {
			antfs_log_error("Bad end of a compression block set");
			err = -EIO;
		}
	} else {
		antfs_log_error("No cluster to free after compression");
		err = -EIO;
	}
	NAttrSetRunlistDirty(na);
	return err;
}

/*
 *		Read existing data, decompress and append buffer
 *	Do nothing if something fails
 *
 * @retval 0 if ok
 * @retval -EIO on general failure
 * @retval -ENOMEM if out of mem
 */
static int ntfs_read_append(struct ntfs_attr *na,
			    const struct runlist_element *rl, s64 offs,
			    u32 compsz, s32 pos, bool appending, char *outbuf,
			    s64 to_write, const void *b)
{
	int err = -EIO;
	char *compbuf;
	u32 decompsz;
	u32 got;

	if (compsz == na->compression_block_size) {
		/* if the full block was requested, it was a hole */
		memset(outbuf, 0, compsz);
		memcpy(&outbuf[pos], b, to_write);
		err = 0;
	} else {
		compbuf = (char *)ntfs_malloc(compsz);
		if (!compbuf)
			return -ENOMEM;
		/* must align to full block for decompression */
		if (appending)
			decompsz = ((pos - 1) | (NTFS_SB_SIZE - 1)) + 1;
		else
			decompsz = na->compression_block_size;
		got = read_clusters(na->ni->vol, rl, offs, compsz, compbuf);
		if ((got == compsz)
		    && !ntfs_decompress((u8 *) outbuf, decompsz,
					(u8 *) compbuf, compsz)) {
			memcpy(&outbuf[pos], b, to_write);
			err = 0;
		}
		ntfs_free(compbuf);
	}
	return err;
}

/*
 *		Flush a full compression block
 *
 *	@return the size actually written (rounded to a full cluster)
 *	@retval 0 if could not compress (and written uncompressed)
 *	@retval negative error code on other error
 */
static s32 ntfs_flush(struct ntfs_attr *na, struct runlist_element *rl,
		      s64 offs, const char *outbuf, s32 count, bool compress,
		      bool appending, VCN *update_from)
{
	s32 rounded;
	s32 written;
	int clsz;

	if (compress) {
		written = ntfs_comp_set(na, rl, offs, count, outbuf);
		if (written == -EDOM)
			compress = FALSE;
		if ((written >= 0)
		    && ntfs_compress_free(na, rl, offs + written,
					  offs + na->compression_block_size,
					  appending, update_from))
			written = -EIO;
	} else
		written = 0;
	if (!compress) {
		clsz = 1 << na->ni->vol->cluster_size_bits;
		rounded = ((count - 1) | (clsz - 1)) + 1;
		written = write_clusters(na->ni->vol, rl,
					 offs, rounded, outbuf);
		if (written != rounded && written >= 0)
			written = -EIO;
	}
	return written;
}

/*
 *		Write some data to be compressed.
 *
 *	Compression only occurs when a few clusters (usually 16) are
 *	full. When this occurs an extra runlist slot may be needed, so
 *	it has to be reserved beforehand.
 *
 *	@return size of uncompressed data written,
 *		or negative error code.
 *
 *	When the returned size is less than requested, new clusters have
 *	to be allocated before the function is called again.
 */
s64 ntfs_compressed_pwrite(struct ntfs_attr *na, struct runlist_element *wrl,
			   s64 wpos, s64 offs, s64 to_write, s64 rounded,
			   const void *b, int compressed_part,
			   VCN *update_from)
{
	struct ntfs_volume *vol;
	struct runlist_element *brl;/* entry containing beginning of block */
	int compression_length;
	s64 written;
	s64 to_read;
	s64 to_flush;
	s64 roffs;
	s64 got;
	s64 start_vcn;
	s64 nextblock;
	s64 endwrite;
	u32 compsz;
	char *inbuf;
	char *outbuf;
	bool fail;
	bool done;
	bool compress;
	bool appending;
	int err;

	err =
	    check_valid_compressed_run(na, wrl, FALSE,
				       "begin compressed write");
	if (err)
		return err;
	if ((*update_from < 0)
	    || (compressed_part < 0)
	    || (compressed_part > (int)na->compression_block_clusters)) {
		antfs_log_error("Bad update vcn or compressed_part %d for "
				"compressed write", compressed_part);
		return -EIO;
	}
	/* make sure there are two unused entries in runlist */
	if (na->unused_runs < 2) {
		antfs_log_error("No unused runs for compressed write");
		return -EIO;
	}
	if (wrl->vcn < *update_from)
		*update_from = wrl->vcn;
	written = -EIO;		/* default return */
	vol = na->ni->vol;
	compression_length = na->compression_block_clusters;
	compress = FALSE;
	done = FALSE;
	/*
	 * Cannot accept writing beyond the current compression set
	 * because when compression occurs, clusters are freed
	 * and have to be reallocated.
	 * (cannot happen with standard fuse 4K buffers)
	 * Caller has to avoid this situation, or face consequences.
	 */
	nextblock = ((offs + (wrl->vcn << vol->cluster_size_bits))
		     | (na->compression_block_size - 1)) + 1;
	/* determine whether we are appending to file */
	endwrite = offs + to_write + (wrl->vcn << vol->cluster_size_bits);
	appending = endwrite >= na->initialized_size;
	if (endwrite >= nextblock) {
		/* it is time to compress */
		compress = TRUE;
		/* only process what we can */
		to_write = rounded = nextblock
		    - (offs + (wrl->vcn << vol->cluster_size_bits));
	}
	start_vcn = 0;
	fail = FALSE;
	brl = wrl;
	roffs = 0;
	/*
	 * If we are about to compress or we need to decompress
	 * existing data, we have to process a full set of blocks.
	 * So relocate the parameters to the beginning of allocation
	 * containing the first byte of the set of blocks.
	 */
	if (compress || compressed_part) {
		/* find the beginning of block */
		start_vcn = (wrl->vcn + (offs >> vol->cluster_size_bits))
		    & -compression_length;
		if (start_vcn < *update_from)
			*update_from = start_vcn;
		while (brl->vcn && (brl->vcn > start_vcn)) {
			/* jumping back a hole means big trouble */
			if (brl->lcn == (LCN) LCN_HOLE) {
				antfs_log_error
				    ("jump back over a hole when appending");
				fail = TRUE;
			}
			brl--;
			offs += brl->length << vol->cluster_size_bits;
		}
		roffs = (start_vcn - brl->vcn) << vol->cluster_size_bits;
	}
	if (compressed_part && !fail) {
		/*
		 * The set of compression blocks contains compressed data
		 * (we are reopening an existing file to append to it)
		 * Decompress the data and append
		 */
		compsz = (s32) compressed_part << vol->cluster_size_bits;
		outbuf = (char *)ntfs_malloc(na->compression_block_size);
		if (outbuf) {
			if (appending) {
				to_read = offs - roffs;
				to_flush = to_read + to_write;
			} else {
				to_read = na->data_size
				    - (brl->vcn << vol->cluster_size_bits);
				if (to_read > na->compression_block_size)
					to_read = na->compression_block_size;
				to_flush = to_read;
			}
			if (!ntfs_read_append(na, brl, roffs, compsz,
					      (s32) (offs - roffs), appending,
					      outbuf, to_write, b)) {
				written = ntfs_flush(na, brl, roffs,
						     outbuf, to_flush, compress,
						     appending, update_from);
				if (written >= 0) {
					written = to_write;
					done = TRUE;
				}
			}
			ntfs_free(outbuf);
		}
	} else {
		if (compress && !fail) {
			/*
			 * we are filling up a block, read the full set
			 * of blocks and compress it
			 */
			inbuf = (char *)ntfs_malloc(na->compression_block_size);
			if (inbuf) {
				to_read = offs - roffs;
				if (to_read)
					got = read_clusters(vol, brl, roffs,
							    to_read, inbuf);
				else
					got = 0;
				if (got == to_read) {
					memcpy(&inbuf[to_read], b, to_write);
					written = ntfs_comp_set(na, brl, roffs,
								to_read +
								to_write,
								inbuf);
					/*
					 * if compression was not successful,
					 * only write the part which was
					 * requested
					 */
					if ((written >= 0)
					    /* free the unused clusters */
					    && !ntfs_compress_free(na, brl,
					    written + roffs,
					    na->compression_block_size +
					    roffs, appending, update_from)) {
						done = TRUE;
						written = to_write;
					}
				}
				ntfs_free(inbuf);
			}
		}
		if (!done) {
			/*
			 * if the compression block is not full, or
			 * if compression failed for whatever reason,
			 * write uncompressed
			 */
			/* check we are not overflowing current allocation */
			if ((wpos + rounded)
			    > ((wrl->lcn + wrl->length)
			       << vol->cluster_size_bits)) {
				antfs_log_error
				    ("writing on unallocated clusters");
			} else {
				written = ntfs_pwrite(vol->dev, wpos,
						      rounded, b);
				if (written == rounded)
					written = to_write;
			}
		}
	}
	if ((written >= 0)
	    && check_valid_compressed_run(na, wrl, TRUE,
					  "end compressed write"))
		written = -EIO;

	return written;
}

/*
 *		Close a file written compressed.
 *
 *	This compresses the last partial compression block of the file.
 *	Two empty runlist slots have to be reserved beforehand.
 *
 *	@retval zero if closing is successful
 *	@retval negative error code on failure
 */

int ntfs_compressed_close(struct ntfs_attr *na, struct runlist_element *wrl,
			  s64 offs, VCN *update_from)
{
	struct ntfs_volume *vol;
	struct runlist_element *brl;/* entry containing beginning of block */
	int compression_length;
	s64 written;
	s64 to_read;
	s64 roffs;
	s64 got;
	s64 start_vcn;
	char *inbuf;
	bool fail;
	bool done;
	int err = 0;

	if (na->unused_runs < 2) {
		antfs_log_error("No unused runs for compressed close");
		return -EIO;
	}
	if (*update_from < 0) {
		antfs_log_error("Bad update vcn for compressed close");
		return -EIO;
	}
	if (wrl->vcn < *update_from)
		*update_from = wrl->vcn;
	vol = na->ni->vol;
	compression_length = na->compression_block_clusters;
	done = FALSE;
	/*
	 * There generally is an uncompressed block at end of file,
	 * read the full block and compress it
	 */
	inbuf = (char *)ntfs_malloc(na->compression_block_size);
	if (!inbuf)
		return -ENOMEM;

	start_vcn = (wrl->vcn + (offs >> vol->cluster_size_bits))
	    & -compression_length;
	if (start_vcn < *update_from)
		*update_from = start_vcn;
	to_read = offs + ((wrl->vcn - start_vcn)
			  << vol->cluster_size_bits);
	brl = wrl;
	fail = FALSE;
	while (brl->vcn && (brl->vcn > start_vcn)) {
		if (brl->lcn == (LCN) LCN_HOLE) {
			antfs_log_error("jump back over a hole when closing");
			fail = TRUE;
			err = -EIO;
		}
		brl--;
	}
	if (!fail) {
		/* roffs can be an offset from another uncomp block */
		roffs = (start_vcn - brl->vcn)
		    << vol->cluster_size_bits;
		if (to_read) {
			got = read_clusters(vol, brl, roffs, to_read, inbuf);
			if (got == to_read) {
				written = ntfs_comp_set(na, brl, roffs,
							to_read, inbuf);
				if ((written >= 0)
				    /* free the unused clusters */
				    && !ntfs_compress_free(na, brl,
					written + roffs,
					na->compression_block_size +
					roffs, TRUE, update_from)) {
					done = TRUE;
				} else
				/* if compression failed, leave uncompressed */
					if (written == -EDOM)
						done = TRUE;
			}
		} else
			done = TRUE;
	}
	ntfs_free(inbuf);
	if (done
	    && check_valid_compressed_run(na, wrl, TRUE,
					  "end compressed close"))
		done = FALSE;
	if (done)
		err = 0;
	else if (!err)
		err = -EIO;

	return err;
}
