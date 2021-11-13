#include <linux/kernel.h>
#include <linux/sizes.h>

#include <linux/io.h>

#include "dysche_partition.h"

struct memory_layout {
	off_t offset;
	size_t size;
};

static const struct memory_layout layouts[] = {
	[DYSCHE_T_SH_DYSCHE_CONFIG] = { 0, SZ_1M },
	[DYSCHE_T_SH_VCONSOLE] = { SZ_1M, SZ_1M },
	[DYSCHE_T_SH_PARTEP] = { SZ_2M, SZ_4M },
	[DYSCHE_T_SLAVE_LOADER] = { SZ_8M, SZ_1M },
	[DYSCHE_T_SLAVE_FDT] = { SZ_8M + SZ_2M, SZ_4M + SZ_2M },
	[DYSCHE_T_SLAVE_KERNEL] = { SZ_16M, SZ_64M },
	[DYSCHE_T_SLAVE_ROOTFS] = { SZ_16M + SZ_64M, SZ_1G },
};

phys_addr_t dysche_get_mem_phy(struct dysche_instance *ins,
			     enum dysche_memory_type type)
{
	if (!ins || !ins->dysche_mem_region_nr)
		return 0;

	return ins->mems[0].phys + layouts[type].offset;
}

size_t dysche_get_mem_size(struct dysche_instance *ins,
			   enum dysche_memory_type type)
{
	return layouts[type].size;
}

int init_memory_layout(struct dysche_instance *ins)
{
	if (ins->dysche_mem_region_nr == 0 || ins->mems[0].phys == 0)
		return -EINVAL;

	ins->config = memremap(
		ins->mems[0].phys + layouts[DYSCHE_T_SH_DYSCHE_CONFIG].offset,
		layouts[DYSCHE_T_SH_DYSCHE_CONFIG].size, MEMREMAP_WB);
	if (!ins->config)
		return -EIO;

	return 0;
}

void fini_memory_layout(struct dysche_instance *ins)
{
	if (ins->config)
		memunmap(ins->config);
}

int fill_memory_region_from_dysche_resource(struct dysche_instance *ins,
					    enum dysche_memory_type type,
					    off_t offset)
{
	int ret = 0;
	off_t end;
	struct dysche_resource *r;
	void*loc;

	if (!ins || ins->dysche_mem_region_nr == 0)
		return -EINVAL;

	switch (type) {
	case DYSCHE_T_SLAVE_LOADER:
		r = &ins->loader;
		break;
	case DYSCHE_T_SLAVE_FDT:
		r = &ins->fdt;
		break;
	case DYSCHE_T_SLAVE_KERNEL:
		r = &ins->kernel;
		break;
	case DYSCHE_T_SLAVE_ROOTFS:
		r = &ins->rootfs;
		break;
	default:
		return -ENOTSUPP;
	}

	if (!r->enabled)
		return -EINVAL;

	if (!r->get_size || !r->get_resource)
		return -ENOTSUPP;

	ret = r->get_size(r);
	if (ret < 0)
		return ret;

	end = offset + ret;
	if (end > layouts[type].size)
		return -EOVERFLOW;

	end += layouts[type].offset;
	if (end > ins->mems[0].size)
		return -EOVERFLOW;

	// TODO: check memory align;

	loc = memremap(ins->mems[0].phys + layouts[type].offset + offset, ret,
		       MEMREMAP_WB);
	if (!loc)
		return -EIO;

	ret = r->get_resource(r, loc, ret);

	memunmap(loc);
	return ret;
}

int fill_memory_region(struct dysche_instance *ins,
		       enum dysche_memory_type type, off_t offset,
		       const void *buf, size_t count)
{
	void *loc;
	off_t end = offset + count;
	if (!ins || ins->dysche_mem_region_nr == 0)
		return -EINVAL;

	if (end > layouts[type].size)
		return -EOVERFLOW;

	end += layouts[type].offset;
	if (end > ins->mems[0].size)
		return -EOVERFLOW;

	// TODO: check memory align;

	loc = memremap(ins->mems[0].phys + layouts[type].offset + offset, count,
		       MEMREMAP_WB);
	if (!loc)
		return -EIO;

	memcpy(loc, buf, count);

	memunmap(loc);
	return 0;
}