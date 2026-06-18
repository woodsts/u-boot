/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Bit definitions and masks for GPT attributes for Chromium OS.
 *
 *  63-61  -- (reserved)
 *     60  -- read-only
 *  59-58  -- (reserved)
 *     57  -- error counter
 *     56  -- success
 *  55-52  -- tries
 *  51-48  -- priority
 *   47-3  -- UEFI: reserved for future use
 *      2  -- UEFI: Legacy BIOS bootable
 *      1  -- UEFI: partition is not mapped
 *      0  -- UEFI: partition is required
 *
 * Reference:
 *   https://chromium.googlesource.com/chromiumos/platform/vboot_reference/+/master/cgpt
 */

#ifndef _CROS_FLAGS_H
#define _CROS_FLAGS_H

#define CROS_FLAG_PRIORITY_MASK      0x000fU
#define CROS_FLAG_TRIES_MASK         0x00f0U
#define CROS_FLAG_SUCCESSFUL_MASK    0x0100U

#define CROS_FLAG_PRIORITY_SHIFT     0U
#define CROS_FLAG_TRIES_SHIFT        4U
#define CROS_FLAG_SUCCESSFUL_SHIFT   8U

static inline unsigned int
cros_flags_get_priority(unsigned int flags)
{
	return (flags & CROS_FLAG_PRIORITY_MASK) >> CROS_FLAG_PRIORITY_SHIFT;
}

static inline unsigned int
cros_flags_set_priority(unsigned int flags, unsigned int priority)
{
	flags &= ~CROS_FLAG_PRIORITY_MASK;
	flags |= (priority & 0xf) << CROS_FLAG_PRIORITY_SHIFT;
	return flags;
}

static inline unsigned int
cros_flags_get_tries(unsigned int flags)
{
	return (flags & CROS_FLAG_TRIES_MASK) >> CROS_FLAG_TRIES_SHIFT;
}

static inline unsigned int
cros_flags_set_tries(unsigned int flags, unsigned int tries)
{
	flags &= ~CROS_FLAG_TRIES_MASK;
	flags |= (tries & 0xf) << CROS_FLAG_TRIES_SHIFT;
	return flags;
}

static inline bool
cros_flags_get_successful(unsigned int flags)
{
	return (flags & CROS_FLAG_SUCCESSFUL_MASK) >> CROS_FLAG_SUCCESSFUL_SHIFT;
}

static inline unsigned int
cros_flags_set_successful(unsigned int flags, bool ok)
{
	flags &= ~CROS_FLAG_SUCCESSFUL_MASK;
	flags |= ok << CROS_FLAG_SUCCESSFUL_SHIFT;
	return flags;
}

#endif /* _CROS_FLAGS_H */
