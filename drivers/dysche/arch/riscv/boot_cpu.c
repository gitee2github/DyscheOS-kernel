#include <asm/sbi.h>
#include <asm/smp.h>

int dysche_boot_cpu(int cpu, unsigned long addr)
{
	struct sbiret ret;
	unsigned long hartid = cpuid_to_hartid_map(cpu);

	ret = sbi_ecall(SBI_EXT_HSM, SBI_EXT_HSM_HART_START, hartid, addr, 0, 0,
			0, 0);

	if (ret.error)
		return sbi_err_map_linux_errno(ret.error);

	return 0;
}