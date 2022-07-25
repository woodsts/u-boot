// SPDX-License-Identifier: GPL-2.0+
/*
 * A general-purpose cyclic execution infrastructure, to allow "small"
 * (run-time wise) functions to be executed at a specified frequency.
 * Things like LED blinking or watchdog triggering are examples for such
 * tasks.
 *
 * Copyright (C) 2022 Stefan Roese <sr@denx.de>
 */

#include <cyclic.h>
#include <log.h>
#include <linker_lists.h>
#include <malloc.h>
#include <time.h>
#include <linux/errno.h>
#include <linux/list.h>

struct list_head cyclic_list;
static bool cyclic_ready;
static bool cyclic_running;

struct cyclic_struct *cyclic_register(cyclic_func_t func, uint64_t delay_us,
				      const char *name, void *ctx)
{
	struct cyclic_struct *cyclic;

	if (!cyclic_ready) {
		pr_debug("Cyclic IF not ready yet\n");
		return NULL;
	}

	cyclic = calloc(1, sizeof(struct cyclic_struct));
	if (!cyclic) {
		pr_debug("Memory allocation error\n");
		return NULL;
	}

	/* Store values in struct */
	cyclic->func = func;
	cyclic->ctx = ctx;
	cyclic->name = strdup(name);
	cyclic->delay_us = delay_us;
	cyclic->start_time_us = timer_get_us();
	list_add_tail(&cyclic->list, &cyclic_list);

	return cyclic;
}

int cyclic_unregister(struct cyclic_struct *cyclic)
{
	list_del(&cyclic->list);
	free(cyclic);

	return 0;
}

void cyclic_run(void)
{
	struct cyclic_struct *cyclic, *tmp;
	uint64_t now, cpu_time;

	/* Prevent recursion */
	if (cyclic_running)
		return;

	cyclic_running = true;
	list_for_each_entry_safe(cyclic, tmp, &cyclic_list, list) {
		/*
		 * Check if this cyclic function needs to get called, e.g.
		 * do not call the cyclic func too often
		 */
		now = timer_get_us();
		if (time_after_eq64(now, cyclic->next_call)) {
			/* Call cyclic function and account it's cpu-time */
			cyclic->next_call = now + cyclic->delay_us;
			cyclic->func(cyclic->ctx);
			cyclic->run_cnt++;
			cpu_time = timer_get_us() - now;
			cyclic->cpu_time_us += cpu_time;

			/* Check if cpu-time exceeds max allowed time */
			if (cpu_time > CONFIG_CYCLIC_MAX_CPU_TIME_US) {
				pr_err("cyclic function %s took too long: %lldus vs %dus max, disabling\n",
				       cyclic->name, cpu_time,
				       CONFIG_CYCLIC_MAX_CPU_TIME_US);

				/* Unregister this cyclic function */
				cyclic_unregister(cyclic);
			}
		}
	}
	cyclic_running = false;
}

int cyclic_uninit(void)
{
	struct cyclic_struct *cyclic, *tmp;

	list_for_each_entry_safe(cyclic, tmp, &cyclic_list, list)
		cyclic_unregister(cyclic);

	return 0;
}

int cyclic_init(void)
{
	INIT_LIST_HEAD(&cyclic_list);
	cyclic_ready = true;

	return 0;
}
