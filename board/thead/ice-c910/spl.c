/*
* Copyright (C) 2017-2020 Alibaba Group Holding Limited
*
* SPDX-License-Identifier: GPL-2.0+
*/

#include <linux/types.h>
#include <common.h>
#include <console.h>
#include <asm/csr.h>
#include <asm/io.h>
#include <spl.h>
#include <asm/spl.h>

extern char init_ddr[];
extern void cpu_clk_config(int cpu_freq);
extern void ddr_clk_config(int ddr_freq);
extern void show_sys_clk(void);

void setup_dev_pmp(void)
{
	printf("%s: %s\n", __FILE__, __func__);

	/* peripherals: 0x3 f000 0000 ~ 0x4 0000 0000 napot rw */
	csr_write(pmpaddr2, 0x3f0000000 >> 2 | ((0x10000000 - 1) >> 3));
	csr_write(pmpcfg0, 0x88980000001b1f1d);
}

void setup_ddr_pmp(void)
{
	printf("%s: %s\n", __FILE__, __func__);

	/* ddr: 0x0 00000000 ~ 0x1 00000000 */
	csr_write(pmpaddr0, 0x0 >> 2 | ((0x100000000 - 1) >> 3));

	/* Leave pmpaddr1 for SDRAM on which this code is running */
	/* Plus, this was set in bootrom */

	csr_write(pmpaddr3, 0x0);
	csr_write(pmpaddr4, 0x0);
	csr_write(pmpaddr5, 0x0);

	//csr_write(pmpcfg0, 0x88980000001b1f1f);

	// ICE-RVB Board, otherwise, it got stuck in pmp config of opensbi
	csr_write(pmpcfg0, 0x88980000009b9f9f);

	// pmpcfg物理内存保护设置寄存器：
	// | 7  |6 5|4 3| 2 | 1 | 0 |
	// |Lock| 0 | A | X | W | R |
	// pmpcfg.L = 1, 锁住表项，无法对pmpaddr0和pmpcfg0进行修改, 在HART核重置之前将保持锁定!
	//
	// RISC-V 规定PMPADDR地址寄存器存放的是物理地址的bit[39:2] (右边移2位);
	// C910 pmpaddr 有效地址位: bit[37:9], 代表的物理地址bit[39:11]
	//
	// 当 pmpcfg.A = NAPOT 时：
	// aa_aaaa_aaaa_aaaa_aaaa_aaaa_aaaa_aa01_1111_1111, NAPOT, 4KB, #pmpaddr设置 => (start_addr >> 2) | (0xfff >> (2+1))
	// aa_aaaa_aaa0_1111_1111_1111_1111_1111_1111_1111, NAPOT, 2G
	//
	// 4KB保护区大小是 2^{2+1+9} = 4KB, 物理地址范围:
	// aa_aaaa_aaaa_aaaa_aaaa_aaaa_aaaa_aa00_0000_0000_00 ~
	// aa_aaaa_aaaa_aaaa_aaaa_aaaa_aaaa_aa11_1111_1111_11
	//
	// 2G保护区大小是 2^{2+1+28} = 2G,  物理地址范围:
	// aa_aaaa_aaa0_0000_0000_0000_0000_0000_0000_0000_00 ~
	// aa_aaaa_aaa0_1111_1111_1111_1111_1111_1111_1111_11

}

void cpu_performance_enable(void)
{
	csr_write(CSR_MCOR, 0x70013);
	csr_write(CSR_MCCR2, 0xe0410009);
	csr_write(CSR_MHCR, 0x11ff);
	csr_write(CSR_MXSTATUS, 0x638000);
	csr_write(CSR_MHINT, 0x16e30c);
}

void board_init_f(ulong dummy)
{
	int ddr_freq = 1600;
	int cpu_freq = 1200;
	int ret;
	void (*ddr_initialize)(void);

	setup_dev_pmp();
	cpu_clk_config(cpu_freq);

	ret = spl_early_init();
	if (ret) {
		printf("spl_early_init() failed: %d\n", ret);
		hang();
	}
	arch_cpu_init_dm();
	preloader_console_init();

	ddr_clk_config(ddr_freq);
	ddr_initialize = (void (*)(void))(init_ddr);
	ddr_initialize();
	show_sys_clk();
	setup_ddr_pmp();

	printf("%s: %s\n", __FILE__, __func__);
}

void board_boot_order(u32 *spl_boot_list)
{
	if ((readl((void *)0x3fff72050) & 0x3) == 3) {
		debug("Wait here for JTAG/GDB connecting\n");
		asm volatile ("ebreak");
	} else {
		spl_boot_list[0] = BOOT_DEVICE_MMC1;
		cpu_performance_enable();
	}
}
