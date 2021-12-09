#include <asm/csr.h>
#include <asm/asm.h>
#include <linux/linkage.h>

    .text

ENTRY(_dysche_loader_start)
    csrw CSR_IE, zero
    csrw CSR_IP, zero

    la a3, .dysche_park
    csrw CSR_TVEC, a3

    la a3, _dysche_loader_start
    la a1, _dysche_fdt_offset
    REG_L   a1, (a1)
    add a1, a1, a3

    la a2, _dysche_kernel_offset
    REG_L   a2, (a2)
    add a2, a2, a3
    jalr a2

.align 2
.dysche_park:
    wfi
    j .dysche_park

.align 2
    .globl _dysche_fdt_offset
_dysche_fdt_offset:
    RISCV_PTR  0

    .globl _dysche_kernel_offset
_dysche_kernel_offset:
    RISCV_PTR  0

ENTRY(_dysche_loader_end)
