obj-$(CONFIG_EULER_DYSCHE) += dysche_sysfs.o memory_layout.o \
					dysche_config.o dysche_instance.o \
					dysche_fdt.o dysche_args.o \
					dysche_host_memory_reserve.o \
					dysche_loader.o

ifneq ($(wildcard arch/$(SRCARCH)/*),)
	obj-$(CONFIG_EULER_DYSCHE) += arch/$(SRCARCH)/
endif