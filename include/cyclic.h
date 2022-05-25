/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * A general-purpose cyclic execution infrastructure, to allow "small"
 * (run-time wise) functions to be executed at a specified frequency.
 * Things like LED blinking or watchdog triggering are examples for such
 * tasks.
 *
 * Copyright (C) 2022 Stefan Roese <sr@denx.de>
 */

#ifndef __cyclic_h
#define __cyclic_h

#include <linux/list.h>
#include <asm/types.h>

struct cyclic_struct {
	void (*func)(void *ctx);
	void *ctx;
	char *name;
	uint64_t delay_us;
	uint64_t start_time_us;
	uint64_t cpu_time_us;
	uint64_t run_cnt;
	uint64_t next_call;
	struct list_head list;
};

/** Function type for cyclic functions */
typedef void (*cyclic_func_t)(void *ctx);

/**
 * cyclic_register - Register a new cyclic function
 *
 * @func: Function to call periodically
 * @delay_us: Delay is us after which this function shall get executed
 * @name: Cyclic function name/id
 * @ctx: Context to pass to the function
 * @return: pointer to cyclic_struct if OK, NULL on error
 */
struct cyclic_struct *cyclic_register(cyclic_func_t func, uint64_t delay_us,
				      const char *name, void *ctx);

/**
 * cyclic_unregister - Unregister a cyclic function
 *
 * @cyclic: Pointer to cyclic_struct of the function that shall be removed
 * @return: 0 if OK, -ve on error
 */
int cyclic_unregister(struct cyclic_struct *cyclic);

#if defined(CONFIG_CYCLIC)
/**
 * cyclic_init() - Set up cyclic functions
 *
 * Init a list of cyclic functions, so that these can be added as needed
 */
int cyclic_init(void);

/**
 * cyclic_uninit() - Clean up cyclic functions
 *
 * This removes all cyclic functions
 */
int cyclic_uninit(void);

void cyclic_run(void);
#else
static inline void cyclic_run(void)
{
}

static inline int cyclic_init(void)
{
	return 0;
}

static inline int cyclic_uninit(void)
{
	return 0;
}
#endif

#endif
