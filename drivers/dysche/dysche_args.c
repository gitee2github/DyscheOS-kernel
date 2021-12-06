#include <linux/types.h>
#include <linux/parser.h>
#include <linux/slab.h>

#include "dysche_partition.h"

typedef int (*dysche_parser)(struct dysche_instance *, char *, int);

enum {
	Opt_slave_name,
	Opt_memory,
	Opt_cpu_ids,
	Opt_cmdline,
	// Opt_heartbeat_timeout,
	Opt_fdt,
	Opt_kernel,
	Opt_rootfs,
	Opt_ostype,
	Opt_err,
};

static int dysche_parse_normal_string(struct dysche_instance *ins, char *buf,
				      int token)
{
	char *target;
	int limit;

	switch (token) {
	case Opt_slave_name:
		target = ins->slave_name;
		limit = DYSCHE_NAME_LEN;
		break;
	case Opt_cmdline:
		target = ins->cmdline;
		limit = DYSCHE_MAX_CMDLINE_LEN;
		break;
	default:
		return -EINVAL;
	}

	if (strlen(buf) > limit)
		return -EOVERFLOW;

	strcpy(target, buf);
	return 0;
}

static int dysche_parse_memory_regions(struct dysche_instance *ins, char *buf,
				       int token)
{
	char *old;
	unsigned long start_at, mem_size;

	if (!buf)
		return -EINVAL;

	old = buf;
	mem_size = memparse(buf, &buf);
	if (old == buf)
		return -EINVAL;

	if (*buf == '@') {
		start_at = memparse(buf + 1, &buf);
		ins->mems[0].phys = start_at;
		ins->mems[0].size = mem_size;
		ins->dysche_mem_region_nr = 1;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int dysche_parse_cpu(struct dysche_instance *ins, char *buf, int token)
{
	int cpu, ret;
	cpumask_t tmp_mask;

	ret = cpulist_parse(buf, &tmp_mask);
	if (ret)
		return ret;

	for_each_cpu(cpu, &tmp_mask)
		if (!cpu_possible(cpu))
			return -EOVERFLOW;

	cpumask_copy(&ins->cpu_mask, &tmp_mask);

	return 0;
}

static int dysche_parse_file_resource(struct dysche_instance *ins, char *buf,
				      int token)
{
	struct dysche_resource *target;
	switch (token) {
	case Opt_kernel:
		target = &ins->kernel;
		break;
	case Opt_rootfs:
		target = &ins->rootfs;
		break;
	case Opt_fdt:
		target = &ins->fdt;
		break;
	default:
		return -EINVAL;
	}

	target->enabled = true;
	target->filename = kstrdup(buf, GFP_KERNEL);
	return 0;
}

static int dysche_parse_ostype(struct dysche_instance *ins, char *buf,
			       int token)
{
	if (strcmp(buf, "linux") == 0)
		ins->ostype = LINUX_KERNEL;
	else
		ins->ostype = UNKNOWN_KERNEL;
	return 0;
}

static dysche_parser dysche_parsers[] = {
	[Opt_slave_name] = dysche_parse_normal_string,
	[Opt_memory] = dysche_parse_memory_regions,
	[Opt_cpu_ids] = dysche_parse_cpu,
	[Opt_cmdline] = dysche_parse_normal_string,
	[Opt_fdt] = dysche_parse_file_resource,
	[Opt_kernel] = dysche_parse_file_resource,
	[Opt_rootfs] = dysche_parse_file_resource,
	[Opt_ostype] = dysche_parse_ostype,
};

static match_table_t s_dysche_create_args = {
	{ Opt_slave_name, "slave_name=%s" },

	// Format: SlaveOS-Memory Configuration, SIZE@ADDR,SIZE@ADDR...
	{ Opt_memory, "memory=%s" },

	// Format: SlaveOS CPU, <cpu number>-cpu number>:<used size>/<group size>
	{ Opt_cpu_ids, "cpu_ids=%s" },

	{ Opt_cmdline, "cmdline=%s" },

	// Format: Resources
	{ Opt_kernel, "kernel=%s" },
	{ Opt_rootfs, "rootfs=%s" },
	{ Opt_fdt, "fdt=%s" },
	{ Opt_ostype, "ostype=%s" },

	// Err
	{ Opt_err, NULL },
};


// TODO: parse devices.
int dysche_parse_args(struct dysche_instance *si, const char *str)
{
	int err;
	int token;
	char *p;
	char *raw_str, *origin_str;
	char buf[512];
	substring_t args[MAX_OPT_ARGS];
	dysche_parser parser;

	origin_str = raw_str = kstrdup(str, GFP_KERNEL);
	if (!raw_str)
		return -ENOMEM;

	while ((p = strsep(&raw_str, " ")) != NULL) {
		if (!*p)
			continue;

		token = match_token(p, s_dysche_create_args, args);
		if (token >= Opt_err || token < 0) {
			err = -EINVAL;
			break;
		}
		match_strlcpy(buf, &args[0], sizeof(buf));
		parser = dysche_parsers[token];
		err = parser(si, buf, token);

		if (err)
			break;
	}

	kfree(origin_str);

	return err;
}