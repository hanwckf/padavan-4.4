/*
 * config file parser helper
 *
 * Copyright (C) 2008 by Vladimir Dronnikov <dronnikov@gmail.com>
 *
 * Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
 * Also for use in uClibc (http://uclibc.org/) licensed under LGPLv2.1 or later.
 */

#if !defined _LIBC
#include "libbb.h"

#if defined ENABLE_PARSE && ENABLE_PARSE
int parse_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int parse_main(int argc UNUSED_PARAM, char **argv)
{
	const char *delims = "# \t";
	unsigned flags = PARSE_NORMAL;
	int mintokens = 0, ntokens = 128;

	opt_complementary = "-1:n+:m+:f+";
	getopt32(argv, "n:m:d:f:", &ntokens, &mintokens, &delims, &flags);
	//argc -= optind;
	argv += optind;
	while (*argv) {
		parser_t *p = config_open(*argv);
		if (p) {
			int n;
			char **t = xmalloc(sizeof(char *) * ntokens);
			while ((n = config_read(p, t, ntokens, mintokens, delims, flags)) != 0) {
				for (int i = 0; i < n; ++i)
					printf("[%s]", t[i]);
				puts("");
			}
			config_close(p);
		}
		argv++;
	}
	return EXIT_SUCCESS;
}
#endif
#else
# include <unistd.h>
# include <string.h>
# include <malloc.h>
# include <bits/uClibc_page.h>
# include "internal/parse_config.h"
# ifndef FAST_FUNC
#  define FAST_FUNC
# endif
# define fopen_or_warn_stdin fopen
# define bb_error_msg(...)
# define xstrdup strdup
# define xfunc_die() return 0
/* Read up to EOF or EOL, treat line-continuations as extending the line.
   Return number of bytes read into .line, -1 otherwise  */
static off_t bb_get_chunk_with_continuation(parser_t* parsr)
{
	off_t pos = 0;
	char *chp;

	while (1) {
		if (fgets(parsr->line + pos, parsr->line_len - pos, parsr->fp) == NULL) {
			memset(parsr->line, 0, parsr->line_len);
			pos = -1;
			break;
		}
		pos += strlen(parsr->line + pos);
		chp = strchr(parsr->line, '\n');
		if (chp) {
			--pos;
			if (--*chp == '\\')
				--pos;
			else
				break;
		} else if (parsr->allocated) {
			parsr->line_len += PAGE_SIZE;
			parsr->data = realloc(parsr->data,
								   parsr->data_len + parsr->line_len);
			parsr->line = parsr->data + parsr->data_len;
		} else {
			/* discard rest of line if not enough space in buffer */
			int c;
			do {
				c = fgetc(parsr->fp);
			} while (c != EOF && c != '\n');
			break;
		}
	}
	return pos;
}
#endif

/*

Typical usage:

----- CUT -----
	char *t[3];	// tokens placeholder
	parser_t *p = config_open(filename);
	if (p) {
		// parse line-by-line
		while (config_read(p, t, 3, 0, delimiters, flags)) { // 1..3 tokens
			// use tokens
			bb_error_msg("TOKENS: [%s][%s][%s]", t[0], t[1], t[2]);
		}
		...
		// free parser
		config_close(p);
	}
----- CUT -----

*/

static __always_inline parser_t * FAST_FUNC config_open2(const char *filename,
	FILE* FAST_FUNC (*fopen_func)(const char *path, const char *mode))
{
	parser_t *parser;
	FILE* fp;

	fp = fopen_func(filename, "r");
	if (!fp)
		return NULL;
	parser = calloc(1, sizeof(*parser));
	if (parser) {
		parser->fp = fp;
	}
	return parser;
}

parser_t * FAST_FUNC config_open(const char *filename)
{
	return config_open2(filename, fopen_or_warn_stdin);
}

#ifdef UNUSED
static void config_free_data(parser_t *parser)
{
	free(parser->data);
	parser->data = parser->line = NULL;
}
#endif

void FAST_FUNC config_close(parser_t *parser)
{
	if (parser) {
		fclose(parser->fp);
		if (parser->allocated)
			free(parser->data);
		free(parser);
	}
}

/*
0. If parser is NULL return 0.
1. Read a line from config file. If nothing to read then return 0.
   Handle continuation character. Advance lineno for each physical line.
   Discard everything past comment character.
2. if PARSE_TRIM is set (default), remove leading and trailing delimiters.
3. If resulting line is empty goto 1.
4. Look for first delimiter. If !PARSE_COLLAPSE or !PARSE_TRIM is set then
   remember the token as empty.
5. Else (default) if number of seen tokens is equal to max number of tokens
   (token is the last one) and PARSE_GREEDY is set then the remainder
   of the line is the last token.
   Else (token is not last or PARSE_GREEDY is not set) just replace
   first delimiter with '\0' thus delimiting the token.
6. Advance line pointer past the end of token. If number of seen tokens
   is less than required number of tokens then goto 4.
7. Check the number of seen tokens is not less the min number of tokens.
   Complain or die otherwise depending on PARSE_MIN_DIE.
8. Return the number of seen tokens.

mintokens > 0 make config_read() print error message if less than mintokens
(but more than 0) are found. Empty lines are always skipped (not warned about).
*/
#undef config_read
int FAST_FUNC config_read(parser_t *parser, char ***tokens,
											unsigned flags, const char *delims)
{
	char *line;
	int ntokens, mintokens;
	off_t len;
	int t;

	if (parser == NULL)
		return 0;
	ntokens = flags & 0xFF;
	mintokens = (flags & 0xFF00) >> 8;
again:
	if (parser->data == NULL) {
		if (parser->line_len == 0)
			parser->line_len = 81;
		if (parser->data_len == 0)
			parser->data_len += 1 + ntokens * sizeof(char *);
		parser->data = malloc(parser->data_len + parser->line_len);
		if (parser->data == NULL)
			return 0;
		parser->allocated |= 1;
	} /* else { assert(parser->data_len > 0); } */
	parser->line = parser->data + parser->data_len;
	/*config_free_data(parser);*/

	/* Read one line (handling continuations with backslash) */
	len = bb_get_chunk_with_continuation(parser);
	if (len == -1)
		return 0;
	line = parser->line;

	/* Skip multiple token-delimiters in the start of line? */
	if (flags & PARSE_TRIM)
		line += strspn(line, delims + 1);

	if (line[0] == '\0' || line[0] == delims[0])
		goto again;

	*tokens = (char **) parser->data;
	memset(*tokens, 0, sizeof(*tokens[0]) * ntokens);

	/* Tokenize the line */
	for (t = 0; *line && *line != delims[0] && t < ntokens; t++) {
		/* Pin token */
		*(*tokens + t) = line;

		/* Combine remaining arguments? */
		if ((t != ntokens-1) || !(flags & PARSE_GREEDY)) {
			/* Vanilla token, find next delimiter */
			line += strcspn(line, delims[0] ? delims : delims + 1);
		} else {
			/* Combining, find comment char if any */
			line = strchrnul(line, delims[0]);

			/* Trim any extra delimiters from the end */
			if (flags & PARSE_TRIM) {
				while (strchr(delims + 1, line[-1]) != NULL)
					line--;
			}
		}

		/* Token not terminated? */
		if (line[0] == delims[0])
			*line = '\0';
		else if (line[0] != '\0')
			*(line++) = '\0';

#if 0 /* unused so far */
		if (flags & PARSE_ESCAPE) {
			const char *from;
			char *to;

			from = to = tokens[t];
			while (*from) {
				if (*from == '\\') {
					from++;
					*to++ = bb_process_escape_sequence(&from);
				} else {
					*to++ = *from++;
				}
			}
			*to = '\0';
		}
#endif

		/* Skip possible delimiters */
		if (flags & PARSE_COLLAPSE)
			line += strspn(line, delims + 1);
	}

	if (t < mintokens) {
		bb_error_msg(/*"bad line %u: "*/"%d tokens found, %d needed",
				/*parser->lineno, */t, mintokens);
		if (flags & PARSE_MIN_DIE)
			xfunc_die();
		goto again;
	}
	return t;
}
