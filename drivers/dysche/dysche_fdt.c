#include <linux/fdtable.h>
#include <asm/boot.h>
#include <linux/slab.h>
#include <linux/efi.h>
#include <linux/libfdt.h>

#include "dysche_partition.h"

#define FDT_SIZE_CELLS_DEFAULT 2
#define FDT_ADDR_CELLS_DEFAULT 2

int dysche_generate_fdt(struct dysche_instance *ins)
{
	int err, offset, i;
	void *buf;
	size_t size;
	u64 memory[DYSCHE_MAX_MEM_REGIONS * 2];
	phys_addr_t phys;
	struct efi_memory_map_data mm;

	if (!ins)
		return -EINVAL;

	if (ins->fdt.enabled)
		return 0;

	err = -ENOMEM;

	buf = kmalloc(MAX_FDT_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	err = fdt_create_empty_tree(buf, MAX_FDT_SIZE);
	if (err)
		goto free_buf;

	// cell size
	offset = fdt_path_offset(buf, "/");
	fdt_setprop_u32(buf, offset, "#address-cells", FDT_ADDR_CELLS_DEFAULT);
	fdt_setprop_u32(buf, offset, "#size-cells", FDT_SIZE_CELLS_DEFAULT);

	// add chosen
	offset = fdt_add_subnode(buf, 0, "chosen");
	if (offset < 0) {
		err = offset;
		goto free_buf;
	}

	// default: uefi-xxx
	fdt_setprop_u32(buf, offset, "linux,uefi-mmap-desc-ver",
			efi.memmap.desc_version);
	fdt_setprop_u32(buf, offset, "linux,uefi-mmap-desc-size",
			efi.memmap.desc_size);
	fdt_setprop_u32(buf, offset, "linux,uefi-mmap-size",
			efi.memmap.nr_map * efi.memmap.desc_size);
	fdt_setprop_u64(buf, offset, "linux,uefi-mmap-start",
			efi.memmap.phys_map);

	phys = efi_get_fdt_params(&mm);
	if (!phys)
		goto free_buf;
	fdt_setprop_u64(buf, offset, "linux,uefi-system-table", phys);

	// bootargs
	fdt_setprop_string(buf, offset, "bootargs", ins->cmdline);

	// initrd
	if (ins->rootfs.enabled) {
		phys = dysche_get_mem_phy(ins, DYSCHE_T_SLAVE_ROOTFS);
		size = dysche_get_mem_size(ins, DYSCHE_T_SLAVE_ROOTFS);

		fdt_setprop_u64(buf, offset, "linux,initrd-start", phys);
		fdt_setprop_u64(buf, offset, "linux,initrd-end", phys + size);
	}

	phys = dysche_get_mem_phy(ins, DYSCHE_T_SLAVE_LOADER);
	size = ins->mems[0].size - (phys - ins->mems[0].phys);
	memory[0] = cpu_to_fdt64(phys);
	memory[1] = cpu_to_fdt64(size);

	// usable-memory-range
	for (i = 1; i < ins->dysche_mem_region_nr; ++i) {
		memory[2 * i] = cpu_to_fdt64(ins->mems[i].phys);
		memory[2 * i + 1] = cpu_to_fdt64(ins->mems[i].size);
	}

	fdt_setprop(buf, offset, "linux,usable-memory-range", memory,
		    16 * ins->dysche_mem_region_nr);

	ins->fdt.resource.rawdata.data = buf;
	ins->fdt.resource.rawdata.size = MAX_FDT_SIZE;
	return 0;

free_buf:
	kfree(buf);
	return err;
}
