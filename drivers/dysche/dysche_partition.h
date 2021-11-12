#ifndef _DYSCHE_PARTITION_
#define _DYSCHE_PARTITION_

#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/mutex.h>
#include <linux/dysche.h>

#define DYSCHE_NAME_LEN 64

#define SI_HOTSCALE_CMD_INVALID 0
#define SI_HOTSCALE_CPU_ADD 1
#define SI_HOTSCALE_CPU_DEL 2
#define SI_HOTSCALE_CPU_RET_INVALID 1

enum si_status_e {
	SI_S_NONE,
	SI_S_CREATED,
	SI_S_BOOTING,
	SI_S_RUNNING,
	SI_S_SHUTTING_DOWN,
	SI_S_REBOOTING,
	SI_S_LOST,
	SI_S_INVALID,
};

#define DYSCHE_CONFIG_MAGIC_BEGIN 0x20200806
#define DYSCHE_CONFIG_MAGIC_END 0x20211102
struct dysche_config {
	u32 magic_begin;

	//info from master to slave
	u32 id;

	// status changed
	enum si_status_e new_status; // only writen by slave

	// heartbeat
	struct {
		atomic_t slave_cnt; // only writen by slave
		atomic_t master_cnt; // only writen by master
	} monitor;

	// hotscale
	struct {
		atomic_t cmd; // cmd 1: add-cpu, 2: del-cpu
		atomic_t req; // req, 1: request.
		atomic_t resp; // resp, 1: response

		u64 cpu; // hotscale cpu mpidr
		int hotscale_ret; // hotscale cpu errno
	} hotscale;

	u32 magic_end;
};

enum OSType {
	LINUX_KERNEL,
	UNKNOWN_KERNEL,
};

#define DYSCHE_MAX_MEM_REGIONS 4

struct dysche_memory {
	phys_addr_t phys;
	size_t size;
};

struct dysche_instance {
	uint32_t slave_id;
	enum OSType ostype;
	struct list_head entry;
	struct kobject dysche_kobj;

	int dysche_mem_region_nr;
	struct dysche_memory mems[DYSCHE_MAX_MEM_REGIONS];

	struct dysche_config* config;
	char slave_name[DYSCHE_NAME_LEN];
	int flag;
};

enum dysche_memory_type {
	DYSCHE_T_SH_DYSCHE_CONFIG,
	DYSCHE_T_SH_VCONSOLE,
	DYSCHE_T_SH_PARTEP,

	DYSCHE_T_SLAVE_LOADER,
	DYSCHE_T_SLAVE_FDT,
	DYSCHE_T_SLAVE_KERNEL,
	DYSCHE_T_SLAVE_ROOTFS,
};

int init_memory_layout(struct dysche_instance*ins);
void fini_memory_layout(struct dysche_instance* ins);

// shared part of dysche memory
int init_dysche_config(struct dysche_instance *ins);
void fini_dysche_config(struct dysche_instance *ins);

// 
int init_dysche_memory(struct dysche_instance *ins);
void fini_dysche_memory(struct dysche_instance *ins);

// fill memory region
int fill_memory_region(struct dysche_instance *ins,
		       enum dysche_memory_type type, off_t offset,
		       const void *buf, size_t count);

static inline struct dysche_instance *kobj_to_dysche(struct kobject *kobj)
{
	return container_of(kobj, struct dysche_instance, dysche_kobj);
}

void si_lock(struct dysche_instance *ins);
void si_unlock(struct dysche_instance *ins);

int si_create(const char *buf, struct dysche_instance **ins);
int si_run(struct dysche_instance *ins);
int si_destroy(struct dysche_instance *ins);

#endif