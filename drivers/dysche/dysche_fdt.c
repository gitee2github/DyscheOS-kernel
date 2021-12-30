#include <linux/fdtable.h>
#include <linux/slab.h>
#include <linux/efi.h>
#include <linux/libfdt.h>

#include "dysche_partition.h"

#define FDT_SIZE_CELLS_DEFAULT 2
#define FDT_ADDR_CELLS_DEFAULT 2

#define MAX_FDT_SIZE SZ_2M

static int node_offset(void *fdt, const char *node_path)
{
	int offset = fdt_path_offset(fdt, node_path);
	if (offset == -FDT_ERR_NOTFOUND)
		/* Add the node to root if not found, dropping the leading '/' */
		offset = fdt_add_subnode(fdt, 0, node_path + 1);
	return offset;
}

static uint32_t get_cell_size(const void *fdt)
{
	int len;
	uint32_t cell_size = 1;
	int offset = fdt_path_offset(fdt, "/");
	const uint32_t *size_len = NULL;

	if (offset != -FDT_ERR_NOTFOUND)
		size_len = fdt_getprop(fdt, offset, "#size-cells", &len);

	if (size_len)
		cell_size = fdt32_to_cpu(*size_len);
	return cell_size;
}

static int setprop(void *fdt, const char *node_path, const char *property,
		   uint32_t *val_array, int size)
{
	int offset = node_offset(fdt, node_path);
	if (offset < 0)
		return offset;
	return fdt_setprop(fdt, offset, property, val_array, size);
}

int dysche_fdt_file_load_resource(struct dysche_resource *res)
{
	struct dysche_instance *ins = res->ins;
	struct file *fp;
	int ret, i, chosen_offset, mem_count = 0;
	uint32_t memsize;
	loff_t pos = 0;
	phys_addr_t paddr;
	uint32_t mem_reg_property[2 * 2 * DYSCHE_MAX_MEM_REGIONS];
	void *buf;

	if (!ins->fdt.enabled)
		return -EINVAL;

	if (!ins->dysche_mem_region_nr)
		return -EINVAL;

	fp = filp_open(res->filename, O_RDONLY, 0);
	if (IS_ERR(fp))
		return PTR_ERR(fp);

	paddr = dysche_get_mem_phy(ins, DYSCHE_T_SLAVE_FDT);
	buf = memremap(paddr, MAX_FDT_SIZE, MEMREMAP_WB);
	if (!buf) {
		ret = -EIO;
		goto close;
	}

	kernel_read(fp, buf, MAX_FDT_SIZE, &pos);

	ret = fdt_open_into(buf, buf, MAX_FDT_SIZE);
	if (ret)
		goto unmap;

	memsize = get_cell_size(buf);

	for (i = 0; i < ins->dysche_mem_region_nr; ++i) {
		if (i == 0) {
			paddr = dysche_get_mem_phy(ins, DYSCHE_T_SLAVE_KERNEL);
			pos = ins->mems[0].size - (paddr - ins->mems[0].phys);
		} else {
			paddr = ins->mems[i].phys;
			pos = ins->mems[i].size;
		}
		if (memsize == 2) {
			uint64_t *mem_reg_prop64 = (uint64_t *)mem_reg_property;
			mem_reg_prop64[mem_count++] = cpu_to_fdt64(paddr);
			mem_reg_prop64[mem_count++] = cpu_to_fdt64(pos);
		} else {
			mem_reg_property[mem_count++] = cpu_to_fdt32(paddr);
			mem_reg_property[mem_count++] = cpu_to_fdt32(pos);
		}
	}

	chosen_offset = node_offset(buf, "/chosen");
	if (chosen_offset < 0) {
		chosen_offset = fdt_add_subnode(buf, 0, "/chosen");
		BUG_ON(chosen_offset < 0);
	}

	if (mem_count) {
		setprop(buf, "/memory", "reg", mem_reg_property,
			4 * mem_count * memsize);
	}
	if (ins->rootfs.enabled) {
		paddr = dysche_get_mem_phy(ins, DYSCHE_T_SLAVE_ROOTFS);
		i = ins->rootfs.get_size(&ins->rootfs);
		fdt_setprop_u64(buf, chosen_offset, "linux,initrd-start",
				paddr);
		fdt_setprop_u64(buf, chosen_offset, "linux,initrd-end",
				paddr + i);
	}

	ret = fdt_pack(buf);
unmap:
	memunmap(buf);
close:
	filp_close(fp, NULL);
	return ret;
}

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

	ins->fdt.rawdata.data = buf;
	ins->fdt.rawdata.size = MAX_FDT_SIZE;
	return 0;

free_buf:
	kfree(buf);
	return err;
}
