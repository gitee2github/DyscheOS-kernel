#include "dysche_partition.h"

int init_dysche_config(struct dysche_instance* ins)
{
	if (!ins->config)
		return -EINVAL;

	memset(ins->config, 0, sizeof(struct dysche_config));

	ins->config->magic_begin = DYSCHE_CONFIG_MAGIC_BEGIN;
	ins->config->magic_end = DYSCHE_CONFIG_MAGIC_END;
	ins->config->id = ins->slave_id;

	return 0;	
}