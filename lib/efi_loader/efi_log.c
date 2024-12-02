// SPDX-License-Identifier: GPL-2.0+
/*
 * Logging (to memory) of calls from an EFI app
 *
 * Copyright 2024 Google LLC
 * Written by Simon Glass <sjg@chromium.org>
 */

#define LOG_CATEGORY LOGC_EFI

#include <bloblist.h>
#include <efi_log.h>
#include <errno.h>
#include <log.h>

/* names for enum efil_tag (abbreviated to keep output to a single line) */
static const char *tag_name[EFILT_COUNT] = {
	"testing",
};

/* names for error codes, trying to keep them short */
static const char *error_name[EFI_ERROR_COUNT] = {
	"OK",
	"load",
	"inval_param",
	"unsupported",
	"bad_buf_sz",
	"buf_small",
	"not_ready",
	"device",
	"write_prot",
	"out_of_rsrc",
	"vol_corrupt",
	"vol_full",
	"no_media",
	"media_chg",
	"not_found",
	"no access",
	"no_response",
	"no_mapping",
	"timeout",
	"not_started",
	"already",
	"aborted",
	"icmp",
	"tftp",
	"protocol",
	"bad version",
	"sec_violate",
	"crc_error",
	"end_media",
	"end_file",
	"inval_lang",
	"compromised",
	"ipaddr_busy",
	"http",
};

static const char *test_enum_name[EFI_LOG_TEST_COUNT] = {
	"test0",
	"test1",
};

/**
 * prep_rec() - prepare a new record in the log
 *
 * This creates a new record at the next available position, setting it up ready
 * to hold data. The size and tag are set up.
 *
 * The log is updated so that the next record will start after this one
 *
 * @tag: tag of the EFI call to record
 * @size: Number of bytes in the caller's struct
 * @recp: Set to point to where the caller should add its data
 * Return: Offset of this record (must be passed to finish_rec())
 */
static int prep_rec(enum efil_tag tag, uint str_size, void **recp)
{
	struct efil_hdr *hdr = bloblist_find(BLOBLISTT_EFI_LOG, 0);
	struct efil_rec_hdr *rec_hdr;
	int ofs, size;

	if (!hdr)
		return -ENOENT;
	size = str_size + sizeof(struct efil_rec_hdr);
	if (hdr->upto + size > hdr->size)
		return -ENOSPC;

	rec_hdr = (void *)hdr + hdr->upto;
	rec_hdr->size = size;
	rec_hdr->tag = tag;
	rec_hdr->ended = false;
	*recp = rec_hdr + 1;

	ofs = hdr->upto;
	hdr->upto += size;

	return ofs;
}

/**
 * finish_rec() - Finish a previously started record
 *
 * @ofs: Offset of record to finish
 * @ret: Return code which is to be returned from the EFI function
 * Return: Pointer to the structure where the caller should add its data
 */
static void *finish_rec(int ofs, efi_status_t ret)
{
	struct efil_hdr *hdr = bloblist_find(BLOBLISTT_EFI_LOG, 0);
	struct efil_rec_hdr *rec_hdr;

	if (!hdr || ofs < 0)
		return NULL;
	rec_hdr = (void *)hdr + ofs;
	rec_hdr->ended = true;
	rec_hdr->e_ret = ret;

	return rec_hdr + 1;
}

int efi_logs_testing(enum efil_test_t enum_val, efi_uintn_t int_val,
		     void *buffer, u64 *memory)
{
	struct efil_testing *rec;
	int ret;

	ret = prep_rec(EFILT_TESTING, sizeof(*rec), (void **)&rec);
	if (ret < 0)
		return ret;

	rec->enum_val = enum_val;
	rec->int_val = int_val;
	rec->buffer = buffer;
	rec->memory = memory;
	rec->e_buffer = NULL;
	rec->e_memory = 0;

	return ret;
}

int efi_loge_testing(int ofs, efi_status_t efi_ret)
{
	struct efil_testing *rec;

	rec = finish_rec(ofs, efi_ret);
	if (!rec)
		return -ENOSPC;
	rec->e_memory = *rec->memory;
	rec->e_buffer = *rec->buffer;

	return 0;
}

static void show_enum(const char *type_name[], int type)
{
	printf("%s ", type_name[type]);
}

static void show_ulong(const char *prompt, ulong val)
{
	printf("%s %lx", prompt, val);
	if (val >= 10)
		printf("/%ld", val);
	printf(" ");
}

static void show_addr(const char *prompt, ulong addr)
{
	printf("%s %lx ", prompt, addr);
}

static void show_ret(efi_status_t ret)
{
	int code;

	code = ret & ~EFI_ERROR_MASK;
	if (code < ARRAY_SIZE(error_name))
		printf("ret %s", error_name[ret]);
	else
		printf("ret %lx", ret);
}

void show_rec(int seq, struct efil_rec_hdr *rec_hdr)
{
	void *start = (void *)rec_hdr + sizeof(struct efil_rec_hdr);

	printf("%3d %12s ", seq, tag_name[rec_hdr->tag]);
	switch (rec_hdr->tag) {
	case EFILT_TESTING: {
		struct efil_testing *rec = start;

		show_enum(test_enum_name, (int)rec->enum_val);
		show_ulong("int", (ulong)rec->int_val);
		show_addr("buf", map_to_sysmem(rec->buffer));
		show_addr("mem", map_to_sysmem(rec->memory));
		if (rec_hdr->ended) {
			show_addr("*buf", (ulong)map_to_sysmem(rec->e_buffer));
			show_addr("*mem", (ulong)rec->e_memory);
			show_ret(rec_hdr->e_ret);
		}
	}
	case EFILT_COUNT:
		break;
	}
	printf("\n");
}

int efi_log_show(void)
{
	struct efil_hdr *hdr = bloblist_find(BLOBLISTT_EFI_LOG, 0);
	struct efil_rec_hdr *rec_hdr;
	int i;

	printf("EFI log (size %x)\n", hdr->upto);
	if (!hdr)
		return -ENOENT;
	for (i = 0, rec_hdr = (void *)hdr + sizeof(*hdr);
	     (void *)rec_hdr - (void *)hdr < hdr->upto;
	     i++, rec_hdr = (void *)rec_hdr + rec_hdr->size)
		show_rec(i, rec_hdr);
	printf("%d records\n", i);

	return 0;
}

int efi_log_reset(void)
{
	struct efil_hdr *hdr = bloblist_find(BLOBLISTT_EFI_LOG, 0);

	if (!hdr)
		return -ENOENT;
	hdr->upto = sizeof(struct efil_hdr);
	hdr->size = CONFIG_EFI_LOG_SIZE;

	return 0;
}

int efi_log_init(void)
{
	struct efil_hdr *hdr;

	hdr = bloblist_add(BLOBLISTT_EFI_LOG, CONFIG_EFI_LOG_SIZE, 0);
	if (!hdr) {
		/*
		 * Return -ENOMEM since we use -ENOSPC to mean that the log is
		 * full
		 */
		log_warning("Failed to setup EFI log\n");
		return log_msg_ret("eli", -ENOMEM);
	}
	efi_log_reset();

	return 0;
}
