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
	[DYSCHE_T_SLAVE_LOADER] = { SZ_8M - SZ_2M, SZ_2M },
	[DYSCHE_T_SLAVE_KERNEL] = { SZ_8M, SZ_128M },
	[DYSCHE_T_SLAVE_FDT] = { SZ_8M + SZ_128M, SZ_8M },
	[DYSCHE_T_SLAVE_ROOTFS] = { SZ_16M + SZ_128M, SZ_512M },
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
