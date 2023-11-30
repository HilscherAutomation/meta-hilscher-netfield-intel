SRC_URI = "git://kernel.googlesource.com/pub/scm/linux/kernel/git/rt/linux-stable-rt.git;protocol=https;name=machine;branch=${KBRANCH} \
           git://git.yoctoproject.org/git/yocto-kernel-cache;protocol=https;type=kmeta;name=meta;branch=${KMETA_BRANCH};destsuffix=${KMETA}"

LIC_FILES_CHKSUM="file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"

KBRANCH = "v5.15-rt"
KMETA_BRANCH = "yocto-5.15"
do_fetch[vardeps] += "KMETA_BRANCH"
KMETA = "kernel-meta"
KCONF_BSP_AUDIT_LEVEL = "2"

LINUX_KERNEL_TYPE = "preempt-rt"
LINUX_VERSION ?= "5.15.137"
PV = "${LINUX_VERSION}+git${SRCPV}"
SRCREV_machine ?= "1554227c4ed0a033d7e82a725b0f1cf2000aa5c7"
SRCREV_meta ?= "328b31a095c93537e53e4581cb0b8b0433bfa40c"

SRC_URI += "file://enable_efiruntime_on_rt.patch"
SRC_URI += "file://enable_preempt_rt.cfg \
            file://baytrail.cfg  \
            file://rtl8723be.cfg \
            file://efi_stub.cfg  \
            file://kpti.cfg      \
            file://retpoline.cfg \
            file://uio.cfg       \
            file://ath10k.cfg    \
            file://allow_updating_microcode_from_bundled_initramfs.patch"

do_kernel_configme_append() {
    sed -i -e 's/CONFIG_PREEMPT=y/# CONFIG_PREEMPT is not set/' \
           -e 's/# CONFIG_PREEMPT_RT is not set/CONFIG_PREEMPT_RT=y/' ${B}/.config
}

require recipes-kernel/linux/linux-yocto.inc
require recipes-kernel/linux/meta-intel-compat-kernel.inc
require recipes-kernel/linux/netfield-linux.inc

COMPATIBLE_MACHINE ?= "(intel-corei7-64|intel-core2-32)"

# Prevent automatically inclusion of kernel-image into rootfs/image
RDEPENDS_kernel-base_remove += "kernel-image"
