SUMMARY = "Initramfs bundled kernel image"
DESCRIPTION = "When built, it packages a initramfs bundled kernel image of the \
preferred virtual/kernel provider."

SECTION = "kernel"

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0;md5=801f80980d171dd6425610833a22dbe6"

require linux-version.inc

inherit deploy nopackages

S = "${STAGING_KERNEL_DIR}"
B = "${WORKDIR}/build"

KERNEL_IMAGETYPE ?= "bzImage"

PACKAGE_ARCH = "${MACHINE_ARCH}"

# Skip processing of this recipe if INITRAMFS_IMAGE or INITRAMFS_IMAGE_BUNDLE
# is not set correctly, to avoid generate a single empty package which makes
# no sense.
python __anonymous () {
    if not d.getVar('INITRAMFS_IMAGE', True) or d.getVar('INITRAMFS_IMAGE_BUNDLE', True) != '1':
        raise bb.parse.SkipPackage("Set INITRAMFS_IMAGE and INITRAMFS_IMAGE_BUNDLE to enable it")
}

# Need the output of deploy.
do_install[depends] += "virtual/kernel:do_deploy"

# We only need the packaging tasks - disable the rest
do_fetch[noexec] = "1"
do_unpack[noexec] = "1"
do_patch[noexec] = "1"
do_configure[noexec] = "1"
do_compile[noexec] = "1"
do_populate_sysroot[noexec] = "1"

inherit uefisign

do_install() {
  echo "Copying initramfs bundled kernel image from ${DEPLOY_DIR_IMAGE}..."
  install -m 0644 ${DEPLOY_DIR_IMAGE}/${KERNEL_IMAGETYPE}-initramfs-${MACHINE}.bin ${D}/${KERNEL_IMAGETYPE}

  # Signing the kernel ...
  uefisign_files /${KERNEL_IMAGEDEST}/${KERNEL_IMAGETYPE}
  kernel=${D}/${KERNEL_IMAGEDEST}/${KERNEL_IMAGETYPE}
  openssl_sign_wrapper ${PLATFORM_KEYNAME} "sha512" ${kernel}

  echo bzImage=\"${KERNEL_IMAGETYPE}-initramfs-${KERNEL_VERSION}-${PR}\" > ${D}/${KERNEL_IMAGEDEST}/readme
}

do_deploy() {
  install -d ${DEPLOYDIR}/boot-files
  cp ${D}/${KERNEL_IMAGEDEST}/${KERNEL_IMAGETYPE}* ${DEPLOYDIR}/boot-files/
}
addtask deploy before do_build after do_install
do_deploy[dirs] += "${DEPLOYDIR}/boot-files"
