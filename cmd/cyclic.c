// SPDX-License-Identifier: GPL-2.0+
/*
 * A general-purpose cyclic execution infrastructure, to allow "small"
 * (run-time wise) functions to be executed at a specified frequency.
 * Things like LED blinking or watchdog triggering are examples for such
 * tasks.
 *
 * Copyright (C) 2022 Stefan Roese <sr@denx.de>
 */

#include <common.h>
#include <command.h>
#include <cyclic.h>

extern struct list_head cyclic_list;

static int do_cyclic_list(struct cmd_tbl *cmdtp, int flag, int argc,
			  char *const argv[])
{
	struct cyclic_struct *cyclic, *tmp;
	uint64_t cnt, freq;

	list_for_each_entry_safe(cyclic, tmp, &cyclic_list, list) {
		cnt = cyclic->run_cnt * 1000000ULL * 100ULL;
		freq = cnt / (timer_get_us() - cyclic->start_time_us);
		printf("function: %s, cpu-time: %lld us, frequency: %lld.%02lld times/s\n",
		       cyclic->name, cyclic->cpu_time_us,
		       freq / 100, freq % 100);
	}

	return 0;
}

#ifdef CONFIG_SYS_LONGHELP
static char cyclic_help_text[] =
	"cyclic list   - list cyclic functions";
#endif

U_BOOT_CMD_WITH_SUBCMDS(cyclic, "Cyclic", cyclic_help_text,
	U_BOOT_SUBCMD_MKENT(list, 1, 1, do_cyclic_list));
