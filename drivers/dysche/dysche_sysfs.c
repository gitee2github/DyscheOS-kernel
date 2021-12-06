#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/kobject.h>
#include <linux/module.h>

#include "dysche_partition.h"

struct kobject *dysche_root;

static void dysche_root_release(struct kobject *root)
{
	kfree(root);
}

static ssize_t dysche_create_store(struct kobject *kobj,
				   struct kobj_attribute *attr, const char *buf,
				   size_t count)
{
	struct dysche_instance *ins;
	int ret;

	ret = si_create(buf, &ins);
	if (ret)
		return ret;

	ret = si_run(ins);
	if (ret) {
		si_destroy(ins);
		return ret;
	}

	return count;
}

static struct kobj_attribute dysche_create_attr =
	__ATTR(create, 0200, NULL, dysche_create_store);

static ssize_t dysche_cpec_show(struct kobject *k, struct kobj_attribute *attr,
				char *buf)
{
	phys_addr_t paddr = 0;
	size_t size = 0;

	//dysche_get_cross_partition_memory_info(&paddr, &size);

	return sprintf(buf, "0x%lx@0x%llx", size, paddr);
}

static struct kobj_attribute dysche_cpec_attr =
	__ATTR(cpec_mem, 0400, dysche_cpec_show, NULL);

static struct attribute *dysche_root_default_attrs[] = {
	&dysche_create_attr.attr,
	&dysche_cpec_attr.attr,
	NULL,
};

static struct kobj_type dysche_root_type = {
	.release = dysche_root_release,
	.sysfs_ops = &kobj_sysfs_ops,
	.default_attrs = dysche_root_default_attrs,
};

// Add root /sys/dysche
static __init int init_dysche_sysfs(void)
{
	int ret;
	dysche_root = kzalloc(sizeof(*dysche_root), GFP_KERNEL);
	if (!dysche_root)
		return -ENOMEM;

	ret = kobject_init_and_add(dysche_root, &dysche_root_type, NULL,
				   "dysche");
	if (ret)
		kfree(dysche_root);

	return ret;
}
module_init(init_dysche_sysfs);

static __exit void fini_dysche_sysfs(void)
{
	if (!dysche_root)
		return;

	kobject_del(dysche_root);
}
module_exit(fini_dysche_sysfs);

static ssize_t status_show(struct kobject *kobj, struct kobj_attribute *attr,
			   char *buf)
{
	//struct dysche_instance *ins = kobj_to_dysche(kobj);

	return sprintf(buf, "TODO: kobj status.");
}
static struct kobj_attribute status_attribute =
	__ATTR(status, 0444, status_show, NULL);

static ssize_t cpu_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	struct dysche_instance *ins = kobj_to_dysche(kobj);

	return sprintf(buf, "%*pbl", cpumask_pr_args(&ins->cpu_mask));
}
static struct kobj_attribute cpu_attribute = __ATTR(cpu, 0444, cpu_show, NULL);

static ssize_t desc_show(struct kobject *kobj, struct kobj_attribute *attr,
			 char *buf)
{
	struct dysche_instance *ins = kobj_to_dysche(kobj);
	int ret;

	si_lock(ins);
	ret = sprintf(buf, "%s\n\tName: %s\n\tID: %d\n\tcmdline: %s\n",
		      "Description", ins->slave_name, ins->slave_id,
		      ins->cmdline);
	si_unlock(ins);

	return ret;
}
static struct kobj_attribute desc_attribute = __ATTR_RO(desc);

static ssize_t heartbeat_show(struct kobject *kobj, struct kobj_attribute *attr,
			      char *buf)
{
	struct dysche_instance *ins = kobj_to_dysche(kobj);
	int ret = 0;

	si_lock(ins);
	//ret = sprintf(buf, "%d\n", ins->max_lost_cnt);
	si_unlock(ins);
	return ret;
}

static ssize_t heartbeat_store(struct kobject *kobj,
			       struct kobj_attribute *attr, const char *buf,
			       size_t count)
{
	struct dysche_instance *ins = kobj_to_dysche(kobj);
	int cnt;

	int ret = kstrtoint_from_user(buf, count, 10, &cnt);
	if (ret)
		return ret;

	si_lock(ins);
	// TODO: set heartbeat
	si_unlock(ins);

	return count;
}
static struct kobj_attribute heartbeat_attribute =
	__ATTR(heartbeat, 0600, heartbeat_show, heartbeat_store);

static ssize_t partep_store(struct kobject *kobj, struct kobj_attribute *attr,
			    const char *buf, size_t count)
{
	struct dysche_instance *ins = kobj_to_dysche(kobj);
	bool enable;

	int ret = kstrtobool_from_user(buf, count, &enable);
	if (ret)
		return ret;

	si_lock(ins);
	/*if (enable)
		slave_partep_create(ins);
	else
		slave_partep_remove(ins);
*/
	si_unlock(ins);

	return count;
}
static struct kobj_attribute partep_attribute = __ATTR_WO(partep);

/*
 * /sys/fs/dysche/X/restart_policy
 * This command is used to restart a dysche due to a fault
 * or proactively restart the dysche.
 * only integer is permitted as input:
 * [...:-1]: invalid input arguments
 * -1 : do not restart
 * 0 : unlimited restart times
 * [1:...]: restart specified times
 */
static ssize_t restart_store(struct kobject *kobj, struct kobj_attribute *attr,
			     const char *buf, size_t count)
{
	return count;
}

static ssize_t restart_show(struct kobject *kobj, struct kobj_attribute *attr,
			    char *buf)
{
	return sprintf(buf, "TODO: restart show.");
}
static struct kobj_attribute restart_attribute = __ATTR_RW(restart);

/* /sys/fs/dysche/X/cmdline */
static ssize_t cmdline_show(struct kobject *kobj, struct kobj_attribute *attr,
			    char *buf)
{
	return sprintf(buf, "TODO: get cmdline.");
}
static struct kobj_attribute cmdline_attribute =
	__ATTR(cmdline, 0400, cmdline_show, NULL);

static struct attribute *partition_default_attrs[] = {
	&status_attribute.attr,	   &cpu_attribute.attr,
	&heartbeat_attribute.attr, &partep_attribute.attr,
	&restart_attribute.attr,   &cmdline_attribute.attr,
	&desc_attribute.attr,	   NULL
};

static struct kobj_type partition_type = {
	.sysfs_ops = &kobj_sysfs_ops,
	.default_attrs = partition_default_attrs,
};

int init_partition_sysfs(struct dysche_instance *part)
{
	int ret;
	if (!dysche_root)
		return -ENODEV;

	ret = kobject_init_and_add(&part->dysche_kobj, &partition_type,
				   dysche_root, "%s", part->slave_name);

	return ret;
}

void fini_partition_sysfs(struct dysche_instance *ins)
{
	kobject_del(&ins->dysche_kobj);
}