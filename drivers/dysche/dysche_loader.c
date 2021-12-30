#include <linux/io.h>

#include "dysche_partition.h"

extern char _dysche_loader_start[];
extern char _dysche_loader_end[];

extern unsigned long _dysche_fdt_offset;
extern unsigned long _dysche_kernel_offset;

static unsigned long text_fdt_offset;
static unsigned long text_kernel_offset;

static unsigned long _fdt_offset;
static unsigned long _kernel_offset;

static int loader_load_resource(struct dysche_resource *r)
{
	struct dysche_instance *ins = r->ins;
	unsigned long *tgt;
	int ret, count;
	phys_addr_t paddr;
	void *buf;

	if (!r || !r->enabled || !r->get_size)
		return -EINVAL;

	paddr = dysche_get_mem_phy(ins, DYSCHE_T_SLAVE_LOADER);
	count = dysche_get_mem_size(ins, DYSCHE_T_SLAVE_LOADER);

	ret = r->get_size(r);
	if (ret < 0)
		return ret;

	if (ret > count)
		return -EOVERFLOW;

	buf = memremap(paddr, ret, MEMREMAP_WB);
	if (!buf)
		return -EIO;

	memcpy(buf, r->rawdata.data, count);

	tgt = (unsigned long *)(buf + text_fdt_offset);
	*tgt = _fdt_offset;
	tgt = (unsigned long *)(buf + text_kernel_offset);
	*tgt = _kernel_offset;

	memunmap(buf);

	return 0;
}

int dysche_prepare_loader(struct dysche_instance *ins)
{
	phys_addr_t loader, fdt, kernel;
	// default to use builtin loader.
	ins->loader.ins = ins;
	ins->loader.type = DYSCHE_T_SLAVE_LOADER;
	ins->loader.rawdata.data = _dysche_loader_start;
	ins->loader.rawdata.size = _dysche_loader_end - _dysche_loader_start;

	loader = dysche_get_mem_phy(ins, DYSCHE_T_SLAVE_LOADER);
	fdt = dysche_get_mem_phy(ins, DYSCHE_T_SLAVE_FDT);
	kernel = dysche_get_mem_phy(ins, DYSCHE_T_SLAVE_KERNEL);

	text_fdt_offset =
		(void *)&_dysche_fdt_offset - (void *)_dysche_loader_start;
	text_kernel_offset =
		(void *)&_dysche_kernel_offset - (void *)_dysche_loader_start;

	_fdt_offset = fdt - loader;
	_kernel_offset = kernel - loader;

	ins->loader.load_resource = loader_load_resource;
	ins->loader.enabled = true;

	return 0;
}