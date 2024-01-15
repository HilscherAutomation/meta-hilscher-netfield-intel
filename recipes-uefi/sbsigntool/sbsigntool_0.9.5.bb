SUMMARY = "Utilities for signing UEFI binaries for use with secure boot"

LICENSE = "GPLv3"

LIC_FILES_CHKSUM = "\
    file://LICENSE.GPLv3;md5=9eef91148a9b14ec7f9df333daebc746 \
    file://COPYING;md5=a7710ac18adec371b84a9594ed04fd20 \
"

DEPENDS += "binutils openssl gnu-efi util-linux"

SRC_URI = "\
    gitsm://kernel.googlesource.com/pub/scm/linux/kernel/git/jejb/sbsigntools.git;protocol=https;branch=master \
    file://disable_man_generation.patch \
    file://fix_pkcs11_segfault.patch \
"
SRCREV="9cfca9fe7aa7a8e29b92fe33ce8433e212c9a8ba"

S = "${WORKDIR}/git"

inherit autotools-brokensep pkgconfig

def efi_arch(d):
    import re
    arch = d.getVar("TARGET_ARCH")
    if re.match("i[3456789]86", arch):
        return "ia32"
    return arch

EXTRA_OEMAKE += "\
    INCLUDES='-I${S}/lib/ccan.git' \
    EFI_CPPFLAGS='-I${STAGING_INCDIR}/efi \
                  -I${STAGING_INCDIR}/efi/${@efi_arch(d)}' \
"

do_configure() {
    cd "${S}"

    OLD_CC="${CC}"

    if [ ! -e lib/ccan ]; then
        export CC="${BUILD_CC}"
        lib/ccan.git/tools/create-ccan-tree \
            --build-type=automake lib/ccan \
                talloc read_write_all build_assert array_size endian || exit 1
    fi

    export CC="${OLD_CC}"
    export CRTPATH="${STAGING_LIBDIR_NATIVE}"
    ./autogen.sh --noconfigure
    oe_runconf
}

BBCLASSEXTEND = "native nativesdk"
