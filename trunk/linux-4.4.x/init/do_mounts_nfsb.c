/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include "do_mounts_nfsb.h"
#include "../drivers/md/dm.h"


static char *DEV_LINEAR = "/dev/linear";
static char *DEV_NFSB = "/dev/nfsb";

#include "nfsb_key_public.h"
#include "nfsb_key_modulus.h"

/* Called in kernel space for non-rootfs image */
int dm_check_nonroot_nfsb(struct nfsb_header *hdr, char *devname)
{
	ssize_t sz = -1;
	mm_segment_t oldfs;
	struct file *fp;

	pr_info("%s: Start to NFSB header found and verified.\n", devname);

	/* Bypass memory boundary check when set KERNEL_DS. */
	oldfs = get_fs();
	set_fs(KERNEL_DS);

	/* Try to read an NFSB header from the root device. */
	fp = filp_open(devname, O_RDONLY, 0);
	if (!fp) {
		pr_err("nfsb: open %s failed\n", devname);
		goto open_fail;
	}

	sz = nfsb_nonroot_read(fp, hdr);
	if (sz == -1) {
		pr_err("nfsb: check %s header fail\n", devname);
		goto read_fail;
	}

	/* Verify the NFSB header. */
	if (nfsb_verify(hdr, rsa_pubkey, rsa_modulus) != 0) {
		pr_err("nfsb: header verification failed\n");
		goto verify_fail;
	}

	pr_info("%s: NFSB header found and verified.\n", devname);

	/* Success. */
	filp_close(fp, NULL);
	set_fs(oldfs);
	return 1;

verify_fail:
read_fail:
	filp_close(fp, NULL);
open_fail:
	set_fs(oldfs);
	return 0;
}

static int __init dm_check_nfsb(struct nfsb_header *hdr, dev_t dev)
{
	int err = 0;
	int root_fd = -1;
	ssize_t sz = -1;

	pr_info("Start to NFSB header found and verified.\n");

	/* Create a temporary root device. */
	err = create_dev(DEV_NFSB, dev);
	if (err < 0) {
		pr_err("nfsb: create_dev failed\n");
		goto create_fail;
	}

	/* Try to read an NFSB header from the root device. */
	root_fd = sys_open(DEV_NFSB, O_RDONLY, 0);
	if (root_fd < 0) {
		pr_err("nfsb: open %s failed\n", DEV_NFSB);
		goto open_fail;
	}
	sz = nfsb_read(root_fd, hdr);
	if (sz == -1)
		goto read_fail;

	/* Verify the NFSB header. */
	if (nfsb_verify(hdr, rsa_pubkey, rsa_modulus) != 0) {
		pr_err("nfsb: header verification failed\n");
		goto verify_fail;
	}
	pr_info("NFSB header found and verified.\n");

	/* Success. */
	sys_close(root_fd);
	sys_unlink(DEV_NFSB);
	return 1;

verify_fail:
read_fail:
	sys_close(root_fd);
open_fail:
	sys_unlink(DEV_NFSB);
create_fail:
	return 0;
}

/* From drivers/md/dm.h */
#define DM_SUSPEND_NOFLUSH_FLAG         (1 << 1)

static int __init dm_setup_linear(struct nfsb_header *hdr, dev_t *dev)
{
	int linear_fd = -1;
	unsigned long linear_size = 0;
	struct mapped_device *md = NULL;
	struct dm_table *table = NULL, *old_map = NULL;
	char *target_params = NULL;
	ssize_t fs_offset = to_sector(nfsb_fs_offset(hdr));

	/* Create linear device. */
	if (create_dev(DEV_LINEAR, *dev) < 0) {
		pr_err("Failed to create linear device\n");
		goto create_dev_fail;
	}

	/* Size block device. */
	linear_fd = sys_open(DEV_LINEAR, O_RDONLY, 0);
	if (linear_fd < 0) {
		pr_err("Failed to open linear device\n");
		goto sys_open_fail;
	}
	if (sys_ioctl(linear_fd, BLKGETSIZE, (unsigned long)&linear_size) < 0) {
		pr_err("Failed to get linear device size\n");
		goto sys_ioctl_fail;
	}

	/* Create a new mapped device. */
	if (dm_create(DM_ANY_MINOR, &md)) {
		pr_err("Failed to create mapped device\n");
		goto create_fail;
	}

	/* Explicitly set read-only. */
	set_disk_ro(dm_disk(md), 1);

	/* Create a mapped device table. */
	if (dm_table_create(&table, FMODE_READ, 1, md)) {
		pr_err("Failed to create table\n");
		goto table_create_fail;
	}

	dm_lock_md_type(md);
	/* Add the linear target to the table. */
	target_params = kmalloc(128, GFP_KERNEL);
	snprintf(target_params, 128, "%s %ld", DEV_LINEAR, fs_offset);
	if (dm_table_add_target(table, "linear", 0, linear_size - fs_offset, target_params)) {
		pr_err("Failed to add linear target to the table\n");
		goto add_target_fail;
	}

	/* Table is complete. */
	if (dm_table_complete(table)) {
		pr_err("Failed to complete NFSB table\n");
		goto table_complete_fail;
	}

	if (dm_get_md_type(md) == DM_TYPE_NONE) {
		dm_set_md_type(md, dm_table_get_type(table));
		if (dm_setup_md_queue(md)) {
			pr_err("Unable to set up device queue for new table.");
			goto setup_md_queue_fail;
		}
	} else if (dm_get_md_type(md) != dm_table_get_type(table)) {
		pr_err("Can't change device type after initial table load.");
		goto setup_md_queue_fail;
	}

	/* Suspend the device so that we can bind it to the table. */
	if (dm_suspend(md, DM_SUSPEND_NOFLUSH_FLAG)) {
		pr_err("Failed to suspend the device pre-bind\n");
		goto suspend_fail;
	}

	/* Bind the table to the device. This is the only way to associate
	 * md->map with the table and set the disk capacity. Ouch.
	*/
	old_map = dm_swap_table(md, table);
	if (IS_ERR(old_map)) {
		pr_err("Failed to bind the device to the table\n");
		dm_table_destroy(table);
		goto swap_table_fail;
	}

	/* Finally, resume and the device should be ready. */
	if (dm_resume(md)) {
		pr_err("Failed to resume the device\n");
		goto resume_fail;
	}

	/* Success. */
	*dev = disk_devt(dm_disk(md));
	pr_info("dm: target linear of size %lu on %s(%i:%i) is ready\n",
		dm_table_get_size(table), dm_device_name(md), MAJOR(*dev), MINOR(*dev));
	if (old_map)
		dm_table_destroy(old_map);
	kfree(target_params);
	dm_unlock_md_type(md);
	sys_close(linear_fd);
	return 1;

resume_fail:
	if (old_map)
		dm_table_destroy(old_map);
swap_table_fail:
suspend_fail:
setup_md_queue_fail:
table_complete_fail:
add_target_fail:
	kfree(target_params);
	dm_unlock_md_type(md);
	dm_table_destroy(table);
table_create_fail:
	dm_put(md);
	dm_destroy(md);
create_fail:
sys_ioctl_fail:
	sys_close(linear_fd);
sys_open_fail:
create_dev_fail:
	return 0;
}

static dev_t __init dm_setup_nfsb(struct nfsb_header *hdr, dev_t *dev)
{
	struct mapped_device *md = NULL;
	struct dm_table *table = NULL, *old_map = NULL;
	char *target_params = NULL;
	ssize_t fs_size = nfsb_fs_size(hdr);
	uint32_t data_blocksize = nfsb_data_blocksize(hdr);
	uint32_t hash_blocksize = nfsb_hash_blocksize(hdr);

	/* Verify data and hash start block indexes. */
	if (fs_size % hash_blocksize) {
		pr_err("Hash start is not a multiple of the hash block size.\n");
		goto hash_start_fail;
	}

	/* Create NFSB device. */
	if (create_dev(DEV_NFSB, *dev) < 0) {
		pr_err("Failed to create NFSB root device\n");
		goto create_dev_fail;
	}

	/* Create a new mapped device. */
	if (dm_create(DM_ANY_MINOR, &md)) {
		pr_err("Failed to create mapped device\n");
		goto create_fail;
	}

	/* Explicitly set read-only. */
	set_disk_ro(dm_disk(md), 1);

	/* Create a mapped device table. */
	if (dm_table_create(&table, FMODE_READ, 1, md)) {
		pr_err("Failed to create table\n");
		goto table_create_fail;
	}

	dm_lock_md_type(md);
	/* Add the NFSB verity target to the table. */
	target_params = kmalloc(384, GFP_KERNEL);
	snprintf(target_params, 384, "%u %s %s %u %u %u %ld %s %s %s",
		 nfsb_hash_type(hdr), DEV_NFSB, DEV_NFSB,
		 data_blocksize, hash_blocksize, nfsb_data_blockcount(hdr),
		 fs_size / hash_blocksize, nfsb_hash_algo(hdr),
		 nfsb_verity_hash(hdr), nfsb_verity_salt(hdr));
	if (dm_table_add_target(table, "verity", 0, to_sector(fs_size), target_params)) {
		pr_err("Failed to add NFSB target to the table\n");
		goto add_target_fail;
	}

	/* Table is complete. */
	if (dm_table_complete(table)) {
		pr_err("Failed to complete NFSB table\n");
		goto table_complete_fail;
	}

	if (dm_get_md_type(md) == DM_TYPE_NONE) {
		dm_set_md_type(md, dm_table_get_type(table));
		if (dm_setup_md_queue(md)) {
			pr_err("Unable to set up device queue for new table.");
			goto setup_md_queue_fail;
		}
	} else if (dm_get_md_type(md) != dm_table_get_type(table)) {
		pr_err("Can't change device type after initial table load.");
		goto setup_md_queue_fail;
	}

	/* Suspend the device so that we can bind it to the table. */
	if (dm_suspend(md, DM_SUSPEND_NOFLUSH_FLAG)) {
		pr_err("Failed to suspend the device pre-bind\n");
		goto suspend_fail;
	}

	/* Bind the table to the device. This is the only way to associate
	 * md->map with the table and set the disk capacity. Ouch.
	*/
	old_map = dm_swap_table(md, table);
	if (IS_ERR(old_map)) {
		pr_err("Failed to bind the device to the table\n");
		dm_table_destroy(table);
		goto swap_table_fail;
	}

	/* Finally, resume and the device should be ready. */
	if (dm_resume(md)) {
		pr_err("Failed to resume the device\n");
		goto resume_fail;
	}

	/* Success. */
	*dev = disk_devt(dm_disk(md));
	pr_info("dm: target verity of size %lu on %s(%i:%i) is ready\n",
		dm_table_get_size(table), dm_device_name(md), MAJOR(*dev), MINOR(*dev));
	if (old_map)
		dm_table_destroy(old_map);
	kfree(target_params);
	dm_unlock_md_type(md);
	return 1;

resume_fail:
	if (old_map)
		dm_table_destroy(old_map);
swap_table_fail:
suspend_fail:
setup_md_queue_fail:
table_complete_fail:
add_target_fail:
	kfree(target_params);
	dm_unlock_md_type(md);
	dm_table_destroy(table);
table_create_fail:
	dm_put(md);
	dm_destroy(md);
create_fail:
create_dev_fail:
hash_start_fail:
	return 0;
}

dev_t __init dm_mount_nfsb(dev_t dev)
{
	struct nfsb_header *hdr = NULL;

	hdr = kmalloc(sizeof(struct nfsb_header), GFP_KERNEL);

	pr_info("Checking for NFSB mount.\n");
	if (!dm_check_nfsb(hdr, dev))
		panic("No NFSB image found.");
	if (!dm_setup_linear(hdr, &dev))
		panic("Linear setup failed.");
	if (!dm_setup_nfsb(hdr, &dev)) {
		sys_unlink(DEV_LINEAR);
		panic("NFSB setup failed.");
	}

	kfree(hdr);
	return dev;
}
