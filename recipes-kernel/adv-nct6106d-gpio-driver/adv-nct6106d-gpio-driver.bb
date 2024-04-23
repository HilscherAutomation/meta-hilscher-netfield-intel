SUMMARY = "NCT6106d GPIO driver"
HOMEPAGE = "https://www.advantech.com/en/support/details/software-api?id=1-2JGC61S"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

inherit module sign-wrapper

SRC_URI = " \
  file://gpio-nct6106d.c \
  file://Makefile \
"

S = "${WORKDIR}"

EXTRA_OEMAKE  = "${@bb.utils.contains('DISTRO_FEATURES', 'grsecurity', 'DISABLE_PAX_PLUGINS=y', '', d)}"
EXTRA_OEMAKE += "KERNEL_SRC=${STAGING_KERNEL_DIR}"

# Make sure package signing works correctly
INHIBIT_PACKAGE_STRIP="1"
EXTRA_OEMAKE += "INSTALL_MOD_STRIP=1"

# libelf is required for CONFIG_STACK_VALIDATION=y
DEPENDS += "elfutils elfutils-native"

do_make_scripts() {
  unset CFLAGS CPPFLAGS CXXFLAGS LDFLAGS 
  make CC="${KERNEL_CC}" LD="${KERNEL_LD}" AR="${KERNEL_AR}" -C ${STAGING_KERNEL_DIR} O=${STAGING_KERNEL_BUILDDIR} ${EXTRA_OEMAKE} scripts
}

