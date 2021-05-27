/* SPDX-License-Identifier: (GPL-2.0 OR BSD-2-Clause) */
/*
 * Simple streaming JSON writer
 *
 * This takes care of the annoying bits of JSON syntax like the commas
 * after elements
 *
 * Authors:	Stephen Hemminger <stephen@networkplumber.org>
 */

#ifndef _JSON_WRITER_H_
#define _JSON_WRITER_H_

#include <stdbool.h>
#include <stdint.h>

/* Opaque class structure */
typedef struct json_writer json_writer_t;

/* Create a new JSON stream */
json_writer_t *jsonw_new(FILE *f);
/* End output to JSON stream */
void jsonw_destroy(json_writer_t **self_p);

/* Cause output to have pretty whitespace */
void jsonw_pretty(json_writer_t *self, bool on);

/* Add property name */
void jsonw_name(json_writer_t *self, const char *name);

/* Add value  */
__attribute__((format(printf, 2, 3)))
void jsonw_printf(json_writer_t *self, const char *fmt, ...);
void jsonw_string(json_writer_t *self, const char *value);
void jsonw_bool(json_writer_t *self, bool value);
void jsonw_float(json_writer_t *self, double number);
void jsonw_float_fmt(json_writer_t *self, const char *fmt, double num);
void jsonw_uint(json_writer_t *self, unsigned int number);
void jsonw_u64(json_writer_t *self, uint64_t number);
void jsonw_xint(json_writer_t *self, uint64_t number);
void jsonw_hhu(json_writer_t *self, unsigned char num);
void jsonw_hu(json_writer_t *self, unsigned short number);
void jsonw_int(json_writer_t *self, int number);
void jsonw_s64(json_writer_t *self, int64_t number);
void jsonw_null(json_writer_t *self);
void jsonw_luint(json_writer_t *self, unsigned long num);
void jsonw_lluint(json_writer_t *self, unsigned long long num);

/* Useful Combinations of name and value */
void jsonw_string_field(json_writer_t *self, const char *prop, const char *val);
void jsonw_bool_field(json_writer_t *self, const char *prop, bool value);
void jsonw_float_field(json_writer_t *self, const char *prop, double num);
void jsonw_uint_field(json_writer_t *self, const char *prop, unsigned int num);
void jsonw_u64_field(json_writer_t *self, const char *prop, uint64_t num);
void jsonw_xint_field(json_writer_t *self, const char *prop, uint64_t num);
void jsonw_hhu_field(json_writer_t *self, const char *prop, unsigned char num);
void jsonw_hu_field(json_writer_t *self, const char *prop, unsigned short num);
void jsonw_int_field(json_writer_t *self, const char *prop, int num);
void jsonw_s64_field(json_writer_t *self, const char *prop, int64_t num);
void jsonw_null_field(json_writer_t *self, const char *prop);
void jsonw_luint_field(json_writer_t *self, const char *prop,
			unsigned long num);
void jsonw_lluint_field(json_writer_t *self, const char *prop,
			unsigned long long num);

/* Collections */
void jsonw_start_object(json_writer_t *self);
void jsonw_end_object(json_writer_t *self);

void jsonw_start_array(json_writer_t *self);
void jsonw_end_array(json_writer_t *self);

/* Override default exception handling */
typedef void (jsonw_err_handler_fn)(const char *);

#endif /* _JSON_WRITER_H_ */
