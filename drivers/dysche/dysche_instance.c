#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/idr.h>
#include <linux/slab.h>
#include <linux/io.h>

#include "dysche_partition.h"

static DEFINE_IDR(dysche_instances);

extern int dysche_boot_cpu(int cpu, phys_addr_t addr);

static int raw_get_size(struct dysche_resource *r)
{
	if (!r || !r->enabled)
		return -EINVAL;

	return r->rawdata.size;
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
	//mm_segment_t fs;
	//struct kstat stat;
	//int ret = 0;
	int size = 0;
	if (!r || !r->enabled)
		return -EINVAL;

	fp = filp_open(r->filename, O_RDONLY, 0);
	if (IS_ERR(fp))
		return PTR_ERR(fp);

	size = fp->f_inode->i_size;
	filp_close(fp, NULL);

	return size;
}

static int file_load_resource(struct dysche_resource *r)
{
	struct dysche_instance *ins = r->ins;
	struct file *fp;
	int ret = 0, count;
	loff_t pos = 0;
	phys_addr_t paddr;
	void *buf;

	if (!r || !r->enabled || !r->get_size)
		return -EINVAL;

	paddr = dysche_get_mem_phy(ins, r->type);
	count = dysche_get_mem_size(ins, r->type);

	ret = r->get_size(r);
	if (ret < 0)
		return ret;

	if (ret > count)
		return -EOVERFLOW;

	buf = memremap(paddr, ret, MEMREMAP_WB);
	if (!buf)
		return -EIO;

	fp = filp_open(r->filename, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		ret = PTR_ERR(fp);
		goto unmap;
	}

	kernel_read(fp, buf, count, &pos);

	filp_close(fp, NULL);

unmap:
	memunmap(buf);

	return 0;
}

extern int dysche_fdt_file_load_resource(struct dysche_resource *res);

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
	ins->kernel.load_resource = ins->rootfs.load_resource =
		file_load_resource;
	ins->fdt.load_resource = dysche_fdt_file_load_resource;
	ins->fdt.release = ins->kernel.release = ins->rootfs.release =
		file_release;
	ins->loader.get_size = raw_get_size;

	ret = dysche_parse_args(ins, buf);
	if (ret)
		goto err_parse;

	ret = init_memory_layout(ins);
	if (ret)
		goto err_parse;

	if (!ins->kernel.enabled) {
		pr_err("No kernel provided, exit.");
		goto err_layout;
	}

	if (!ins->loader.enabled) {
		ins->loader.get_size = raw_get_size;
		ret = dysche_prepare_loader(ins);
		if (ret)
			goto err_layout;
	}

	// TODO: fill cmdline for dysche_instance.

	if (!ins->fdt.enabled) {
		printk(KERN_INFO"Prepare FDT for [%s]\n", ins->name);
		ret = dysche_generate_fdt(ins);
		if (ret)
			goto err_layout;

		ins->fdt.release = raw_fdt_release;
		ins->fdt.enabled = true;
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

	pr_info("Dysche Run: %s", ins->name);
	ret = init_dysche_config(ins);
	if (ret)
		return ret;

	if (!ins->loader.enabled)
		return -ENOTSUPP;

	ret = ins->loader.load_resource(&ins->loader);
	if (ret) {
		pr_err("load loader failed.");
		return -ENOTSUPP;
	}
	pr_debug(":loader loaded.");

	if (!ins->kernel.enabled)
		return -ENOTSUPP;

	ret = ins->kernel.load_resource(&ins->kernel);
	if (ret) {
		pr_err("load kernel failed.");
		return -ENOTSUPP;
	}
	pr_debug(": kernel loaded.");

	if (ins->fdt.enabled) {
		ret = ins->fdt.load_resource(&ins->fdt);
		if (ret)
			pr_warn("load fdt failed, do not use fdt.");
	}

	if (ins->rootfs.enabled) {
		ret = ins->rootfs.load_resource(&ins->rootfs);
		if (ret)
			pr_warn("load rootfs failed, do not use rootfs.");
	}

	// TODO: prepare resources.

	cpu = cpumask_first(&ins->cpu_mask);
	addr = dysche_get_mem_phy(ins, DYSCHE_T_SLAVE_LOADER);
	pr_debug("Use cpu%d as boot cpu, start from addr(0x%lx)", cpu, addr);

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
