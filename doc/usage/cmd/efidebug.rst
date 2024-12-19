.. SPDX-License-Identifier: GPL-2.0+
.. Copyright 2024 Google LLC
.. Written by Simon Glass <sjg@chromium.org>

.. index::
   single: efidebug (command)

efidebug command
================

Synopsis
--------

::

    efidebug log

Description
-----------

The *efidebug* command provides access to debugging features for the EFI-loader
subsystem.

Only one of the subcommands are documented at present.

efidebug log
~~~~~~~~~~~~

This shows a log of EFI boot-services calls which have been handled since U-Boot
started. This can be useful to see what the app is doing, or even what U-Boot
itself has called.


Example
-------

This shows checking the log, then using 'efidebug tables' to fully set up the
EFI-loader subsystem, then checking the log again::

    => efidebug log
    EFI log (size 158)
    0   alloc_pool bt-data size 33/51 buf 7fffd8448ad0 *buf 7c20010 ret OK
    1  alloc_pages any-pages bt-data pgs 1 mem 7fffd8448a80 *mem 7c20000 ret OK
    2   alloc_pool bt-data size 60/96 buf 7fffd8448ac0 *buf 7c1f010 ret OK
    3  alloc_pages any-pages bt-data pgs 1 mem 7fffd8448a60 *mem 7c1f000 ret OK
    4   alloc_pool bt-data size 60/96 buf 7fffd8448ac0 *buf 7c1e010 ret OK
    5  alloc_pages any-pages bt-data pgs 1 mem 7fffd8448a60 *mem 7c1e000 ret OK
    6 records
    => efidebug tables
    efi_var_to_file() Cannot persist EFI variables without system partition
    0000000017bfc010  36122546-f7ef-4c8f-bd9b-eb8525b50c0b  EFI Conformance Profiles Table
    0000000017bd4010  b122a263-3661-4f68-9929-78f8b0d62180  EFI System Resource Table
    0000000017bd8010  1e2ed096-30e2-4254-bd89-863bbef82325  TCG2 Final Events Table
    0000000017bd6010  eb66918a-7eef-402a-842e-931d21c38ae9  Runtime properties
    0000000008c49000  8868e871-e4f1-11d3-bc22-0080c73c8881  ACPI table
    0000000018c5b000  f2fd1544-9794-4a2c-992e-e5bbcf20e394  SMBIOS3 table
    => efidebug log
    EFI log (size a20)
    0   alloc_pool bt-data size 33/51 buf 7fffd8448ad0 *buf 7c20010 ret OK
    1  alloc_pages any-pages bt-data pgs 1 mem 7fffd8448a80 *mem 7c20000 ret OK
    2   alloc_pool bt-data size 60/96 buf 7fffd8448ac0 *buf 7c1f010 ret OK
    3  alloc_pages any-pages bt-data pgs 1 mem 7fffd8448a60 *mem 7c1f000 ret OK
    4   alloc_pool bt-data size 60/96 buf 7fffd8448ac0 *buf 7c1e010 ret OK
    5  alloc_pages any-pages bt-data pgs 1 mem 7fffd8448a60 *mem 7c1e000 ret OK
    6  alloc_pages any-pages rt-data pgs 20/32 mem 7fffd8448838 *mem 7bfe000 ret OK
    7   alloc_pool rt-data size 60/96 buf 7fffd84487e0 *buf 7bfd010 ret OK
    8  alloc_pages any-pages rt-data pgs 1 mem 7fffd8448780 *mem 7bfd000 ret OK
    9   alloc_pool rt-data size 180/384 buf 56f190ffd890 *buf 7bfc010 ret OK
    10  alloc_pages any-pages rt-data pgs 1 mem 7fffd8448800 *mem 7bfc000 ret OK
    11   alloc_pool bt-data size 4 buf 7fffd8448840 *buf 7bfb010 ret OK
    12  alloc_pages any-pages bt-data pgs 1 mem 7fffd84487f0 *mem 7bfb000 ret OK
    13   alloc_pool bt-data size 10/16 buf 7fffd8448728 *buf 7bfa010 ret OK
    14  alloc_pages any-pages bt-data pgs 1 mem 7fffd84486d0 *mem 7bfa000 ret OK
    15   alloc_pool bt-data size 60/96 buf 7fffd84487e0 *buf 7bf9010 ret OK
    16  alloc_pages any-pages bt-data pgs 1 mem 7fffd8448780 *mem 7bf9000 ret OK
    17   alloc_pool bt-data size 10000/65536 buf 56f19100fae0 *buf 7be8010 ret OK
    18  alloc_pages any-pages bt-data pgs 11/17 mem 7fffd84487d0 *mem 7be8000 ret OK
    19   alloc_pool acpi-nvs size 10000/65536 buf 56f19100fae8 *buf 7bd7010 ret OK
    20  alloc_pages any-pages acpi-nvs pgs 11/17 mem 7fffd84487d0 *mem 7bd7000 ret OK
    21   alloc_pool bt-data size 60/96 buf 7fffd84487d0 *buf 7bd6010 ret OK
    22  alloc_pages any-pages bt-data pgs 1 mem 7fffd8448770 *mem 7bd6000 ret OK
    23   alloc_pool rt-data size 8 buf 7fffd8448818 *buf 7bd5010 ret OK
    24  alloc_pages any-pages rt-data pgs 1 mem 7fffd84487c0 *mem 7bd5000 ret OK
    25   alloc_pool bt-data size 8 buf 7fffd8448360 *buf 7bd4010 ret OK
    26  alloc_pages any-pages bt-data pgs 1 mem 7fffd8448160 *mem 7bd4000 ret OK
    27   alloc_pool bt-data size f0/240 buf 7fffd8448378 *buf 7bd3010 ret OK
    28  alloc_pages any-pages bt-data pgs 1 mem 7fffd84482d0 *mem 7bd3000 ret OK
    29    free_pool buf 7bd3010 ret OK
    30   free_pages mem 7bd3000 pag 1 ret OK
    31   alloc_pool bt-data size 60/96 buf 7fffd84482d8 *buf 7bd3010 ret OK
    32  alloc_pages any-pages bt-data pgs 1 mem 7fffd8448280 *mem 7bd3000 ret OK
    33    free_pool buf 7bfa010 ret OK
    34   free_pages mem 7bfa000 pag 1 ret OK
    35   alloc_pool bt-data size f0/240 buf 7fffd8448380 *buf 7bfa010 ret OK
    36  alloc_pages any-pages bt-data pgs 1 mem 7fffd84482d0 *mem 7bfa000 ret OK
    37    free_pool buf 7bfa010 ret OK
    38   free_pages mem 7bfa000 pag 1 ret OK
    39    free_pool buf 7bd4010 ret OK
    40   free_pages mem 7bd4000 pag 1 ret OK
    41   alloc_pool bt-data size 61/97 buf 7fffd8448810 *buf 7bfa010 ret OK
    42  alloc_pages any-pages bt-data pgs 1 mem 7fffd84487c0 *mem 7bfa000 ret OK
    43   alloc_pool bt-data size 60/96 buf 7fffd8448800 *buf 7bd4010 ret OK
    44  alloc_pages any-pages bt-data pgs 1 mem 7fffd84487a0 *mem 7bd4000 ret OK
    45   alloc_pool bt-data size 60/96 buf 7fffd8448800 *buf 7bd2010 ret OK
    46  alloc_pages any-pages bt-data pgs 1 mem 7fffd84487a0 *mem 7bd2000 ret OK
    47   alloc_pool bt-data size 60/96 buf 7fffd8448810 *buf 7bd1010 ret OK
    48  alloc_pages any-pages bt-data pgs 1 mem 7fffd84487b0 *mem 7bd1000 ret OK
    49 records
    =>
