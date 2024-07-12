SRC_URI = "git://kernel.googlesource.com/pub/scm/linux/kernel/git/rt/linux-stable-rt.git;protocol=https;name=machine;branch=${KBRANCH} \
           git://git.yoctoproject.org/git/yocto-kernel-cache;protocol=https;type=kmeta;name=meta;branch=${KMETA_BRANCH};destsuffix=${KMETA}"

LIC_FILES_CHKSUM="file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"

KBRANCH = "v5.15-rt"
KMETA_BRANCH = "yocto-5.15"
do_fetch[vardeps] += "KMETA_BRANCH"
KMETA = "kernel-meta"
KCONF_BSP_AUDIT_LEVEL = "2"

LINUX_KERNEL_TYPE = "preempt-rt"
LINUX_VERSION ?= "5.15.162"
# Update kernel via patch, as it is not yet available mainline
SRC_URI:append = " file://linux-5.15.160-to-162.patch"
addtask do_kernel_version_sanity_check after do_patch

PV = "${LINUX_VERSION}+git${SRCPV}"
SRCREV_machine ?= "1671cc3c15cc3955367d7f7ab4e2759ac1c798e1"
SRCREV_meta ?= "a9112e1b2552a7b037b2f90699505e7c1e4d6a34"

SRC_URI += "file://enable_efiruntime_on_rt.patch"
SRC_URI += "file://enable_preempt_rt.cfg \
            file://allow_updating_microcode_from_bundled_initramfs.patch"

do_kernel_configme:append() {
    sed -i -e 's/CONFIG_PREEMPT=y/# CONFIG_PREEMPT is not set/' \
           -e 's/# CONFIG_PREEMPT_RT is not set/CONFIG_PREEMPT_RT=y/' ${B}/.config
}

require recipes-kernel/linux/linux-yocto.inc
require recipes-kernel/linux/meta-intel-compat-kernel.inc
require recipes-kernel/linux/netfield-linux.inc
require cve-exclusions.inc

COMPATIBLE_MACHINE ?= "(intel-corei7-64|intel-core2-32)"

# Prevent automatically inclusion of kernel-image into rootfs/image
RDEPENDS:kernel-base:remove = " kernel-image"
