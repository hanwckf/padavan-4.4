/*
 * This file is subject to the terms and conditions of the LGPL V2.1
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2019 Kalray Inc.
 */

#ifndef _KVX_DL_TLS_H
#define _KVX_DL_TLS_H 1

/* Type used to represent a TLS descriptor in the GOT.  */
struct tlsdesc
{
  ptrdiff_t (*entry) (struct tlsdesc *);
  void *arg;
};

typedef struct dl_tls_index
{
  unsigned long int ti_module;
  unsigned long int ti_offset;
} tls_index;

/* Type used as the argument in a TLS descriptor for a symbol that
   needs dynamic TLS offsets.  */
struct tlsdesc_dynamic_arg
{
  tls_index tlsinfo;
  size_t gen_count;
};

extern ptrdiff_t attribute_hidden
_dl_tlsdesc_return (struct tlsdesc *);

# ifdef SHARED
extern void *_dl_make_tlsdesc_dynamic (struct link_map *, size_t);

extern ptrdiff_t attribute_hidden
_dl_tlsdesc_dynamic (struct tlsdesc *);
# endif

extern void *__tls_get_addr (tls_index *ti);

#define TLS_DTV_UNALLOCATED ((void *) -1l)

#endif
