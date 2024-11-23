/* SPDX-License-Identifier: GPL-2.0+ */
#ifdef CONFIG_OF_SEPARATE
.section .embedded_dtb, "a"
.globl __dtb
__dtb: .fill 1024*1024
#endif
