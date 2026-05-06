# SPDX-License-Identifier:      GPL-2.0+
#
# Copyright (c) 2018, Linaro Limited
# Author: Takahiro Akashi <takahiro.akashi@linaro.org>

"""Helper functions for dealing with filesystems"""

import re
import os
import shutil
from subprocess import check_call, check_output, CalledProcessError
from subprocess import DEVNULL
import tempfile


class FsHelper:
    """Creating a filesystem containing test files

    Usage:
        with FsHelper(ubman.config, 'ext4', 10, 'mmc1') as fsh:
            # create files in the self.srcdir directory
            fsh.mk_fs()
            # Now use the filesystem

        # The filesystem and srcdir are erased after the 'with' statement.

        It is also possible to use an existing srcdir:

            with FsHelper(ubman.config, 'fat32', 10, 'usb2') as fsh:
                fsh.srcdir = src_dir
                fsh.mk_fs()
                ...

    Properties:
        fs_img (str): Filename for the filesystem image
    """
    def __init__(self, config, fs_type, size_mb, prefix):
        """Set up a new object

        Args:
            config (u_boot_config): U-Boot configuration
            fs_type (str): File system type: one of ext2, ext3, ext4, vfat,
                fat12, fat16, fat32, exfat, fs_generic (which means vfat)
            size_mb (int): Size of file system in MB
            prefix (str): Prefix string of volume's file name
        """
        if fs_type not in ['fat12', 'fat16', 'fat32', 'vfat',
                          'ext2', 'ext3', 'ext4',
                          'exfat', 'fs_generic']:
            raise ValueError(f"Unsupported filesystem type '{fs_type}'")

        self.config = config
        self.fs_type = fs_type
        self.size_mb = size_mb
        self.prefix = prefix
        self.quiet = True
        self.fs_img = None
        self.tmpdir = None
        self.srcdir = None
        self._do_cleanup = False

        # Some distributions do not add /sbin to the default PATH, where
        # mkfs lives
        if '/sbin' not in os.environ['PATH'].split(os.pathsep):
            os.environ['PATH'] += os.pathsep + '/sbin'

    def _get_fs_args(self):
        """Get the mkfs options and program to use

        Returns:
            tuple:
                str: mkfs options, e.g. '-F 32' for fat32
                str: mkfs program to use, e.g. 'ext4' for ext4
        """
        if self.fs_type == 'fat12':
            mkfs_opt = '-F 12'
        elif self.fs_type == 'fat16':
            mkfs_opt = '-F 16'
        elif self.fs_type == 'fat32':
            mkfs_opt = '-F 32'
        elif self.fs_type.startswith('ext'):
            mkfs_opt = f'-d {self.srcdir}'
        else:
            mkfs_opt = ''

        if self.fs_type == 'exfat':
            fs_lnxtype = 'exfat'
        elif re.match('fat', self.fs_type) or self.fs_type == 'fs_generic':
            fs_lnxtype = 'vfat'
        else:
            fs_lnxtype = self.fs_type
        return mkfs_opt, fs_lnxtype

    def mk_fs(self):
        """Make a new filesystem and copy in the files

        Raises:
            CalledProcessError: if any error occurs when creating the
                filesystem
        """
        self.setup()
        self._do_cleanup = True
        src_dir = self.srcdir if os.listdir(self.srcdir) else None

        fs_img = os.path.join(self.config.persistent_data_dir,
                              f'{self.prefix}.{self.fs_type}.img')

        mkfs_opt, fs_lnxtype = self._get_fs_args()

        check_call(f'rm -f {fs_img}', shell=True)
        check_call(f'truncate -s {self.size_mb}M {fs_img}', shell=True)
        check_call(f'mkfs.{fs_lnxtype} {mkfs_opt} {fs_img}', shell=True,
                   stdout=DEVNULL if self.quiet else None)
        if self.fs_type == 'ext4':
            sb_content = check_output(f'tune2fs -l {fs_img}',
                                      shell=True).decode()
            if 'metadata_csum' in sb_content:
                check_call(f'tune2fs -O ^metadata_csum {fs_img}', shell=True)
        elif fs_lnxtype == 'vfat' and src_dir:
            flags = f"-smpQ{'' if self.quiet else 'v'}"
            check_call(f'mcopy -i {fs_img} {flags} {src_dir}/* ::/',
                       shell=True)
        elif fs_lnxtype == 'exfat' and src_dir:
            check_call(f'fattools cp {src_dir}/* {fs_img}', shell=True)
        self.fs_img = fs_img

    def setup(self):
        """Set up the srcdir ready to receive files"""
        if not self.srcdir:
            if self.config:
                self.srcdir = os.path.join(self.config.persistent_data_dir,
                                           f'{self.prefix}.{self.fs_type}.tmp')
                if os.path.exists(self.srcdir):
                    shutil.rmtree(self.srcdir)
                os.mkdir(self.srcdir)
            else:
                self.tmpdir = tempfile.TemporaryDirectory('fs_helper')
                self.srcdir = self.tmpdir.name

    def cleanup(self):
        """Remove created image"""
        if self.tmpdir:
            self.tmpdir.cleanup()
        if self._do_cleanup:
            os.remove(self.fs_img)

    def __enter__(self):
        self.setup()
        return self

    def __exit__(self, extype, value, traceback):
        self.cleanup()


class DiskHelper:
    """Helper class for creating disk images containing filesytems

    Usage:
        with DiskHelper(ubman.config, 0, 'mmc') as img, \
                FsHelper(ubman.config, 'ext1', 1, 'mmc') as fsh:
            # Write files to fsh.srcdir
            ...

            # Create the filesystem
            fsh.mk_fs()

            # Add this filesystem to the disk
            img.add_fs(fsh, DiskHelper.VFAT)

            # Add more filesystems as needed (add another 'with' clause)
            ...

            # Get the final disk image
            data = img.create()
    """

    # Partition-type codes
    VFAT  = 0xc
    EXT4  = 0x83

    def __init__(self, config, devnum, prefix, cur_dir=False):
        """Set up a new disk image

        Args:
            config (u_boot_config): U-Boot configuration
            devnum (int): Device number (for filename)
            prefix (str): Prefix string of volume's file name
            cur_dir (bool): True to put the file in the current directory,
                instead of the persistent-data directory
        """
        self.fs_list = []
        self.fname = os.path.join('' if cur_dir else config.persistent_data_dir,
                                  f'{prefix}{devnum}.img')

    def add_fs(self, fs_img, part_type, bootable=False):
        """Add a new filesystem

        Args:
            fs_img (FsHelper): Filesystem to add
            part_type (DiskHelper.FAT or DiskHelper.EXT4): Partition type
            bootable (bool): True to set the 'bootable' flat
        """
        self.fs_list.append([fs_img, part_type, bootable])

    def create(self):
        """Create the disk image

        Create an image with a partition table and the filesystems
        """
        spec = ''
        pos = 1   # Reserve 1MB for the partition table itself
        for fsi, part_type, bootable in self.fs_list:
            if spec:
                spec += '\n'
            spec += f'type={part_type:x}, size={fsi.size_mb}M, start={pos}M'
            if bootable:
                spec += ', bootable'
            pos += fsi.size_mb

        img_size = pos
        try:
            check_call(f'qemu-img create {self.fname} {img_size}M', shell=True)
            check_call(f'printf "{spec}" | sfdisk {self.fname}', shell=True)
        except CalledProcessError:
            os.remove(self.fname)
            raise

        pos = 1   # Reserve 1MB for the partition table itself
        for fsi, part_type, bootable in self.fs_list:
            check_call(
                f'dd if={fsi.fs_img} of={self.fname} bs=1M seek={pos} conv=notrunc',
                shell=True)
            pos += fsi.size_mb
        return self.fname

    def cleanup(self, remove_full_img=False):
        """Remove created file"""
        os.remove(self.fname)

    def __enter__(self):
        return self

    def __exit__(self, extype, value, traceback):
        self.cleanup()


# Just for trying out
if __name__ == "__main__":
    import collections

    CNF = collections.namedtuple('config', 'persistent_data_dir')

    fsh = FsHelper(CNF('.'), 'ext4', 16, 'pref')
    fsh.setup()
    fsh.mk_fs()
