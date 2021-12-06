#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/idr.h>
#include <linux/slab.h>

#include "dysche_partition.h"

static DEFINE_IDR(dysche_instances);

extern int dysche_boot_cpu(int cpu, phys_addr_t addr);

static int raw_get_size(struct dysche_resource *r)
{
	if (!r || !r->enabled)
		return -EINVAL;

	return r->rawdata.size;
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

	memcpy(buf, r->rawdata.data, count);

	return 0;
}

static int raw_fdt_release(struct dysche_resource *r)
{
	if (!r)
		return -EINVAL;

	kfree(r->rawdata.data);
	return 0;
}

static int file_release(struct dysche_resource *r)
{
	if (!r)
		return 0;

	kfree(r->filename);
	return 0;
}

static int file_get_size(struct dysche_resource *r)
{
	struct file *fp;
	mm_segment_t fs;
	struct kstat stat;
	int ret, size = 0;
	if (!r || !r->enabled)
		return -EINVAL;

	fp = filp_open(r->filename, O_RDONLY, 0);
	if (IS_ERR(fp))
		return PTR_ERR(fp);

	//fs = get_fs();
	//set_fs(KERNEL_DS);

	//ret = vfs_stat(r->filename, &stat);
	//size = stat.size;
	size = fp->f_inode->i_size;
	filp_close(fp, NULL);
	//set_fs(fs);

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

	fp = filp_open(r->filename, O_RDONLY, 0);
	if (IS_ERR(fp))
		return PTR_ERR(fp);

	//fs = get_fs();
	//set_fs(KERNEL_DS);

	kernel_read(fp, buf, count, &pos);

	filp_close(fp, NULL);
	//set_fs(fs);

	return 0;
}

int si_create(const char *buf, struct dysche_instance **contain)
{
	// TODO: status change.
	struct dysche_instance *ins;
	int ret;

	ins = kzalloc(sizeof(*ins), GFP_KERNEL);
	if (!ins)
		return -ENOMEM;

	ret = idr_alloc(&dysche_instances, ins, 1, PARTITION_MAXCNT,
			GFP_KERNEL);
	if (ret < 0)
		goto err_alloc;

	ins->slave_id = ret;

	// default callbacks.
	ins->fdt.get_size = ins->kernel.get_size = ins->rootfs.get_size =
		file_get_size;
	ins->fdt.get_resource = ins->kernel.get_resource =
		ins->rootfs.get_resource = file_get_resource;
	ins->fdt.release = ins->kernel.release = ins->rootfs.release =
		file_release;
	ins->loader.get_size = ins->fdt.get_size = raw_get_size;
	ins->loader.get_resource = ins->fdt.get_resource = raw_get_resource;

	ret = dysche_parse_args(ins, buf);
	if (ret)
		goto err_parse;

	ret = init_memory_layout(ins);
	if (ret)
		goto err_parse;

	ret = dysche_prepare_loader(ins);
	if (ret)
		goto err_layout;

	// TODO: fill cmdline for dysche_instance.

	if (!ins->fdt.enabled) {
		ret = dysche_generate_fdt(ins);
		if (ret)
			goto err_layout;

		ins->fdt.release = raw_fdt_release;
		ins->fdt.enabled = true;
	}

	if (!ins->kernel.enabled) {
		pr_err("No kernel provided, exit.");
		goto err_layout;
	}

	ret = init_partition_sysfs(ins);
	if (ret)
		goto err_layout;

	*contain = ins;
	return 0;

err_layout:
	fini_memory_layout(ins);

err_parse:
	release_dysche_resource(&ins->kernel);
	release_dysche_resource(&ins->loader);
	release_dysche_resource(&ins->rootfs);
	release_dysche_resource(&ins->fdt);
	idr_remove(&dysche_instances, ins->slave_id);

err_alloc:
	kfree(ins);
	return ret;
}

void si_lock(struct dysche_instance *ins)
{
}
void si_unlock(struct dysche_instance *ins)
{
}

int si_run(struct dysche_instance *ins)
{
	// you can reload kernel and rootfs with reboot.
	// TODO: status change.
	int ret, cpu;
	unsigned long addr;
	ret = init_dysche_config(ins);
	if (ret)
		return ret;

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

	// TODO: prepare resources.

	cpu = cpumask_first(&ins->cpu_mask);
	addr = dysche_get_mem_phy(ins, DYSCHE_T_SLAVE_LOADER);
	pr_info("Use cpu%d for boot cpu on addr(0x%lx)", cpu, addr);
	ret = dysche_boot_cpu(cpu, addr);
	if (ret) {
		pr_err("boot cpu failed(%d).", ret);
		return ret;
	}
	return 0;
}
int si_destroy(struct dysche_instance *ins)
{
	fini_dysche_config(ins);

	fini_partition_sysfs(ins);

	release_dysche_resource(&ins->kernel);
	release_dysche_resource(&ins->loader);
	release_dysche_resource(&ins->rootfs);
	release_dysche_resource(&ins->fdt);

	idr_remove(&dysche_instances, ins->slave_id);

	kfree(ins);
	return 0;
}