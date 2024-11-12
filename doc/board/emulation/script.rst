.. SPDX-License-Identifier: GPL-2.0+

Script for building and running
===============================

You may find the script `scripts/build-qemu.sh` helpful for building and testing
U-Boot on QEMU.

If uses a environment variables to control how it works:

ubdir
    base directory for building U-Boot, with each board being in its own
    subdirectory

imagedir
    directory containing OS images, containin a subdirectory for each distro
    type (e.g. ubuntu/

Once configured, you can build and run QEMU for arm64 like this::

    scripts/build-qemu.sh -rsw

No support is currently included for specifying a root disk, so this script can
only be used to start installers.

Options
~~~~~~~

Options are available to control the script:

-a <arch>
    Select architecture (default arm, x86)

-B
    Don't build; assume a build exists

-k
    Use kvm - kernel-based Virtual Machine. By default QEMU uses its own
    emulator

-o <os>
    Run an Operating System. For now this only supports 'ubuntu'. The name of
    the OS file must remain unchanged from its standard name on the Ubuntu
    website.

-r
    Run QEMU with the image (by default this is not done)

-R
    Select OS release (e.g. 24.04).

-s
    Use serial only (no display)

-w
    Use word version (32-bit). By default, 64-bit is used

.. note::

    Note: For now this is a shell script, but if it expands it might be better
    as Python, accepting the slower startup.
