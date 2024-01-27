SUMMARY = "Platform driver for the CPS100 platform"
HOMEPAGE = "www.hilscher.com"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0;md5=801f80980d171dd6425610833a22dbe6"

inherit module sign-wrapper

RDEPENDS:${PN} = "cps100-gpio-driver nct7904-gpio-driver"

SRC_URI = " \
  file://platform-cps100.c \
  file://Makefile \
"

S = "${WORKDIR}"

EXTRA_OEMAKE  = "${@bb.utils.contains('DISTRO_FEATURES', 'grsecurity', 'DISABLE_PAX_PLUGINS=y', '', d)}"
EXTRA_OEMAKE += "KERNEL_SRC=${STAGING_KERNEL_DIR}"

# Make sure package signing works correctly
INHIBIT_PACKAGE_STRIP="1"
EXTRA_OEMAKE   += "INSTALL_MOD_STRIP=1"

# libelf is required for CONFIG_STACK_VALIDATION=y
DEPENDS += "elfutils elfutils-native"

do_make_scripts() {
  unset CFLAGS CPPFLAGS CXXFLAGS LDFLAGS 
  make CC="${KERNEL_CC}" LD="${KERNEL_LD}" AR="${KERNEL_AR}" -C ${STAGING_KERNEL_DIR} O=${STAGING_KERNEL_BUILDDIR} ${EXTRA_OEMAKE} scripts
}

