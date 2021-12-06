#include "dysche_partition.h"

extern char _dysche_loader_start[];
extern char _dysche_loader_end[];

extern unsigned long _dysche_fdt_offset;
extern unsigned long _dysche_kernel_offset;

static unsigned long text_fdt_offset;
static unsigned long text_kernel_offset;

static unsigned long _fdt_offset;
static unsigned long _kernel_offset;

static int loader_get_resource(struct dysche_resource *r, void *buf,
			       size_t count)
{
	unsigned long *tgt;
	int ret;
	if (!r || !r->enabled || !r->get_size)
		return -EINVAL;

	ret = r->get_size(r);
	if (ret < 0)
		return ret;

	if (ret > count)
		return -EOVERFLOW;

	memcpy(buf, r->rawdata.data, count);

	tgt = (unsigned long *)(buf + text_fdt_offset);
	*tgt = _fdt_offset;
	tgt = (unsigned long *)(buf + text_kernel_offset);
	*tgt = _kernel_offset;

	return 0;
}

int dysche_prepare_loader(struct dysche_instance *ins)
{
	phys_addr_t loader, fdt, kernel;
	// default to use builtin loader.
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

	ins->loader.get_resource = loader_get_resource;
	ins->loader.enabled = true;

	return 0;
}