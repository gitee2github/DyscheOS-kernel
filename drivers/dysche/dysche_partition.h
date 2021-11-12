#ifndef _DYSCHE_PARTITION_
#define _DYSCHE_PARTITION_

#include <linux/kernel.h>
#include <linux/mutex.h>

#define DYSCHE_NAME_LEN 64

enum dysche_status {
	SI_S_NONE,
	SI_S_CREATED,
	SI_S_BOOTING,
	SI_S_RUNNING,
	SI_S_SHUTTING_DOWN,
	SI_S_REBOOTING,
	SI_S_LOST,
	SI_S_INVALID,
};

enum OSType {
	LINUX_KERNEL,
	UNKNOWN_KERNEL,
};

struct dysche_instance {
	uint32_t slave_id;
	enum dysche_status status;
	enum OSType ostype;
	struct list_head entry;
	struct kobject dysche_kobj;

	char slave_name[DYSCHE_NAME_LEN];
	int flag;
};

static inline struct dysche_instance *kobj_to_dysche(struct kobject *kobj)
{
	return container_of(kobj, struct dysche_instance, dysche_kobj);
}

static inline void si_lock(struct dysche_instance *ins)
{
}
static inline void si_unlock(struct dysche_instance *ins)
{
}

static inline int si_create(const char *buf, struct dysche_instance **ins)
{
	return 0;
}
static inline int si_run(struct dysche_instance *ins)
{
	return 0;
}
static inline int si_destroy(struct dysche_instance *ins)
{
	return 0;
}

#endif