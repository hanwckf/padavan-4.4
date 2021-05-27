#ifndef _XTABLES_H
#define _XTABLES_H

/*
 * Changing any structs/functions may incur a needed change
 * in libxtables_vcurrent/vage too.
 */

#include <sys/socket.h> /* PF_* */
#include <sys/types.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/types.h>
#include <linux/netfilter.h>
#include <linux/netfilter/x_tables.h>

#ifndef IPPROTO_SCTP
#define IPPROTO_SCTP 132
#endif
#ifndef IPPROTO_DCCP
#define IPPROTO_DCCP 33
#endif
#ifndef IPPROTO_MH
#	define IPPROTO_MH 135
#endif
#ifndef IPPROTO_UDPLITE
#define IPPROTO_UDPLITE	136
#endif

#include <xtables-version.h>

struct in_addr;

/*
 * .size is here so that there is a somewhat reasonable check
 * against the chosen .type.
 */
#define XTOPT_POINTER(stype, member) \
	.ptroff = offsetof(stype, member), \
	.size = sizeof(((stype *)NULL)->member)
#define XTOPT_TABLEEND {.name = NULL}

/**
 * Select the format the input has to conform to, as well as the target type
 * (area pointed to with XTOPT_POINTER). Note that the storing is not always
 * uniform. @cb->val will be populated with as much as there is space, i.e.
 * exactly 2 items for ranges, but the target area can receive more values
 * (e.g. in case of ranges), or less values (e.g. %XTTYPE_HOSTMASK).
 *
 * %XTTYPE_NONE:	option takes no argument
 * %XTTYPE_UINT*:	standard integer
 * %XTTYPE_UINT*RC:	colon-separated range of standard integers
 * %XTTYPE_DOUBLE:	double-precision floating point number
 * %XTTYPE_STRING:	arbitrary string
 * %XTTYPE_TOSMASK:	8-bit TOS value with optional mask
 * %XTTYPE_MARKMASK32:	32-bit mark with optional mask
 * %XTTYPE_SYSLOGLEVEL:	syslog level by name or number
 * %XTTYPE_HOST:	one host or address (ptr: union nf_inet_addr)
 * %XTTYPE_HOSTMASK:	one host or address, with an optional prefix length
 * 			(ptr: union nf_inet_addr; only host portion is stored)
 * %XTTYPE_PROTOCOL:	protocol number/name from /etc/protocols (ptr: uint8_t)
 * %XTTYPE_PORT:	16-bit port name or number (supports %XTOPT_NBO)
 * %XTTYPE_PORTRC:	colon-separated port range (names acceptable),
 * 			(supports %XTOPT_NBO)
 * %XTTYPE_PLEN:	prefix length
 * %XTTYPE_PLENMASK:	prefix length (ptr: union nf_inet_addr)
 * %XTTYPE_ETHERMAC:	Ethernet MAC address in hex form
 */
enum xt_option_type {
	XTTYPE_NONE,
	XTTYPE_UINT8,
	XTTYPE_UINT16,
	XTTYPE_UINT32,
	XTTYPE_UINT64,
	XTTYPE_UINT8RC,
	XTTYPE_UINT16RC,
	XTTYPE_UINT32RC,
	XTTYPE_UINT64RC,
	XTTYPE_DOUBLE,
	XTTYPE_STRING,
	XTTYPE_TOSMASK,
	XTTYPE_MARKMASK32,
	XTTYPE_SYSLOGLEVEL,
	XTTYPE_HOST,
	XTTYPE_HOSTMASK,
	XTTYPE_PROTOCOL,
	XTTYPE_PORT,
	XTTYPE_PORTRC,
	XTTYPE_PLEN,
	XTTYPE_PLENMASK,
	XTTYPE_ETHERMAC,
};

/**
 * %XTOPT_INVERT:	option is invertible (usable with !)
 * %XTOPT_MAND:		option is mandatory
 * %XTOPT_MULTI:	option may be specified multiple times
 * %XTOPT_PUT:		store value into memory at @ptroff
 * %XTOPT_NBO:		store value in network-byte order
 * 			(only certain XTTYPEs recognize this)
 */
enum xt_option_flags {
	XTOPT_INVERT = 1 << 0,
	XTOPT_MAND   = 1 << 1,
	XTOPT_MULTI  = 1 << 2,
	XTOPT_PUT    = 1 << 3,
	XTOPT_NBO    = 1 << 4,
};

/**
 * @name:	name of option
 * @type:	type of input and validation method, see %XTTYPE_*
 * @id:		unique number (within extension) for option, 0-31
 * @excl:	bitmask of flags that cannot be used with this option
 * @also:	bitmask of flags that must be used with this option
 * @flags:	bitmask of option flags, see %XTOPT_*
 * @ptroff:	offset into private structure for member
 * @size:	size of the item pointed to by @ptroff; this is a safeguard
 * @min:	lowest allowed value (for singular integral types)
 * @max:	highest allowed value (for singular integral types)
 */
struct xt_option_entry {
	const char *name;
	enum xt_option_type type;
	unsigned int id, excl, also, flags;
	unsigned int ptroff;
	size_t size;
	unsigned int min, max;
};

/**
 * @arg:	input from command line
 * @ext_name:	name of extension currently being processed
 * @entry:	current option being processed
 * @data:	per-extension kernel data block
 * @xflags:	options of the extension that have been used
 * @invert:	whether option was used with !
 * @nvals:	number of results in uXX_multi
 * @val:	parsed result
 * @udata:	per-extension private scratch area
 * 		(cf. xtables_{match,target}->udata_size)
 */
struct xt_option_call {
	const char *arg, *ext_name;
	const struct xt_option_entry *entry;
	void *data;
	unsigned int xflags;
	bool invert;
	uint8_t nvals;
	union {
		uint8_t u8, u8_range[2], syslog_level, protocol;
		uint16_t u16, u16_range[2], port, port_range[2];
		uint32_t u32, u32_range[2];
		uint64_t u64, u64_range[2];
		double dbl;
		struct {
			union nf_inet_addr haddr, hmask;
			uint8_t hlen;
		};
		struct {
			uint8_t tos_value, tos_mask;
		};
		struct {
			uint32_t mark, mask;
		};
		uint8_t ethermac[6];
	} val;
	/* Wished for a world where the ones below were gone: */
	union {
		struct xt_entry_match **match;
		struct xt_entry_target **target;
	};
	void *xt_entry;
	void *udata;
};

/**
 * @ext_name:	name of extension currently being processed
 * @data:	per-extension (kernel) data block
 * @udata:	per-extension private scratch area
 * 		(cf. xtables_{match,target}->udata_size)
 * @xflags:	options of the extension that have been used
 */
struct xt_fcheck_call {
	const char *ext_name;
	void *data, *udata;
	unsigned int xflags;
};

/**
 * A "linear"/linked-list based name<->id map, for files similar to
 * /etc/iproute2/.
 */
struct xtables_lmap {
	char *name;
	int id;
	struct xtables_lmap *next;
};

enum xtables_ext_flags {
	XTABLES_EXT_ALIAS = 1 << 0,
};

struct xt_xlate;

struct xt_xlate_mt_params {
	const void			*ip;
	const struct xt_entry_match	*match;
	int				numeric;
	bool				escape_quotes;
};

struct xt_xlate_tg_params {
	const void			*ip;
	const struct xt_entry_target	*target;
	int				numeric;
	bool				escape_quotes;
};

/* Include file for additions: new matches and targets. */
struct xtables_match {
	/*
	 * ABI/API version this module requires. Must be first member,
	 * as the rest of this struct may be subject to ABI changes.
	 */
	const char *version;

	struct xtables_match *next;

	const char *name;
	const char *real_name;

	/* Revision of match (0 by default). */
	uint8_t revision;

	/* Extension flags */
	uint8_t ext_flags;

	uint16_t family;

	/* Size of match data. */
	size_t size;

	/* Size of match data relevant for userspace comparison purposes */
	size_t userspacesize;

	/* Function which prints out usage message. */
	void (*help)(void);

	/* Initialize the match. */
	void (*init)(struct xt_entry_match *m);

	/* Function which parses command options; returns true if it
           ate an option */
	/* entry is struct ipt_entry for example */
	int (*parse)(int c, char **argv, int invert, unsigned int *flags,
		     const void *entry,
		     struct xt_entry_match **match);

	/* Final check; exit if not ok. */
	void (*final_check)(unsigned int flags);

	/* Prints out the match iff non-NULL: put space at end */
	/* ip is struct ipt_ip * for example */
	void (*print)(const void *ip,
		      const struct xt_entry_match *match, int numeric);

	/* Saves the match info in parsable form to stdout. */
	/* ip is struct ipt_ip * for example */
	void (*save)(const void *ip, const struct xt_entry_match *match);

	/* Print match name or alias */
	const char *(*alias)(const struct xt_entry_match *match);

	/* Pointer to list of extra command-line options */
	const struct option *extra_opts;

	/* New parser */
	void (*x6_parse)(struct xt_option_call *);
	void (*x6_fcheck)(struct xt_fcheck_call *);
	const struct xt_option_entry *x6_options;

	/* Translate iptables to nft */
	int (*xlate)(struct xt_xlate *xl,
		     const struct xt_xlate_mt_params *params);

	/* Size of per-extension instance extra "global" scratch space */
	size_t udata_size;

	/* Ignore these men behind the curtain: */
	void *udata;
	unsigned int option_offset;
	struct xt_entry_match *m;
	unsigned int mflags;
	unsigned int loaded; /* simulate loading so options are merged properly */
};

struct xtables_target {
	/*
	 * ABI/API version this module requires. Must be first member,
	 * as the rest of this struct may be subject to ABI changes.
	 */
	const char *version;

	struct xtables_target *next;


	const char *name;

	/* Real target behind this, if any. */
	const char *real_name;

	/* Revision of target (0 by default). */
	uint8_t revision;

	/* Extension flags */
	uint8_t ext_flags;

	uint16_t family;


	/* Size of target data. */
	size_t size;

	/* Size of target data relevant for userspace comparison purposes */
	size_t userspacesize;

	/* Function which prints out usage message. */
	void (*help)(void);

	/* Initialize the target. */
	void (*init)(struct xt_entry_target *t);

	/* Function which parses command options; returns true if it
           ate an option */
	/* entry is struct ipt_entry for example */
	int (*parse)(int c, char **argv, int invert, unsigned int *flags,
		     const void *entry,
		     struct xt_entry_target **targetinfo);

	/* Final check; exit if not ok. */
	void (*final_check)(unsigned int flags);

	/* Prints out the target iff non-NULL: put space at end */
	void (*print)(const void *ip,
		      const struct xt_entry_target *target, int numeric);

	/* Saves the targinfo in parsable form to stdout. */
	void (*save)(const void *ip,
		     const struct xt_entry_target *target);

	/* Print target name or alias */
	const char *(*alias)(const struct xt_entry_target *target);

	/* Pointer to list of extra command-line options */
	const struct option *extra_opts;

	/* New parser */
	void (*x6_parse)(struct xt_option_call *);
	void (*x6_fcheck)(struct xt_fcheck_call *);
	const struct xt_option_entry *x6_options;

	/* Translate iptables to nft */
	int (*xlate)(struct xt_xlate *xl,
		     const struct xt_xlate_tg_params *params);

	size_t udata_size;

	/* Ignore these men behind the curtain: */
	void *udata;
	unsigned int option_offset;
	struct xt_entry_target *t;
	unsigned int tflags;
	unsigned int used;
	unsigned int loaded; /* simulate loading so options are merged properly */
};

struct xtables_rule_match {
	struct xtables_rule_match *next;
	struct xtables_match *match;
	/* Multiple matches of the same type: the ones before
	   the current one are completed from parsing point of view */
	bool completed;
};

/**
 * struct xtables_pprot -
 *
 * A few hardcoded protocols for 'all' and in case the user has no
 * /etc/protocols.
 */
struct xtables_pprot {
	const char *name;
	uint8_t num;
};

enum xtables_tryload {
	XTF_DONT_LOAD,
	XTF_DURING_LOAD,
	XTF_TRY_LOAD,
	XTF_LOAD_MUST_SUCCEED,
};

enum xtables_exittype {
	OTHER_PROBLEM = 1,
	PARAMETER_PROBLEM,
	VERSION_PROBLEM,
	RESOURCE_PROBLEM,
	XTF_ONLY_ONCE,
	XTF_NO_INVERT,
	XTF_BAD_VALUE,
	XTF_ONE_ACTION,
};

struct xtables_globals
{
	unsigned int option_offset;
	const char *program_name, *program_version;
	struct option *orig_opts;
	struct option *opts;
	void (*exit_err)(enum xtables_exittype status, const char *msg, ...) __attribute__((noreturn, format(printf,2,3)));
	int (*compat_rev)(const char *name, uint8_t rev, int opt);
};

#define XT_GETOPT_TABLEEND {.name = NULL, .has_arg = false}

/*
 * enum op-
 *
 * For writing clean nftables translations code
 */
enum xt_op {
	XT_OP_EQ,
	XT_OP_NEQ,
	XT_OP_MAX,
};

#ifdef __cplusplus
extern "C" {
#endif

extern const char *xtables_modprobe_program;
extern struct xtables_match *xtables_matches;
extern struct xtables_target *xtables_targets;

extern void xtables_init(void);
extern void xtables_fini(void);
extern void xtables_set_nfproto(uint8_t);
extern void *xtables_calloc(size_t, size_t);
extern void *xtables_malloc(size_t);
extern void *xtables_realloc(void *, size_t);

extern int xtables_insmod(const char *, const char *, bool);
extern int xtables_load_ko(const char *, bool);
extern int xtables_set_params(struct xtables_globals *xtp);
extern void xtables_free_opts(int reset_offset);
extern struct option *xtables_merge_options(struct option *origopts,
	struct option *oldopts, const struct option *newopts,
	unsigned int *option_offset);

extern int xtables_init_all(struct xtables_globals *xtp, uint8_t nfproto);
extern struct xtables_match *xtables_find_match(const char *name,
	enum xtables_tryload, struct xtables_rule_match **match);
extern struct xtables_match *xtables_find_match_revision(const char *name,
	enum xtables_tryload tryload, struct xtables_match *match,
	int revision);
extern struct xtables_target *xtables_find_target(const char *name,
	enum xtables_tryload);
struct xtables_target *xtables_find_target_revision(const char *name,
	enum xtables_tryload tryload, struct xtables_target *target,
	int revision);
extern int xtables_compatible_revision(const char *name, uint8_t revision,
				       int opt);

extern void xtables_rule_matches_free(struct xtables_rule_match **matches);

/* Your shared library should call one of these. */
extern void xtables_register_match(struct xtables_match *me);
extern void xtables_register_matches(struct xtables_match *, unsigned int);
extern void xtables_register_target(struct xtables_target *me);
extern void xtables_register_targets(struct xtables_target *, unsigned int);

extern bool xtables_strtoul(const char *, char **, uintmax_t *,
	uintmax_t, uintmax_t);
extern bool xtables_strtoui(const char *, char **, unsigned int *,
	unsigned int, unsigned int);
extern int xtables_service_to_port(const char *name, const char *proto);
extern uint16_t xtables_parse_port(const char *port, const char *proto);
extern void
xtables_parse_interface(const char *arg, char *vianame, unsigned char *mask);

/* this is a special 64bit data type that is 8-byte aligned */
#define aligned_u64 uint64_t __attribute__((aligned(8)))

extern struct xtables_globals *xt_params;
#define xtables_error (xt_params->exit_err)

extern void xtables_param_act(unsigned int, const char *, ...);

extern const char *xtables_ipaddr_to_numeric(const struct in_addr *);
extern const char *xtables_ipaddr_to_anyname(const struct in_addr *);
extern const char *xtables_ipmask_to_numeric(const struct in_addr *);
extern struct in_addr *xtables_numeric_to_ipaddr(const char *);
extern struct in_addr *xtables_numeric_to_ipmask(const char *);
extern int xtables_ipmask_to_cidr(const struct in_addr *);
extern void xtables_ipparse_any(const char *, struct in_addr **,
	struct in_addr *, unsigned int *);
extern void xtables_ipparse_multiple(const char *, struct in_addr **,
	struct in_addr **, unsigned int *);

extern struct in6_addr *xtables_numeric_to_ip6addr(const char *);
extern const char *xtables_ip6addr_to_numeric(const struct in6_addr *);
extern const char *xtables_ip6addr_to_anyname(const struct in6_addr *);
extern const char *xtables_ip6mask_to_numeric(const struct in6_addr *);
extern int xtables_ip6mask_to_cidr(const struct in6_addr *);
extern void xtables_ip6parse_any(const char *, struct in6_addr **,
	struct in6_addr *, unsigned int *);
extern void xtables_ip6parse_multiple(const char *, struct in6_addr **,
	struct in6_addr **, unsigned int *);

/* Absolute file name for network data base files.  */
#define XT_PATH_ETHERTYPES     "/etc/ethertypes"

struct xt_ethertypeent {
	char *e_name;           /* Official ethernet type name.  */
	char **e_aliases;       /* Alias list.  */
	int e_ethertype;        /* Ethernet type number.  */
};

extern struct xt_ethertypeent *xtables_getethertypebyname(const char *name);
extern struct xt_ethertypeent *xtables_getethertypebynumber(int ethertype);

/**
 * Print the specified value to standard output, quoting dangerous
 * characters if required.
 */
extern void xtables_save_string(const char *value);

#define FMT_NUMERIC		0x0001
#define FMT_NOCOUNTS		0x0002
#define FMT_KILOMEGAGIGA	0x0004
#define FMT_OPTIONS		0x0008
#define FMT_NOTABLE		0x0010
#define FMT_NOTARGET		0x0020
#define FMT_VIA			0x0040
#define FMT_NONEWLINE		0x0080
#define FMT_LINENUMBERS		0x0100
#define FMT_EBT_SAVE		0x0200
#define FMT_C_COUNTS		0x0400

#define FMT_PRINT_RULE (FMT_NOCOUNTS | FMT_OPTIONS | FMT_VIA \
                        | FMT_NUMERIC | FMT_NOTABLE)
#define FMT(tab,notab) ((format) & FMT_NOTABLE ? (notab) : (tab))

extern void xtables_print_num(uint64_t number, unsigned int format);
extern int xtables_parse_mac_and_mask(const char *from, void *to, void *mask);
extern int xtables_print_well_known_mac_and_mask(const void *mac,
						 const void *mask);
extern void xtables_print_mac(const unsigned char *macaddress);
extern void xtables_print_mac_and_mask(const unsigned char *mac,
				       const unsigned char *mask);

extern void xtables_parse_val_mask(struct xt_option_call *cb,
				   unsigned int *val, unsigned int *mask,
				   const struct xtables_lmap *lmap);

static inline void xtables_parse_mark_mask(struct xt_option_call *cb,
					   unsigned int *mark,
					   unsigned int *mask)
{
	xtables_parse_val_mask(cb, mark, mask, NULL);
}

extern void xtables_print_val_mask(unsigned int val, unsigned int mask,
				   const struct xtables_lmap *lmap);

static inline void xtables_print_mark_mask(unsigned int mark,
					   unsigned int mask)
{
	xtables_print_val_mask(mark, mask, NULL);
}

#if defined(ALL_INCLUSIVE) || defined(NO_SHARED_LIBS)
#	ifdef _INIT
#		undef _init
#		define _init _INIT
#	endif
	extern void init_extensions(void);
	extern void init_extensions4(void);
	extern void init_extensions6(void);
#else
#	define _init __attribute__((constructor)) _INIT
#endif

extern const struct xtables_pprot xtables_chain_protos[];
extern uint16_t xtables_parse_protocol(const char *s);

/* kernel revision handling */
extern int kernel_version;
extern void get_kernel_version(void);
#define LINUX_VERSION(x,y,z)	(0x10000*(x) + 0x100*(y) + z)
#define LINUX_VERSION_MAJOR(x)	(((x)>>16) & 0xFF)
#define LINUX_VERSION_MINOR(x)	(((x)>> 8) & 0xFF)
#define LINUX_VERSION_PATCH(x)	( (x)      & 0xFF)

/* xtoptions.c */
extern void xtables_option_metavalidate(const char *,
					const struct xt_option_entry *);
extern struct option *xtables_options_xfrm(struct option *, struct option *,
					   const struct xt_option_entry *,
					   unsigned int *);
extern void xtables_option_parse(struct xt_option_call *);
extern void xtables_option_tpcall(unsigned int, char **, bool,
				  struct xtables_target *, void *);
extern void xtables_option_mpcall(unsigned int, char **, bool,
				  struct xtables_match *, void *);
extern void xtables_option_tfcall(struct xtables_target *);
extern void xtables_option_mfcall(struct xtables_match *);
extern void xtables_options_fcheck(const char *, unsigned int,
				   const struct xt_option_entry *);

extern struct xtables_lmap *xtables_lmap_init(const char *);
extern void xtables_lmap_free(struct xtables_lmap *);
extern int xtables_lmap_name2id(const struct xtables_lmap *, const char *);
extern const char *xtables_lmap_id2name(const struct xtables_lmap *, int);

/* xlate infrastructure */
struct xt_xlate *xt_xlate_alloc(int size);
void xt_xlate_free(struct xt_xlate *xl);
void xt_xlate_add(struct xt_xlate *xl, const char *fmt, ...) __attribute__((format(printf,2,3)));
void xt_xlate_add_comment(struct xt_xlate *xl, const char *comment);
const char *xt_xlate_get_comment(struct xt_xlate *xl);
const char *xt_xlate_get(struct xt_xlate *xl);

#ifdef XTABLES_INTERNAL

/* Shipped modules rely on this... */

#	ifndef ARRAY_SIZE
#		define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))
#	endif

extern void _init(void);

#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _XTABLES_H */
