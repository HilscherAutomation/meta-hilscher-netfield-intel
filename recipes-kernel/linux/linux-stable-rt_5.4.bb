SRC_URI = "git://kernel.googlesource.com/pub/scm/linux/kernel/git/rt/linux-stable-rt.git;protocol=https;name=machine;branch=${KBRANCH} \
           git://git.yoctoproject.org/git/yocto-kernel-cache;protocol=https;type=kmeta;name=meta;branch=${KMETA_BRANCH};destsuffix=${KMETA}"

LIC_FILES_CHKSUM="file://COPYING;md5=bbea815ee2795b2f4230826c0c6b8814"

KBRANCH = "v5.4-rt"
KMETA_BRANCH = "yocto-5.4"
do_fetch[vardeps] += "KMETA_BRANCH"
KMETA = "kernel-meta"
KCONF_BSP_AUDIT_LEVEL = "2"

LINUX_KERNEL_TYPE = "preempt-rt"
require linux-version.inc
PV = "${LINUX_VERSION}+git${SRCPV}"
SRCREV_machine ?= "f30e118c1cde0ba5319bd707b88bd9106284f6be"
SRCREV_meta ?= "70b2480497528245c948ec259c734d74ea4fa3f1"

SRC_URI += "file://enable_efiruntime_on_rt.patch"
SRC_URI += "file://enable_preempt_rt.cfg \
            file://baytrail.cfg  \
            file://rtl8723be.cfg \
            file://efi_stub.cfg  \
            file://kpti.cfg      \
            file://retpoline.cfg \
            file://uio.cfg       \
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
