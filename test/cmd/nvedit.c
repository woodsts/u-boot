// SPDX-License-Identifier: GPL-2.0+
/*
 * Tests for env command
 *
 * Copyright (C) 2025 Christoph Niedermaier <cniedermaier@dh-electronics.com>
 */

#include <asm/global_data.h>
#include <command.h>
#include <env.h>
#include <env_internal.h>
#include <linux/errno.h>
#include <log.h>
#include <string.h>
#include <test/cmd.h>
#include <test/ut.h>

/* Declare a new nvedit test */
#define NVEDIT_TEST(_name, _flags)	UNIT_TEST(_name, _flags, nvedit)

static int nvedit_test_print(struct unit_test_state *uts)
{
	ut_assertok(run_command("env set .Testdotvar dotvalue", 0));
	ut_assertok(run_command("env set TestvarP1 valueP1", 0));
	ut_assertok(run_command("env set TestvarP2 valueP2", 0));
	ut_assert_console_end();

	ut_assertok(run_command("env print .Testdotvar TestvarP1 TestvarP2", 0));
	ut_assert_nextline(".Testdotvar=dotvalue");
	ut_assert_nextline("TestvarP1=valueP1");
	ut_assert_nextline("TestvarP2=valueP2");
	ut_assert_console_end();

	return 0;
}
NVEDIT_TEST(nvedit_test_print, UTF_CONSOLE);

#if CONFIG_IS_ENABLED(CMD_GREPENV)
static int nvedit_test_grep(struct unit_test_state *uts)
{
	ut_assertok(run_command("env set TestvarG1 valueG1", 0));
	ut_assertok(run_command("env set TestvarG2 valueG2", 0));
	ut_assertok(run_command("env set TestvarG3 valueG3", 0));
	ut_assertok(run_command("env set TestGREP '-e -n -v -b'", 0));
	ut_assert_console_end();

	/* Match for name */
	ut_assertok(run_command("env grep -n TestvarG1", 0));
	ut_assert_nextline("TestvarG1=valueG1");
	ut_assert_console_end();
	ut_assert(run_command("env grep -n valueG1", 0));
	ut_assert_console_end();

	/* Match for value */
	ut_assertok(run_command("env grep -v valueG2", 0));
	ut_assert_nextline("TestvarG2=valueG2");
	ut_assert_console_end();
	ut_assert(run_command("env grep -v TestvarG2", 0));
	ut_assert_console_end();

	/* Match for name and value */
	ut_assertok(run_command("env grep -b TestvarG3", 0));
	ut_assert_nextline("TestvarG3=valueG3");
	ut_assert_console_end();
	ut_assertok(run_command("env grep -b valueG3", 0));
	ut_assert_nextline("TestvarG3=valueG3");
	ut_assert_console_end();

	/* Match for name and value (without a parameter) */
	ut_assertok(run_command("env grep TestvarG3", 0));
	ut_assert_nextline("TestvarG3=valueG3");
	ut_assert_console_end();
	ut_assertok(run_command("env grep valueG3", 0));
	ut_assert_nextline("TestvarG3=valueG3");
	ut_assert_console_end();

#if CONFIG_IS_ENABLED(REGEX)
	ut_assertok(run_command("env grep -e TestvarG[13]", 0));
	ut_assert_nextline("TestvarG1=valueG1");
	ut_assert_nextline("TestvarG3=valueG3");
	ut_assert_console_end();
#endif

	/* Test '--' */
	ut_assertok(run_command("env grep -- '-e -n -v -b'", 0));
	ut_assert_nextline("TestGREP=-e -n -v -b");
	ut_assert_console_end();

	return 0;
}
NVEDIT_TEST(nvedit_test_grep, UTF_CONSOLE);
#endif /* CONFIG_IS_ENABLED(CMD_GREPENV) */

static int nvedit_test_default(struct unit_test_state *uts)
{
	ut_assertok(run_command("env set TestvarDEF1 valueDEF1", 0));
	ut_assertok(run_command("env set TestvarDEF2 valueDEF2", 0));
	ut_assert_console_end();

	ut_assertok(run_command("env default TestvarDEF1", 0));
	ut_assert_nextline("WARNING: 'TestvarDEF1' not in imported env, deleting it!");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarDEF1", 0));
	ut_assert_nextline("## Error: \"TestvarDEF1\" not defined");
	ut_assert_console_end();
	ut_asserteq(0, run_command("env print TestvarDEF2", 0));
	ut_assert_nextline("TestvarDEF2=valueDEF2");
	ut_assert_console_end();

	ut_assertok(run_command("env set TestvarDEF1 nextvalue", 0));
	ut_assert_console_end();
	ut_assertok(run_command("env default -k TestvarDEF1", 0));
	ut_assert_console_end();
	ut_asserteq(0, run_command("env print TestvarDEF1", 0));
	ut_assert_nextline("TestvarDEF1=nextvalue");
	ut_assert_console_end();
	ut_asserteq(0, run_command("env print TestvarDEF2", 0));
	ut_assert_nextline("TestvarDEF2=valueDEF2");
	ut_assert_console_end();

	ut_assertok(run_command("env default -k -a -f", 0));
	ut_assert_nextline("## Resetting to default environment");
	ut_assert_console_end();
	ut_asserteq(0, run_command("env print TestvarDEF1", 0));
	ut_assert_nextline("TestvarDEF1=nextvalue");
	ut_assert_console_end();
	ut_asserteq(0, run_command("env print TestvarDEF2", 0));
	ut_assert_nextline("TestvarDEF2=valueDEF2");
	ut_assert_console_end();

	return 0;
}
NVEDIT_TEST(nvedit_test_default, UTF_CONSOLE);

static int nvedit_test_delete(struct unit_test_state *uts)
{
	ut_assertok(run_command("env set TestvarDEL1 valueDEL1", 0));
	ut_assertok(run_command("env set TestvarDEL2 valueDEL2", 0));
	ut_assert_console_end();

	/* Delete one by one */
	ut_asserteq(0, run_command("env print TestvarDEL1", 0));
	ut_assert_nextline("TestvarDEL1=valueDEL1");
	ut_assert_console_end();
	ut_asserteq(0, run_command("env print TestvarDEL2", 0));
	ut_assert_nextline("TestvarDEL2=valueDEL2");
	ut_assert_console_end();
	ut_assertok(run_command("env delete TestvarDEL1", 0));
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarDEL1", 0));
	ut_assert_nextline("## Error: \"TestvarDEL1\" not defined");
	ut_assert_console_end();
	ut_asserteq(0, run_command("env print TestvarDEL2", 0));
	ut_assert_nextline("TestvarDEL2=valueDEL2");
	ut_assert_console_end();
	ut_assertok(run_command("env delete TestvarDEL2", 0));
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarDEL1", 0));
	ut_assert_nextline("## Error: \"TestvarDEL1\" not defined");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarDEL2", 0));
	ut_assert_nextline("## Error: \"TestvarDEL2\" not defined");
	ut_assert_console_end();

	ut_assertok(run_command("env set TestvarDEL1 valueDEL1", 0));
	ut_assertok(run_command("env set TestvarDEL2 valueDEL2", 0));
	ut_assert_console_end();

	/* Delete both together */
	ut_asserteq(0, run_command("env print TestvarDEL1", 0));
	ut_assert_nextline("TestvarDEL1=valueDEL1");
	ut_assert_console_end();
	ut_asserteq(0, run_command("env print TestvarDEL2", 0));
	ut_assert_nextline("TestvarDEL2=valueDEL2");
	ut_assert_console_end();
	ut_assertok(run_command("env delete TestvarDEL1 TestvarDEL2", 0));
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarDEL1", 0));
	ut_assert_nextline("## Error: \"TestvarDEL1\" not defined");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarDEL2", 0));
	ut_assert_nextline("## Error: \"TestvarDEL2\" not defined");
	ut_assert_console_end();

	return 0;
}
NVEDIT_TEST(nvedit_test_delete, UTF_CONSOLE);

#if CONFIG_IS_ENABLED(CMD_NVEDIT_INFO)
static int nvedit_test_info(struct unit_test_state *uts)
{
	int ret = 0;

	/* Without a parameter */
	ut_assertok(run_command("env info", 0));
	ut_assert_nextlinen("env_valid = ");
	ut_assert_nextlinen("env_ready = ");
	ut_assert_nextlinen("env_use_default = ");
	ut_assert_console_end();

	/* Parameter -q (quiet output) */
	ut_asserteq(0, run_command("env info -q", 0));
	ut_assert_console_end();

	/* Parameter -d (default environment is used) */
	ret = run_command("env info -d", 0);
	if (ret == 0)
		ut_assert_nextline("Default environment is used");
	else if (ret == 1)
		ut_assert_nextline("Environment was loaded from persistent storage");
	ut_assert_console_end();

	/* Parameter -dq (default environment is used | quiet output) */
	ret = run_command("env info -dq", 0);
	ut_asserteq(0, ret > 1);
	ut_assert_console_end();

	/* Parameter -p (environment can be persisted) */
	ret = run_command("env info -p", 0);
	if (ret == 0)
		ut_assert_nextline("Environment can be persisted");
	else if (ret == 1)
		ut_assert_nextline("Environment cannot be persisted");
	ut_assert_console_end();

	/* Parameter -pq (environment can be persisted | quiet output) */
	ret = run_command("env info -pq", 0);
	ut_asserteq(0, ret > 1);
	ut_assert_console_end();

	return 0;
}
NVEDIT_TEST(nvedit_test_info, UTF_CONSOLE);
#endif

#if CONFIG_IS_ENABLED(CMD_EXPORTENV)
static int nvedit_test_export(struct unit_test_state *uts)
{
	ut_assertok(run_command("env set TestvarE1 valueE1", 0));
	ut_assertok(run_command("env set TestvarE2 valueE2", 0));
	ut_assert_console_end();

	/* Test error (combined parameter) */
	ut_asserteq(1, run_command("env export -tb ${loadaddr} TestvarE1 TestvarE2", 0));
	ut_assert_nextline("## Error: export: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env export -tc ${loadaddr} TestvarE1 TestvarE2", 0));
	ut_assert_nextline("## Error: export: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env export -bc ${loadaddr} TestvarE1 TestvarE2", 0));
	ut_assert_nextline("## Error: export: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env export -tbc ${loadaddr} TestvarE1 TestvarE2", 0));
	ut_assert_nextline("## Error: export: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();

	/* Test error (sperate parameter) */
	ut_asserteq(1, run_command("env export -t -b ${loadaddr} TestvarE1 TestvarE2", 0));
	ut_assert_nextline("## Error: export: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env export -t -c ${loadaddr} TestvarE1 TestvarE2", 0));
	ut_assert_nextline("## Error: export: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env export -b -c ${loadaddr} TestvarE1 TestvarE2", 0));
	ut_assert_nextline("## Error: export: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env export -t -b -c ${loadaddr} TestvarE1 TestvarE2", 0));
	ut_assert_nextline("## Error: export: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();

	/* Text format (without parameter -t) */
	ut_assertok(run_command("env export ${loadaddr} TestvarE1 TestvarE2", 0));
	ut_assert_console_end();
	ut_assertok(run_command("md.b ${loadaddr} 25", 0));
	ut_assert_nextline("%08lx: 54 65 73 74 76 61 72 45 31 3d 76 61 6c 75 65 45  TestvarE1=valueE",
			   hextoul(env_get("loadaddr"), NULL) + 0x00);
	ut_assert_nextline("%08lx: 31 0a 54 65 73 74 76 61 72 45 32 3d 76 61 6c 75  1.TestvarE2=valu",
			   hextoul(env_get("loadaddr"), NULL) + 0x10);
	ut_assert_nextline("%08lx: 65 45 32 0a 00                                   eE2..",
			   hextoul(env_get("loadaddr"), NULL) + 0x20);
	ut_assert_console_end();
	ut_asserteq_str("25", env_get("filesize"));
	ut_assert_console_end();

	/* Text format */
	ut_assertok(run_command("env export -t ${loadaddr} TestvarE1 TestvarE2", 0));
	ut_assert_console_end();
	ut_assertok(run_command("md.b ${loadaddr} 25", 0));
	ut_assert_nextline("%08lx: 54 65 73 74 76 61 72 45 31 3d 76 61 6c 75 65 45  TestvarE1=valueE",
			   hextoul(env_get("loadaddr"), NULL) + 0x00);
	ut_assert_nextline("%08lx: 31 0a 54 65 73 74 76 61 72 45 32 3d 76 61 6c 75  1.TestvarE2=valu",
			   hextoul(env_get("loadaddr"), NULL) + 0x10);
	ut_assert_nextline("%08lx: 65 45 32 0a 00                                   eE2..",
			   hextoul(env_get("loadaddr"), NULL) + 0x20);
	ut_assert_console_end();
	ut_asserteq_str("25", env_get("filesize"));
	ut_assert_console_end();

	/* Text format (size too small) */
	ut_asserteq(1, run_command("env export -t -s 24 ${loadaddr} TestvarE1 TestvarE2", 0));
	ut_assert_nextline("Env export buffer too small: 36, but need 37");
	ut_assert_nextline("## Error: Cannot export environment: errno = 12");
	ut_assert_console_end();

	/* Text format (correct size) */
	ut_assertok(run_command("env export -t -s 25 ${loadaddr} TestvarE1 TestvarE2", 0));
	ut_assert_console_end();
	ut_assertok(run_command("md.b ${loadaddr} 25", 0));
	ut_assert_nextline("%08lx: 54 65 73 74 76 61 72 45 31 3d 76 61 6c 75 65 45  TestvarE1=valueE",
			   hextoul(env_get("loadaddr"), NULL) + 0x00);
	ut_assert_nextline("%08lx: 31 0a 54 65 73 74 76 61 72 45 32 3d 76 61 6c 75  1.TestvarE2=valu",
			   hextoul(env_get("loadaddr"), NULL) + 0x10);
	ut_assert_nextline("%08lx: 65 45 32 0a 00                                   eE2..",
			   hextoul(env_get("loadaddr"), NULL) + 0x20);
	ut_assert_console_end();
	ut_asserteq_str("25", env_get("filesize"));
	ut_assert_console_end();

	/* Text format (size too big => padded with '\0') */
	ut_assertok(run_command("env export -t -s 30 ${loadaddr} TestvarE1 TestvarE2", 0));
	ut_assert_console_end();
	ut_assertok(run_command("md.b ${loadaddr} 30", 0));
	ut_assert_nextline("%08lx: 54 65 73 74 76 61 72 45 31 3d 76 61 6c 75 65 45  TestvarE1=valueE",
			   hextoul(env_get("loadaddr"), NULL) + 0x00);
	ut_assert_nextline("%08lx: 31 0a 54 65 73 74 76 61 72 45 32 3d 76 61 6c 75  1.TestvarE2=valu",
			   hextoul(env_get("loadaddr"), NULL) + 0x10);
	ut_assert_nextline("%08lx: 65 45 32 0a 00 00 00 00 00 00 00 00 00 00 00 00  eE2.............",
			   hextoul(env_get("loadaddr"), NULL) + 0x20);
	ut_assert_console_end();
	ut_asserteq_str("30", env_get("filesize"));
	ut_assert_console_end();

	/* Binary format */
	ut_assertok(run_command("env export -b ${loadaddr} TestvarE1 TestvarE2", 0));
	ut_assert_console_end();
	ut_assertok(run_command("md.b ${loadaddr} 25", 0));
	ut_assert_nextline("%08lx: 54 65 73 74 76 61 72 45 31 3d 76 61 6c 75 65 45  TestvarE1=valueE",
			   hextoul(env_get("loadaddr"), NULL) + 0x00);
	ut_assert_nextline("%08lx: 31 00 54 65 73 74 76 61 72 45 32 3d 76 61 6c 75  1.TestvarE2=valu",
			   hextoul(env_get("loadaddr"), NULL) + 0x10);
	ut_assert_nextline("%08lx: 65 45 32 00 00                                   eE2..",
			   hextoul(env_get("loadaddr"), NULL) + 0x20);
	ut_assert_console_end();
	ut_assertok(run_command("echo ${filesize}", 0));
	ut_assert_nextline("%x", CONFIG_ENV_SIZE);
	ut_assert_console_end();

	/* Clear memory at loadaddr */
	ut_assertok(run_commandf("mw.b ${loadaddr} 0x00 %x", CONFIG_ENV_SIZE));
	ut_assert_console_end();

	/* Binary format with checksum */
	ut_assertok(run_command("env export -c ${loadaddr} TestvarE1 TestvarE2", 0));
	ut_assert_console_end();
#if CONFIG_IS_ENABLED(SYS_REDUNDAND_ENVIRONMENT)
	ut_assertok(run_command("md.b ${loadaddr} 2a", 0));
#if CONFIG_IS_ENABLED(ENV_ADDR_REDUND)
	ut_assert_nextline("%08lx: b0 8b 81 b5 01 54 65 73 74 76 61 72 45 31 3d 76  .....TestvarE1=v",
			   hextoul(env_get("loadaddr"), NULL) + 0x00);
#else
	ut_assert_nextline("%08lx: b0 8b 81 b5 00 54 65 73 74 76 61 72 45 31 3d 76  .....TestvarE1=v",
			   hextoul(env_get("loadaddr"), NULL) + 0x00);
#endif
	ut_assert_nextline("%08lx: 61 6c 75 65 45 31 00 54 65 73 74 76 61 72 45 32  alueE1.TestvarE2",
			   hextoul(env_get("loadaddr"), NULL) + 0x10);
	ut_assert_nextline("%08lx: 3d 76 61 6c 75 65 45 32 00 00                    =valueE2..",
			   hextoul(env_get("loadaddr"), NULL) + 0x20);
#else
	ut_assertok(run_command("md.b ${loadaddr} 29", 0));
	ut_assert_nextline("%08lx: 8a dd d6 19 54 65 73 74 76 61 72 45 31 3d 76 61  ....TestvarE1=va",
			   hextoul(env_get("loadaddr"), NULL) + 0x00);
	ut_assert_nextline("%08lx: 6c 75 65 45 31 00 54 65 73 74 76 61 72 45 32 3d  lueE1.TestvarE2=",
			   hextoul(env_get("loadaddr"), NULL) + 0x10);
	ut_assert_nextline("%08lx: 76 61 6c 75 65 45 32 00 00                       valueE2..",
			   hextoul(env_get("loadaddr"), NULL) + 0x20);
#endif /* CONFIG_IS_ENABLED(SYS_REDUNDAND_ENVIRONMENT) */
	ut_assert_console_end();
	ut_assertok(run_command("echo ${filesize}", 0));
	ut_assert_nextline("%x", CONFIG_ENV_SIZE);
	ut_assert_console_end();

	/* Clear memory at loadaddr */
	ut_assertok(run_commandf("mw.b ${loadaddr} 0x00 %x", CONFIG_ENV_SIZE));
	ut_assert_console_end();

	/* Binary format with checksum and size */
#if CONFIG_IS_ENABLED(SYS_REDUNDAND_ENVIRONMENT)
	ut_assertok(run_command("env export -c -s 2a ${loadaddr} TestvarE1 TestvarE2", 0));
	ut_assert_console_end();
	ut_assertok(run_command("md.b ${loadaddr} 2a", 0));
#if CONFIG_IS_ENABLED(ENV_ADDR_REDUND)
	ut_assert_nextline("%08lx: bb 9e b9 96 01 54 65 73 74 76 61 72 45 31 3d 76  .....TestvarE1=v",
			   hextoul(env_get("loadaddr"), NULL) + 0x00);
#else
	ut_assert_nextline("%08lx: bb 9e b9 96 00 54 65 73 74 76 61 72 45 31 3d 76  .....TestvarE1=v",
			   hextoul(env_get("loadaddr"), NULL) + 0x00);
#endif
	ut_assert_nextline("%08lx: 61 6c 75 65 45 31 00 54 65 73 74 76 61 72 45 32  alueE1.TestvarE2",
			   hextoul(env_get("loadaddr"), NULL) + 0x10);
	ut_assert_nextline("%08lx: 3d 76 61 6c 75 65 45 32 00 00                    =valueE2..",
			   hextoul(env_get("loadaddr"), NULL) + 0x20);
#else
	ut_assertok(run_command("env export -c -s 29 ${loadaddr} TestvarE1 TestvarE2", 0));
	ut_assert_console_end();
	ut_assertok(run_command("md.b ${loadaddr} 29", 0));
	ut_assert_nextline("%08lx: bb 9e b9 96 54 65 73 74 76 61 72 45 31 3d 76 61  ....TestvarE1=va",
			   hextoul(env_get("loadaddr"), NULL) + 0x00);
	ut_assert_nextline("%08lx: 6c 75 65 45 31 00 54 65 73 74 76 61 72 45 32 3d  lueE1.TestvarE2=",
			   hextoul(env_get("loadaddr"), NULL) + 0x10);
	ut_assert_nextline("%08lx: 76 61 6c 75 65 45 32 00 00                       valueE2..",
			   hextoul(env_get("loadaddr"), NULL) + 0x20);
#endif /* CONFIG_IS_ENABLED(SYS_REDUNDAND_ENVIRONMENT) */
	ut_assert_console_end();
	ut_assertok(run_command("echo ${filesize}", 0));
	ut_assert_nextline("%x", CONFIG_ENV_SIZE);
	ut_assert_console_end();

	return 0;
}
NVEDIT_TEST(nvedit_test_export, UTF_CONSOLE);
#endif

#if CONFIG_IS_ENABLED(CMD_IMPORTENV)
static int nvedit_test_import(struct unit_test_state *uts)
{
				/* TestvarI1=valueI1\nTestvarI2=valueI2\n\0 */
	const char example_t[]   = {0x54, 0x65, 0x73, 0x74, 0x76, 0x61, 0x72, 0x49, 0x31, 0x3d,
				    0x76, 0x61, 0x6c, 0x75, 0x65, 0x49, 0x31, 0x0a, 0x54, 0x65,
				    0x73, 0x74, 0x76, 0x61, 0x72, 0x49, 0x32, 0x3d, 0x76, 0x61,
				    0x6c, 0x75, 0x65, 0x49, 0x32, 0x0a, 0x00};
				/* TestvarI1=valueI1\r\nTestvarI2=valueI2\r\n\0 */
	const char example_tr[]  = {0x54, 0x65, 0x73, 0x74, 0x76, 0x61, 0x72, 0x49, 0x31, 0x3d,
				    0x76, 0x61, 0x6c, 0x75, 0x65, 0x49, 0x31, 0x0d, 0x0a, 0x54,
				    0x65, 0x73, 0x74, 0x76, 0x61, 0x72, 0x49, 0x32, 0x3d, 0x76,
				    0x61, 0x6c, 0x75, 0x65, 0x49, 0x32, 0x0d, 0x0a, 0x00};
				/* TestvarI1=valueI1\0TestvarI2=valueI2\0\0 */
	const char example_b[]   = {0x54, 0x65, 0x73, 0x74, 0x76, 0x61, 0x72, 0x49, 0x31, 0x3d,
				    0x76, 0x61, 0x6c, 0x75, 0x65, 0x49, 0x31, 0x00, 0x54, 0x65,
				    0x73, 0x74, 0x76, 0x61, 0x72, 0x49, 0x32, 0x3d, 0x76, 0x61,
				    0x6c, 0x75, 0x65, 0x49, 0x32, 0x00, 0x00};
#if CONFIG_IS_ENABLED(SYS_REDUNDAND_ENVIRONMENT)
				/* CORRECT_CHKSUM[4]\0TestvarI1=valueI1\0TestvarI2=valueI2\0\0 */
	const char example_c[]   = {0xa3, 0x4b, 0x72, 0xf5, 0x00, 0x54, 0x65, 0x73, 0x74, 0x76,
				    0x61, 0x72, 0x49, 0x31, 0x3d, 0x76, 0x61, 0x6c, 0x75, 0x65,
				    0x49, 0x31, 0x00, 0x54, 0x65, 0x73, 0x74, 0x76, 0x61, 0x72,
				    0x49, 0x32, 0x3d, 0x76, 0x61, 0x6c, 0x75, 0x65, 0x49, 0x32,
				    0x00, 0x00};
				/* WRONG_CHKSUM[4]\0TestvarI1=valueI1\0TestvarI2=valueI2\0\0 */
	const char example_cw[]  = {0x3a, 0x57, 0xae, 0xc8, 0x00, 0x54, 0x65, 0x73, 0x74, 0x76,
				    0x61, 0x72, 0x49, 0x31, 0x3d, 0x76, 0x61, 0x6c, 0x75, 0x65,
				    0x49, 0x31, 0x00, 0x54, 0x65, 0x73, 0x74, 0x76, 0x61, 0x72,
				    0x49, 0x32, 0x3d, 0x76, 0x61, 0x6c, 0x75, 0x65, 0x49, 0x32,
				    0x00, 0x00};
#else
				/* CORRECT_CHKSUM[4]TestvarI1=valueI1\0TestvarI2=valueI2\0\0 */
	const char example_c[]   = {0xa3, 0x4b, 0x72, 0xf5, 0x54, 0x65, 0x73, 0x74, 0x76,
				    0x61, 0x72, 0x49, 0x31, 0x3d, 0x76, 0x61, 0x6c, 0x75, 0x65,
				    0x49, 0x31, 0x00, 0x54, 0x65, 0x73, 0x74, 0x76, 0x61, 0x72,
				    0x49, 0x32, 0x3d, 0x76, 0x61, 0x6c, 0x75, 0x65, 0x49, 0x32,
				    0x00, 0x00};
				/* WRONG_CHKSUM[4]TestvarI1=valueI1\0TestvarI2=valueI2\0\0 */
	const char example_cw[]  = {0x3a, 0x57, 0xae, 0xc8, 0x54, 0x65, 0x73, 0x74, 0x76,
				    0x61, 0x72, 0x49, 0x31, 0x3d, 0x76, 0x61, 0x6c, 0x75, 0x65,
				    0x49, 0x31, 0x00, 0x54, 0x65, 0x73, 0x74, 0x76, 0x61, 0x72,
				    0x49, 0x32, 0x3d, 0x76, 0x61, 0x6c, 0x75, 0x65, 0x49, 0x32,
				    0x00, 0x00};
#endif
				/* ==\n\0 */
	const char example_inv[] = {0x3d, 0x3d, 0x0a, 0x00};
	unsigned long loadaddr = hextoul(env_get("loadaddr"), NULL);
	int index = 0;

	/* Test error (combined parameter) */
	ut_asserteq(1, run_command("env import -tb ${loadaddr} TestvarI1 TestvarI2", 0));
	ut_assert_nextline("## import: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env import -tc ${loadaddr} TestvarI1 TestvarI2", 0));
	ut_assert_nextline("## import: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env import -trb ${loadaddr} TestvarI1 TestvarI2", 0));
	ut_assert_nextline("## import: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env import -trc ${loadaddr} TestvarI1 TestvarI2", 0));
	ut_assert_nextline("## import: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env import -bc ${loadaddr} TestvarI1 TestvarI2", 0));
	ut_assert_nextline("## import: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env import -tbc ${loadaddr} TestvarI1 TestvarI2", 0));
	ut_assert_nextline("## import: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env import -trbc ${loadaddr} TestvarI1 TestvarI2", 0));
	ut_assert_nextline("## import: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();

	/* Test error (sperate parameter) */
	ut_asserteq(1, run_command("env import -t -b ${loadaddr} TestvarI1 TestvarI2", 0));
	ut_assert_nextline("## import: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env import -t -c ${loadaddr} TestvarI1 TestvarI2", 0));
	ut_assert_nextline("## import: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env import -t -r -b ${loadaddr} TestvarI1 TestvarI2", 0));
	ut_assert_nextline("## import: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env import -t -r -c ${loadaddr} TestvarI1 TestvarI2", 0));
	ut_assert_nextline("## import: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env import -b -c ${loadaddr} TestvarI1 TestvarI2", 0));
	ut_assert_nextline("## import: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env import -t -b -c ${loadaddr} TestvarI1 TestvarI2", 0));
	ut_assert_nextline("## import: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env import -t -r -b -c ${loadaddr} TestvarI1 TestvarI2", 0));
	ut_assert_nextline("## import: only one of \"-b\", \"-c\" or \"-t\" allowed");
	ut_assert_console_end();

	/* Text format (without parameter -t) */
	for (index = 0; index < sizeof(example_t); index++)
		ut_assertok(run_commandf("mw.b %lx %x", loadaddr + index, example_t[index]));
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI1", 0));
	ut_assert_nextline("## Error: \"TestvarI1\" not defined");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI2", 0));
	ut_assert_nextline("## Error: \"TestvarI2\" not defined");
	ut_assert_console_end();
	ut_assertok(run_command("env import ${loadaddr} 25 TestvarI1 TestvarI2", 0));
	ut_assert_nextline("## Warning: defaulting to text format");
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI1", 0));
	ut_assert_nextline("TestvarI1=valueI1");
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI2", 0));
	ut_assert_nextline("TestvarI2=valueI2");
	ut_assert_console_end();
	ut_assertok(run_command("env delete TestvarI1 TestvarI2", 0));
	ut_assert_console_end();

	/* Text format */
	for (index = 0; index < sizeof(example_t); index++)
		ut_assertok(run_commandf("mw.b %lx %x", loadaddr + index, example_t[index]));
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI1", 0));
	ut_assert_nextline("## Error: \"TestvarI1\" not defined");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI2", 0));
	ut_assert_nextline("## Error: \"TestvarI2\" not defined");
	ut_assert_console_end();
	ut_assertok(run_command("env import -t ${loadaddr} 25 TestvarI1 TestvarI2", 0));
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI1", 0));
	ut_assert_nextline("TestvarI1=valueI1");
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI2", 0));
	ut_assert_nextline("TestvarI2=valueI2");
	ut_assert_console_end();
	ut_assertok(run_command("env delete TestvarI1 TestvarI2", 0));
	ut_assert_console_end();

	/* Text format CRLF (without parameter -t) */
	for (index = 0; index < sizeof(example_tr); index++)
		ut_assertok(run_commandf("mw.b %lx %x", loadaddr + index, example_tr[index]));
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI1", 0));
	ut_assert_nextline("## Error: \"TestvarI1\" not defined");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI2", 0));
	ut_assert_nextline("## Error: \"TestvarI2\" not defined");
	ut_assert_console_end();
	ut_assertok(run_command("env import -r ${loadaddr} 27 TestvarI1 TestvarI2", 0));
	ut_assert_nextline("## Warning: defaulting to text format");
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI1", 0));
	ut_assert_nextline("TestvarI1=valueI1");
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI2", 0));
	ut_assert_nextline("TestvarI2=valueI2");
	ut_assert_console_end();
	ut_assertok(run_command("env delete TestvarI1 TestvarI2", 0));
	ut_assert_console_end();

	/* Text format CRLF */
	for (index = 0; index < sizeof(example_tr); index++)
		ut_assertok(run_commandf("mw.b %lx %x", loadaddr + index, example_tr[index]));
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI1", 0));
	ut_assert_nextline("## Error: \"TestvarI1\" not defined");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI2", 0));
	ut_assert_nextline("## Error: \"TestvarI2\" not defined");
	ut_assert_console_end();
	ut_assertok(run_command("env import -tr ${loadaddr} 27 TestvarI1 TestvarI2", 0));
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI1", 0));
	ut_assert_nextline("TestvarI1=valueI1");
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI2", 0));
	ut_assert_nextline("TestvarI2=valueI2");
	ut_assert_console_end();
	ut_assertok(run_command("env delete TestvarI1 TestvarI2", 0));
	ut_assert_console_end();

	/* Text format without size (without parameter -t) */
	for (index = 0; index < sizeof(example_t); index++)
		ut_assertok(run_commandf("mw.b %lx %x", loadaddr + index, example_t[index]));
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI1", 0));
	ut_assert_nextline("## Error: \"TestvarI1\" not defined");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI2", 0));
	ut_assert_nextline("## Error: \"TestvarI2\" not defined");
	ut_assert_console_end();
	ut_assertok(run_command("env import ${loadaddr} - TestvarI1 TestvarI2", 0));
	ut_assert_nextline("## Warning: defaulting to text format");
	ut_assert_nextline("## Info: input data size = 37 = 0x25");
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI1", 0));
	ut_assert_nextline("TestvarI1=valueI1");
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI2", 0));
	ut_assert_nextline("TestvarI2=valueI2");
	ut_assert_console_end();
	ut_assertok(run_command("env delete TestvarI1 TestvarI2", 0));
	ut_assert_console_end();

	/* Text format without size */
	for (index = 0; index < sizeof(example_t); index++)
		ut_assertok(run_commandf("mw.b %lx %x", loadaddr + index, example_t[index]));
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI1", 0));
	ut_assert_nextline("## Error: \"TestvarI1\" not defined");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI2", 0));
	ut_assert_nextline("## Error: \"TestvarI2\" not defined");
	ut_assert_console_end();
	ut_assertok(run_command("env import -t ${loadaddr} - TestvarI1 TestvarI2", 0));
	ut_assert_nextline("## Info: input data size = 37 = 0x25");
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI1", 0));
	ut_assert_nextline("TestvarI1=valueI1");
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI2", 0));
	ut_assert_nextline("TestvarI2=valueI2");
	ut_assert_console_end();
	ut_assertok(run_command("env delete TestvarI1 TestvarI2", 0));
	ut_assert_console_end();

	/* Text format with delete parameter (without parameter -t) */
	for (index = 0; index < sizeof(example_t); index++)
		ut_assertok(run_commandf("mw.b %lx %x", loadaddr + index, example_t[index]));
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI1", 0));
	ut_assert_nextline("## Error: \"TestvarI1\" not defined");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI2", 0));
	ut_assert_nextline("## Error: \"TestvarI2\" not defined");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI3", 0));
	ut_assert_nextline("## Error: \"TestvarI3\" not defined");
	ut_assert_console_end();
	ut_assertok(run_command("env set TestvarI3 valueI3", 0));
	ut_assert_console_end();
	ut_assertok(run_command("env import -d ${loadaddr} 25 TestvarI1 TestvarI2 TestvarI3", 0));
	ut_assert_nextline("## Warning: defaulting to text format");
	ut_assert_nextline("WARNING: 'TestvarI3' not in imported env, deleting it!");
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI1", 0));
	ut_assert_nextline("TestvarI1=valueI1");
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI2", 0));
	ut_assert_nextline("TestvarI2=valueI2");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI3", 0));
	ut_assert_nextline("## Error: \"TestvarI3\" not defined");
	ut_assert_console_end();
	ut_assertok(run_command("env delete TestvarI1 TestvarI2", 0));
	ut_assert_console_end();

	/* Text format with delete parameter */
	for (index = 0; index < sizeof(example_t); index++)
		ut_assertok(run_commandf("mw.b %lx %x", loadaddr + index, example_t[index]));
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI1", 0));
	ut_assert_nextline("## Error: \"TestvarI1\" not defined");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI2", 0));
	ut_assert_nextline("## Error: \"TestvarI2\" not defined");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI3", 0));
	ut_assert_nextline("## Error: \"TestvarI3\" not defined");
	ut_assert_console_end();
	ut_assertok(run_command("env set TestvarI3 valueI3", 0));
	ut_assert_console_end();
	ut_assertok(run_command("env import -d -t ${loadaddr} 25 TestvarI1 TestvarI2 TestvarI3", 0));
	ut_assert_nextline("WARNING: 'TestvarI3' not in imported env, deleting it!");
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI1", 0));
	ut_assert_nextline("TestvarI1=valueI1");
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI2", 0));
	ut_assert_nextline("TestvarI2=valueI2");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI3", 0));
	ut_assert_nextline("## Error: \"TestvarI3\" not defined");
	ut_assert_console_end();
	ut_assertok(run_command("env delete TestvarI1 TestvarI2", 0));
	ut_assert_console_end();

	/* Text format with delete parameter and without vars (without parameter -t) */
	for (index = 0; index < sizeof(example_t); index++)
		ut_assertok(run_commandf("mw.b %lx %x", loadaddr + index, example_t[index]));
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI1", 0));
	ut_assert_nextline("## Error: \"TestvarI1\" not defined");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI2", 0));
	ut_assert_nextline("## Error: \"TestvarI2\" not defined");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI3", 0));
	ut_assert_nextline("## Error: \"TestvarI3\" not defined");
	ut_assert_console_end();
	ut_assertok(run_command("env set TestvarI3 valueI3", 0));
	ut_assert_console_end();
	ut_assertok(run_command("env import -d ${loadaddr} 25", 0));
	ut_assert_nextline("## Warning: defaulting to text format");
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI1", 0));
	ut_assert_nextline("TestvarI1=valueI1");
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI2", 0));
	ut_assert_nextline("TestvarI2=valueI2");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI3", 0));
	ut_assert_nextline("## Error: \"TestvarI3\" not defined");
	ut_assert_console_end();
	ut_assertok(run_command("env default -a", 0));
	ut_assert_nextline("## Resetting to default environment");
	ut_assert_console_end();

	/* Text format with delete parameter and without vars */
	for (index = 0; index < sizeof(example_t); index++)
		ut_assertok(run_commandf("mw.b %lx %x", loadaddr + index, example_t[index]));
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI1", 0));
	ut_assert_nextline("## Error: \"TestvarI1\" not defined");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI2", 0));
	ut_assert_nextline("## Error: \"TestvarI2\" not defined");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI3", 0));
	ut_assert_nextline("## Error: \"TestvarI3\" not defined");
	ut_assert_console_end();
	ut_assertok(run_command("env set TestvarI3 valueI3", 0));
	ut_assert_console_end();
	ut_assertok(run_command("env import -d -t ${loadaddr} 25", 0));
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI1", 0));
	ut_assert_nextline("TestvarI1=valueI1");
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI2", 0));
	ut_assert_nextline("TestvarI2=valueI2");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI3", 0));
	ut_assert_nextline("## Error: \"TestvarI3\" not defined");
	ut_assert_console_end();
	ut_assertok(run_command("env default -a", 0));
	ut_assert_nextline("## Resetting to default environment");
	ut_assert_console_end();

	/* Text format with wrong data */
	for (index = 0; index < sizeof(example_inv); index++)
		ut_assertok(run_commandf("mw.b %lx %x", loadaddr + index, example_inv[index]));
	ut_assert_console_end();
	ut_asserteq(1, run_command("env import -t ${loadaddr} 4", 0));
	ut_assert_nextline("## Error: Environment import failed: errno = 22");
	ut_assert_console_end();

	/* Binary format */
	for (index = 0; index < sizeof(example_b); index++)
		ut_assertok(run_commandf("mw.b %lx %x", loadaddr + index, example_b[index]));
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI1", 0));
	ut_assert_nextline("## Error: \"TestvarI1\" not defined");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI2", 0));
	ut_assert_nextline("## Error: \"TestvarI2\" not defined");
	ut_assert_console_end();
	ut_assertok(run_command("env import -b ${loadaddr} 25 TestvarI1 TestvarI2", 0));
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI1", 0));
	ut_assert_nextline("TestvarI1=valueI1");
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI2", 0));
	ut_assert_nextline("TestvarI2=valueI2");
	ut_assert_console_end();
	ut_assertok(run_command("env delete TestvarI1 TestvarI2", 0));
	ut_assert_console_end();

	/* Binary format without size */
	for (index = 0; index < sizeof(example_b); index++)
		ut_assertok(run_commandf("mw.b %lx %x", loadaddr + index, example_b[index]));
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI1", 0));
	ut_assert_nextline("## Error: \"TestvarI1\" not defined");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI2", 0));
	ut_assert_nextline("## Error: \"TestvarI2\" not defined");
	ut_assert_console_end();
	ut_assertok(run_command("env import -b ${loadaddr} - TestvarI1 TestvarI2", 0));
	ut_assert_nextline("## Info: input data size = 37 = 0x25");
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI1", 0));
	ut_assert_nextline("TestvarI1=valueI1");
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI2", 0));
	ut_assert_nextline("TestvarI2=valueI2");
	ut_assert_console_end();
	ut_assertok(run_command("env delete TestvarI1 TestvarI2", 0));
	ut_assert_console_end();

	/* Binary format with checksum */
	for (index = 0; index < sizeof(example_c); index++)
		ut_assertok(run_commandf("mw.b %lx %x", loadaddr + index, example_c[index]));
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI1", 0));
	ut_assert_nextline("## Error: \"TestvarI1\" not defined");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI2", 0));
	ut_assert_nextline("## Error: \"TestvarI2\" not defined");
	ut_assert_console_end();
#if CONFIG_IS_ENABLED(SYS_REDUNDAND_ENVIRONMENT)
	ut_assertok(run_command("env import -c ${loadaddr} 2a TestvarI1 TestvarI2", 0));
#else
	ut_assertok(run_command("env import -c ${loadaddr} 29 TestvarI1 TestvarI2", 0));
#endif
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI1", 0));
	ut_assert_nextline("TestvarI1=valueI1");
	ut_assert_console_end();
	ut_assertok(run_command("env print TestvarI2", 0));
	ut_assert_nextline("TestvarI2=valueI2");
	ut_assert_console_end();
	ut_assertok(run_command("env delete TestvarI1 TestvarI2", 0));
	ut_assert_console_end();

	/* Binary format with wrong checksum */
	for (index = 0; index < sizeof(example_cw); index++)
		ut_assertok(run_commandf("mw.b %lx %x", loadaddr + index, example_cw[index]));
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI1", 0));
	ut_assert_nextline("## Error: \"TestvarI1\" not defined");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI2", 0));
	ut_assert_nextline("## Error: \"TestvarI2\" not defined");
	ut_assert_console_end();
#if CONFIG_IS_ENABLED(SYS_REDUNDAND_ENVIRONMENT)
	ut_asserteq(1, run_command("env import -c ${loadaddr} 2a TestvarI1 TestvarI2", 0));
#else
	ut_asserteq(1, run_command("env import -c ${loadaddr} 29 TestvarI1 TestvarI2", 0));
#endif
	ut_assert_nextline("## Error: bad CRC, import failed");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI1", 0));
	ut_assert_nextline("## Error: \"TestvarI1\" not defined");
	ut_assert_console_end();
	ut_asserteq(1, run_command("env print TestvarI2", 0));
	ut_assert_nextline("## Error: \"TestvarI2\" not defined");
	ut_assert_console_end();

	/* Binary format with checksum and without size */
	for (index = 0; index < sizeof(example_c); index++)
		ut_assertok(run_commandf("mw.b %lx %x", loadaddr + index, example_c[index]));
	ut_assert_console_end();
	ut_asserteq(1, run_command("env import -c ${loadaddr}", 0));
	ut_assert_nextline("## Error: external checksum format must pass size");
	ut_assert_console_end();

	/* Binary format with checksum and with wrong size */
	for (index = 0; index < sizeof(example_c); index++)
		ut_assertok(run_commandf("mw.b %lx %x", loadaddr + index, example_c[index]));
	ut_assert_console_end();
	ut_asserteq(1, run_command("env import -c ${loadaddr} 4", 0));
	ut_assert_nextline("## Error: Invalid size 0x4");
	ut_assert_console_end();

	return 0;
}
NVEDIT_TEST(nvedit_test_import, UTF_CONSOLE);
#endif
