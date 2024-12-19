/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Logging (to memory) of calls from an EFI app
 *
 * Copyright 2024 Google LLC
 * Written by Simon Glass <sjg@chromium.org>
 */

#ifndef __EFI_LOG_H
#define __EFI_LOG_H

#include <linux/types.h>
#include <efi.h>

/**
 * enum efil_tag - Types of logging records which can be created
 */
enum efil_tag {
	EFILT_ALLOCATE_PAGES,
	EFILT_FREE_PAGES,
	EFILT_ALLOCATE_POOL,
	EFILT_FREE_POOL,

	EFILT_TESTING,

	EFILT_COUNT,
};

/**
 * struct efil_rec_hdr - Header for each logging record
 *
 * @tag: Tag which indicates the type of the record
 * @size: Size of the record in bytes
 * @ended: true if record has been completed (i.e. the function returned), false
 *	if it is still pending
 * @e_ret: Records the return function from the logged function
 */
struct efil_rec_hdr {
	enum efil_tag tag;
	int size;
	bool ended;
	efi_status_t e_ret;
};

/**
 * struct efil_hdr - Holds the header for the log
 *
 * @upto: Offset at which to store the next log record
 * @size: Total size of the log in bytes
 */
struct efil_hdr {
	int upto;
	int size;
};

enum efil_test_t {
	EFI_LOG_TEST0,
	EFI_LOG_TEST1,

	EFI_LOG_TEST_COUNT,
};

/**
 * struct efil_testing - used for testing the log
 */
struct efil_testing {
	enum efil_test_t enum_val;
	efi_uintn_t int_val;
	u64 *memory;
	void **buffer;
	u64 e_memory;
	void *e_buffer;
};

/**
 * struct efil_allocate_pages - holds info from efi_allocate_pages() call
 *
 * @e_memory: Contains the value of *@memory on return from the EFI function
 */
struct efil_allocate_pages {
	enum efi_allocate_type type;
	enum efi_memory_type memory_type;
	efi_uintn_t pages;
	u64 *memory;
	u64 e_memory;
};

/** struct efil_free_pages - holds info from efi_free_pages() call */
struct efil_free_pages {
	u64 memory;
	efi_uintn_t pages;
};

/** struct efil_allocate_pool - holds info from efi_allocate_pool() call
 *
 * @e_buffer: Contains the value of *@buffer on return from the EFI function
 */
struct efil_allocate_pool {
	enum efi_memory_type pool_type;
	efi_uintn_t size;
	void **buffer;
	void *e_buffer;
};

/** struct efil_free_pool - holds log-info from efi_free_pool() call */
struct efil_free_pool {
	void *buffer;
};

/*
 * The functions below are in pairs, with a 'start' and 'end' call for each EFI
 * function. The 'start' function (efi_logs_...) is called when the function is
 * started. It records all the arguments. The 'end' function (efi_loge_...) is
 * called when the function is ready to return. It records any output arguments
 * as well as the return value.
 *
 * The start function returns the offset of the log record. This must be passed
 * to the end function, so it can add the status code and any other useful
 * information. It is not possible for the end functions to remember the offset
 * from the associated start function, since EFI functions may be called in a
 * nested way and there is no obvious way to determine the log record to which
 * the end function refers.
 *
 * If the start function returns an error code (i.e. an offset < 0) then it is
 * safe to pass that to the end function. It will simply ignore the operation.
 * Common errors are -ENOENT if there is no log and -ENOSPC if the log is full
 */

#if CONFIG_IS_ENABLED(EFI_LOG)

/**
 * efi_logs_testing() - Record a test call to an efi function
 *
 * @enum_val:	enum value
 * @int_val:	integer value
 * @buffer:	place to write pointer address
 * @memory:	place to write memory address
 * Return:	log-offset of this new record, or -ve error code
 */
int efi_logs_testing(enum efil_test_t enum_val, efi_uintn_t int_value,
		     void *buffer, u64 *memory);

/**
 * efi_loge_testing() - Record a return from a test call
 *
 * This stores the value of the pointers also
 *
 * ofs: Offset of the record to end
 * efi_ret: status code to record
 */
int efi_loge_testing(int ofs, efi_status_t efi_ret);

/**
 * efi_logs_allocate_pages() - Record a call to efi_allocate_pages()
 *
 * @type:		type of allocation to be performed
 * @memory_type:	usage type of the allocated memory
 * @pages:		number of pages to be allocated
 * @memory:		place to write address of allocated memory
 * Return:		log-offset of this new record, or -ve error code
 */
int efi_logs_allocate_pages(enum efi_allocate_type type,
			    enum efi_memory_type memory_type, efi_uintn_t pages,
			    u64 *memory);

/**
 * efi_loge_allocate_pages() - Record a return from efi_allocate_pages()
 *
 * This stores the value of the memory pointer also
 *
 * ofs: Offset of the record to end
 * efi_ret: status code to record
 */
int efi_loge_allocate_pages(int ofs, efi_status_t efi_ret);

/**
 * efi_logs_free_pages() - Record a call to efi_free_pages()
 *
 * @memory:	start of the memory area to be freed
 * @pages:	number of pages to be freed
 * Return:	log-offset of this new record, or -ve error code
 */
int efi_logs_free_pages(u64 memory, efi_uintn_t pages);

/**
 * efi_loge_free_pages() - Record a return from efi_free_pages()
 *
 * ofs: Offset of the record to end
 * efi_ret: status code to record
 */
int efi_loge_free_pages(int ofs, efi_status_t efi_ret);

/**
 * efi_logs_allocate_pool() - Record a call to efi_allocate_pool()
 *
 * @pool_type:	type of the pool from which memory is to be allocated
 * @size:	number of bytes to be allocated
 * @buffer:	place to hold pointer to allocated memory
 * Return:	log-offset of this new record, or -ve error code
 */
int efi_logs_allocate_pool(enum efi_memory_type pool_type, efi_uintn_t size,
			   void **buffer);

/**
 * efi_loge_allocate_pool() - Record a return from efi_allocate_pool()
 *
 * This stores the value of the buffer pointer also
 *
 * ofs: Offset of the record to end
 * efi_ret: status code to record
 */
int efi_loge_allocate_pool(int ofs, efi_status_t efi_ret);

/**
 * efi_logs_free_pool() - Record a call to efi_free_pool()
 *
 * @buffer:	start of memory to be freed
 * Return:	log-offset of this new record, or -ve error code
 */
int efi_logs_free_pool(void *buffer);

/**
 * efi_loge_free_pool() - Record a return from efi_free_pool()
 *
 * ofs: Offset of the record to end
 * efi_ret: status code to record
 */
int efi_loge_free_pool(int ofs, efi_status_t efi_ret);

#else /* !EFI_LOG */

static inline int efi_logs_allocate_pages(enum efi_allocate_type type,
					  enum efi_memory_type memory_type,
					  efi_uintn_t pages, u64 *memory)
{
	return -ENOSYS;
}

static inline int efi_loge_allocate_pages(int ofs, efi_status_t efi_ret)
{
	return -ENOSYS;
}

static inline int efi_logs_free_pages(u64 memory, efi_uintn_t pages)
{
	return -ENOSYS;
}

static inline int efi_loge_free_pages(int ofs, efi_status_t efi_ret)
{
	return -ENOSYS;
}

static inline int efi_logs_allocate_pool(enum efi_memory_type pool_type,
					 efi_uintn_t size, void **buffer)
{
	return -ENOSYS;
}

static inline int efi_loge_allocate_pool(int ofs, efi_status_t efi_ret)
{
	return -ENOSYS;
}

static inline int efi_logs_free_pool(void *buffer)
{
	return -ENOSYS;
}

static inline int efi_loge_free_pool(int ofs, efi_status_t efi_ret)
{
	return -ENOSYS;
}

static inline int efi_logs_testing(enum efil_test_t enum_val,
				   efi_uintn_t int_value, void *buffer,
				   u64 *memory)
{
	return -ENOSYS;
}

static inline int efi_loge_testing(int ofs, efi_status_t efi_ret)
{
	return -ENOSYS;
}

#endif /* EFI_LOG */

/* below are some general functions */

/**
 * efi_log_show() - Show the EFI log
 *
 * Displays the log of EFI boot-services calls which are so-far enabled for
 * logging
 *
 * Return: 0 on success, or -ve error code
 */
int efi_log_show(void);

/**
 * efi_log_reset() - Reset the log, erasing all records
 *
 * Return 0 if OK, -ENOENT if the log could not be found

 */
int efi_log_reset(void);

/**
 * efi_log_init() - Create a log in the bloblist, then reset it
 *
 * Return 0 if OK, -ENOMEM if the bloblist is not large enough
 */
int efi_log_init(void);

#endif /* __EFI_LOG_H */
