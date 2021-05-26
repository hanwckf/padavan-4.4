/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/device-mapper.h>
#include <linux/nfsb.h>

#include "do_mounts.h"

void dm_destroy(struct mapped_device *md);
int dm_table_alloc_md_mempools(struct dm_table *t);
void dm_table_destroy(struct dm_table *t);
int dm_table_set_type(struct dm_table *t);
