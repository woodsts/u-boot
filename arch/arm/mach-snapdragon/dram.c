// SPDX-License-Identifier: GPL-2.0+
/*
 * Memory layout parsing for Qualcomm.
 */

#define LOG_CATEGORY LOGC_BOARD
#define pr_fmt(fmt) "QCOM-DRAM: " fmt

#include <asm-generic/unaligned.h>
#include <dm.h>
#include <log.h>
#include <sort.h>

static struct {
	phys_addr_t start;
	phys_size_t size;
} prevbl_ddr_banks[CONFIG_NR_DRAM_BANKS] __section(".data") = { 0 };

int dram_init(void)
{
	/*
	 * gd->ram_base / ram_size have been setup already
	 * in qcom_parse_memory().
	 */
	return 0;
}

static int ddr_bank_cmp(const void *v1, const void *v2)
{
	const struct {
		phys_addr_t start;
		phys_size_t size;
	} *res1 = v1, *res2 = v2;

	if (!res1->size)
		return 1;
	if (!res2->size)
		return -1;

	return (res1->start >> 24) - (res2->start >> 24);
}

/* This has to be done post-relocation since gd->bd isn't preserved */
static void qcom_configure_bi_dram(void)
{
	int i;

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		gd->bd->bi_dram[i].start = prevbl_ddr_banks[i].start;
		gd->bd->bi_dram[i].size = prevbl_ddr_banks[i].size;
	}
}

int dram_init_banksize(void)
{
	qcom_configure_bi_dram();

	return 0;
}

static void qcom_parse_memory_dt(const fdt64_t *memory, int banks, phys_addr_t *ram_end)
{
	int i, j;

	if (banks > CONFIG_NR_DRAM_BANKS)
		log_err("Provided more memory banks than we can handle\n");

	for (i = 0, j = 0; i < banks * 2; i += 2, j++) {
		prevbl_ddr_banks[j].start = get_unaligned_be64(&memory[i]);
		prevbl_ddr_banks[j].size = get_unaligned_be64(&memory[i + 1]);
		/* SM8650 boards sometimes have empty regions! */
		if (!prevbl_ddr_banks[j].size) {
			j--;
			continue;
		}
		*ram_end = max(*ram_end, prevbl_ddr_banks[j].start + prevbl_ddr_banks[j].size);
	}
}

/* Parse the memory layout from the FDT. */
void qcom_parse_memory(void)
{
	ofnode node;
	const fdt64_t *memory;
	int memsize;
	phys_addr_t ram_end = 0;
	int banks;

	node = ofnode_path("/memory");
	if (!ofnode_valid(node)) {
		log_err("No memory node found in device tree!\n");
		return;
	}
	memory = ofnode_read_prop(node, "reg", &memsize);
	if (!memory) {
		log_err("No memory configuration was provided by the previous bootloader!\n");
		return;
	}

	banks = min(memsize / (2 * sizeof(u64)), (ulong)CONFIG_NR_DRAM_BANKS);

	if (memsize / sizeof(u64) > CONFIG_NR_DRAM_BANKS * 2)
		log_err("Provided more than the max of %d memory banks\n", CONFIG_NR_DRAM_BANKS);

	qcom_parse_memory_dt(memory, banks, &ram_end);

	debug("%d banks, ram_base = %#011lx, ram_size = %#011llx, ram_end = %#011llx\n",
	      banks, gd->ram_base, gd->ram_size, ram_end);
	/* Sort our RAM banks -_- */
	qsort(prevbl_ddr_banks, banks, sizeof(prevbl_ddr_banks[0]), ddr_bank_cmp);

	gd->ram_base = prevbl_ddr_banks[0].start;
	gd->ram_size = ram_end - gd->ram_base;
	debug("%d banks, ram_base = %#011lx, ram_size = %#011llx, ram_end = %#011llx\n",
	      banks, gd->ram_base, gd->ram_size, ram_end);
}
