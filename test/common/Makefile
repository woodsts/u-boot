# SPDX-License-Identifier: GPL-2.0+
obj-$(CONFIG_AUTOBOOT) += test_autoboot.o

ifneq ($(CONFIG_BLOBLIST),)
ifdef CONFIG_BLOBLIST_FIXED
obj-$(CONFIG_CMDLINE) += bloblist.o
endif
endif

obj-$(CONFIG_CYCLIC) += cyclic.o
obj-$(CONFIG_EVENT_DYNAMIC) += event.o
obj-y += cread.o
obj-$(CONFIG_CMDLINE) += print.o
