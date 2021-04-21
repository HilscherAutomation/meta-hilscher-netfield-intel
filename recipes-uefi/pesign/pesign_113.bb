SUMMARY="Signing tool for PE-COFF binaries."

LICENSE="GPLv3+"
LIC_FILES_CHKSUM = "file://COPYING;md5=f27defe1e96c2e1ecd4e0c9be8967949"

SRC_URI = "git://github.com/rhboot/pesign.git \
           file://disable_lto.patch \
           file://fix_compile_error.patch"
SRCREV="cbc37d9eb282c428a117a7f0af52ec8d9964e464"

S = "${WORKDIR}/git"

DEPENDS = "efivar nss util-linux popt"

export CROSS_COMPILE = "${TARGET_PREFIX}"

CFLAGS_append += "-I${STAGING_INCDIR}/nss3 -I${STAGING_INCDIR}/efivar"

do_compile() {
	oe_runmake RANLIB="${RANLIB}"
}

do_install() {
	oe_runmake install INSTALLROOT=${D}
}

BBCLASSEXTEND = "native"
