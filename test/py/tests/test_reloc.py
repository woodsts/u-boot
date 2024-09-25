# SPDX-License-Identifier: GPL-2.0
# Copyright 2024 Google LLC
# Written by Simon Glass <sjg@chromium.org>

import pytest

@pytest.mark.buildconfigspec('target_qemu_arm_64bit_tpl')
def test_reloc_loader(u_boot_console):
    try:
        cons = u_boot_console
        # x = cons.restart_uboot()
        output = cons.get_spawn_output().replace('\r', '')
        assert 'spl_reloc TPL->SPL' in output
        assert 'Loading to 40100200' in output

        # Sanity check that it is picking up the correct environment
        board_name = cons.run_command('print board_name')
        assert board_name == 'board_name="qemu-arm64_tpl"'
    finally:
        # Restart afterward to get the normal U-Boot back
        u_boot_console.restart_uboot()
