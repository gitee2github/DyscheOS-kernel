#include <linux/fs.h>
#include <asm/uaccess.h>

#include "dysche_partition.h"

static int raw_get_size(struct dysche_resource* r)
{
	if (!r||!r->enabled)
		return -EINVAL;

	return r->resource.rawdata.size;
}

static int raw_get_resource(struct dysche_resource *r, void *buf, size_t count)
{
	int ret;
	if (!r || !r->enabled || !r->get_size)
		return -EINVAL;

	ret = r->get_size(r);
	if (ret < 0)
		return ret;

	if (ret > count)
		return -EOVERFLOW;

	memcpy(buf, r->resource.rawdata.data, count);

	return 0;
}

static int file_get_size(struct dysche_resource *r)
{
	struct file* fp;
	mm_segment_t fs;
	struct kstat stat;
	int size = 0;
	if (!r||!r->enabled)
		return -EINVAL;
	
	fp = filp_open(r->resource.filename, O_RDONLY, 0);
	if (IS_ERR(fp))
		return PTR_ERR(fp);

	fs = get_fs();
	set_fs(KERNEL_DS);
	
	vfs_stat(r->resource.filename, &stat);
	size = stat.size;
	
	filp_close(fp, NULL);
	set_fs(fs);

	return size;
}

static int file_get_resource(struct dysche_resource *r, void *buf, size_t count)
{
	struct file *fp;
	mm_segment_t fs;
	int size = 0;
	loff_t pos = 0;
	if (!r || !r->enabled || !r->get_size)
		return -EINVAL;

	size = r->get_size(r);
	if (size < 0)
		return size;

	if (size > count)
		return -EOVERFLOW;

	fp = filp_open(r->resource.filename, O_RDONLY, 0);
	if (IS_ERR(fp))
		return PTR_ERR(fp);

	fs = get_fs();
	set_fs(KERNEL_DS);

	kernel_read(fp, buf, count, &pos);

	filp_close(fp, NULL);
	set_fs(fs);

	return size;
}

int si_create(const char *buf, struct dysche_instance **ins)
{
	// TODO: parse args.
	// fill cpu memory pci kernel fdt rootfs resources

	return 0;
}

void si_lock(struct dysche_instance *ins)
{
}
void si_unlock(struct dysche_instance *ins)
{
}

int si_run(struct dysche_instance *ins)
{
	// TODO:
	int ret;
	ret = fill_memory_region_from_dysche_resource(ins,
						      DYSCHE_T_SLAVE_LOADER, 0);
	if (ret) {
		pr_err("load loader failed.");
		return -ENOTSUPP;
	}

	ret = fill_memory_region_from_dysche_resource(ins,
						      DYSCHE_T_SLAVE_KERNEL, 0);
	if (ret) {
		pr_err("load kernel failed.");
		return -ENOTSUPP;
	}

	ret = fill_memory_region_from_dysche_resource(ins, DYSCHE_T_SLAVE_FDT,
						      0);
	if (ret)
		pr_warn("load fdt failed.");

	ret = fill_memory_region_from_dysche_resource(ins,
						      DYSCHE_T_SLAVE_ROOTFS, 0);
	if (ret)
		pr_warn("load rootfs failed.");
	return 0;
}
int si_destroy(struct dysche_instance *ins)
{
	release_dysche_resource(&ins->kernel);
	release_dysche_resource(&ins->loader);
	release_dysche_resource(&ins->rootfs);
	release_dysche_resource(&ins->fdt);
	return 0;
}