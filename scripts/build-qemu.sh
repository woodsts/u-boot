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
	echo "   -a   - Select architecture (arm, x86)"
	echo "   -B   - Don't build; assume a build exists"
	echo "   -k   - Use kvm (kernel-based Virtual Machine)"
	echo "   -o   - Run Operating System ('ubuntu' only for now)"
	echo "   -r   - Run QEMU with the image"
	echo "   -R   - Select OS release (e.g. 24.04)"
	echo "   -s   - Use serial only (no display)"
	echo "   -w   - Use word version (32-bit)" ) >&2
	exit 1
}

# Directory tree for OS images
imagedir=${imagedir-/vid/software/linux}

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

while getopts "a:Bko:rR:sw" opt; do
	case "${opt}" in
	a)
		arch=$OPTARG
		;;
	B)
		build=
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
	extra+=" -machine virt"
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
fi

if [[ -n "${run}" ]]; then
	run_qemu
fi
