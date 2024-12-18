#!/bin/bash
# SPDX-License-Identifier: GPL-2.0+
#
# Script to build U-Boot suitable for booting with QEMU, possibly running
# it, possibly with an OS image

# This just an example. It assumes that

# - you build U-Boot in ${ubdir}/<name> where <name> is the U-Boot board config
# - your OS images are in ${imagedir}/{distroname}/...

# So far the script supports only ARM and x86.

set -e

usage() {
	(
	if [[ -n "$1" ]]; then
		echo "$1"
		echo
	fi
	echo "Usage: $0 -aBkrsw"
	echo
	echo "   -a <arch> - Select architecture (arm, x86)"
	echo "   -B        - Don't build; assume a build exists"
	echo "   -e        - Run UEFI Self-Certification Test (SCT)"
	echo "   -k        - Use kvm (kernel-based Virtual Machine)"
	echo "   -o <name> - Run Operating System ('ubuntu' only for now)"
	echo "   -r        - Run QEMU with the image"
	echo "   -R <os>   - Select OS release (e.g. 24.04)"
	echo "   -s        - Use serial only (no display)"
	echo "   -S <seq>  - Select SCT sequence-file"
	echo "   -w        - Use word version (32-bit)" ) >&2
	exit 1
}

# Directory tree for OS images
imagedir=${imagedir-/vid/software/linux}

# Directory for UEFI Self-Certification Test (SCT)
sctdir=${sctdir-/vid/software/devel/uefi/sct}

# Mount point for use when writing to disk images
mnt=${mnt-/mnt}

# architecture (arm or x86)
arch=arm

# 32- or 64-bit build
bitness=64

# Build U-Boot
build=yes

# Extra setings
extra=

# Operating System to boot (ubuntu)
os=

release=24.04.1

# run the image with QEMU
run=

# run QEMU without a display (U-Boot must be set to stdout=serial)
serial=

# Use kvm
kvm=

# Set ubdir to the build directory where you build U-Boot out-of-tree
# We avoid in-tree build because it gets confusing trying different builds
ubdir=${ubdir-/tmp/b}

while getopts "a:Beko:rR:sS:w" opt; do
	case "${opt}" in
	a)
		arch=$OPTARG
		;;
	B)
		build=
		;;
	e)
		extra+=" -m 4G -smp 4"
		extra+=" -display none"
		extra+=" -device virtio-gpu-pci"
		extra+=" -device qemu-xhci"
		extra+=" -device usb-kbd"
		extra+=" -drive file=${sctdir}/sct.img,format=raw,if=none,id=vda"
		extra+=" -device virtio-blk-device,drive=vda,bootindex=1"
		extra+=" -device virtio-net-device,netdev=net0"
		extra+=" -netdev user,id=net0"
		;;
	k)
		kvm="-enable-kvm"
		;;
	o)
		os=$OPTARG

		# Expand memory and CPUs
		extra+=" -m 4G -smp 4"
		;;
	r)
		run=1
		;;
	R)
		release=$OPTARG
		;;
	s)
		serial=1
		;;
	S)
		seq=$OPTARG
		;;
	w)
		bitness=32
		;;
	*)
		usage
		;;
	esac
done

# Build U-Boot for the selected board
build_u_boot() {
	buildman -w -o $DIR --board $BOARD -I || exit $?
}

# Write the SCT test-sequence file into the SCT image
update_sct_seq() {
	if [[ -z "${seq}" ]]; then
		return
	fi
	LOOP=$(sudo losetup --show -f -P "${sctdir}/sct.img")
	PART="${LOOP}p1"
	sudo mount -o loop ${PART} ${mnt} -o uid=$(id -u),gid=$(id -g)
	cp "${seq}" "${mnt}/."
	sudo umount ${mnt}
	sudo losetup -d ${LOOP}
}

# Run QEMU with U-Boot
run_qemu() {
	if [[ -n "${os_image}" ]]; then
		extra+=" -drive if=virtio,file=${os_image},format=raw,id=hd0"
	fi
	if [[ -n "${serial}" ]]; then
		extra+=" -display none -serial mon:stdio"
	else
		extra+=" -serial mon:stdio"
	fi
	echo "Running ${qemu} ${extra}"
	"${qemu}" -bios "$DIR/${BIOS}" \
		-m 512 \
		-nic none \
		${kvm} \
		${extra}
}

# Check architecture
case "${arch}" in
arm)
	BOARD="qemu_arm"
	BIOS="u-boot.bin"
	qemu=qemu-system-arm
	extra+=" -machine virt -accel tcg"
	suffix="arm"
	if [[ "${bitness}" == "64" ]]; then
		BOARD="qemu_arm64"
		qemu=qemu-system-aarch64
		extra+=" -cpu cortex-a57"
		suffix="arm64"
	fi
	;;
x86)
	BOARD="qemu-x86"
	BIOS="u-boot.rom"
	qemu=qemu-system-i386
	suffix="i386"
	if [[ "${bitness}" == "64" ]]; then
		BOARD="qemu-x86_64"
		qemu=qemu-system-x86_64
		suffix="amd64"
	fi
	;;
*)
	usage "Unknown architecture '${arch}'"
esac

# Check OS
case "${os}" in
ubuntu)
	os_image="${imagedir}/${os}/${os}-${release}-desktop-${suffix}.iso"
	;;
"")
	;;
*)
	usage "Unknown OS '${os}'"
esac

DIR=${ubdir}/${BOARD}

if [[ -n "${build}" ]]; then
	build_u_boot
	update_sct_seq
fi

if [[ -n "${run}" ]]; then
	run_qemu
fi
