/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __TC_EMATCH_H_
#define __TC_EMATCH_H_

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "utils.h"
#include "tc_util.h"

#define EMATCHKINDSIZ 16

struct bstr {
	char	*data;
	unsigned int	len;
	int		quoted;
	struct bstr	*next;
};

struct bstr *bstr_alloc(const char *text);

static inline struct bstr *bstr_new(char *data, unsigned int len)
{
	struct bstr *b = calloc(1, sizeof(*b));

	if (b == NULL)
		return NULL;

	b->data = data;
	b->len = len;

	return b;
}

static inline int bstrcmp(const struct bstr *b, const char *text)
{
	int len = strlen(text);
	int d = b->len - len;

	if (d == 0)
		return strncmp(b->data, text, len);

	return d;
}

static inline struct bstr *bstr_next(struct bstr *b)
{
	return b->next;
}

unsigned long bstrtoul(const struct bstr *b);

struct ematch {
	struct bstr	*args;
	int		index;
	int		inverted;
	int		relation;
	int		child_ref;
	struct ematch	*child;
	struct ematch	*next;
};

static inline struct ematch *new_ematch(struct bstr *args, int inverted)
{
	struct ematch *e = calloc(1, sizeof(*e));

	if (e == NULL)
		return NULL;

	e->args = args;
	e->inverted = inverted;

	return e;
}

void print_ematch_tree(const struct ematch *tree);

struct ematch_util {
	char			kind[EMATCHKINDSIZ];
	int			kind_num;
	int	(*parse_eopt)(struct nlmsghdr *, struct tcf_ematch_hdr *,
			      struct bstr *);
	int	(*parse_eopt_argv)(struct nlmsghdr *, struct tcf_ematch_hdr *,
				   int, char **);
	int	(*print_eopt)(FILE *, struct tcf_ematch_hdr *, void *, int);
	void	(*print_usage)(FILE *);
	struct ematch_util	*next;
};

static inline int parse_layer(const struct bstr *b)
{
	if (*((char *) b->data) == 'l')
		return TCF_LAYER_LINK;
	else if (*((char *) b->data) == 'n')
		return TCF_LAYER_NETWORK;
	else if (*((char *) b->data) == 't')
		return TCF_LAYER_TRANSPORT;
	else
		return INT_MAX;
}

__attribute__((format(printf, 5, 6)))
int em_parse_error(int err, struct bstr *args, struct bstr *carg,
		   struct ematch_util *, char *fmt, ...);
int print_ematch(FILE *, const struct rtattr *);
int parse_ematch(int *, char ***, int, struct nlmsghdr *);

#endif
