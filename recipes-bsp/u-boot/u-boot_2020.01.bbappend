FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI_append += " \
    file://boot.cmd \
    file://boot-recovery.cmd \
    file://netfield.cfg \
    file://bzimage_size_limit.patch \
"

inherit uefisign
DEPENDS_append += "u-boot-tools-native"

BOOT_SCRIPTS="boot boot-recovery"

inherit dts-sign
#variables required to patch public key into dts
DTS_SIGN_ENFORCE="${PLATFORM_SIGN}"
DTS_SIGN_KEY_DIR="${PLATFORM_KEYDIR}"
DTS_SIGN_KEY_NAME="${PLATFORM_KEYNAME}"
DTS_TO_SIGN="${S}/arch/x86/dts/${UBOOT_DEVICE_TREE}.dts"

do_compile_append() {
    for file in ${BOOT_SCRIPTS}; do
        mkimage -A x86_64 -T script -C none -n "Boot script" -d ${WORKDIR}/$file.cmd $file.scr
    done

    ln -s u-boot-payload.efi ${B}/bootx64.efi
    uefisign_files ${B}/bootx64.efi
}

do_install_append() {
    install -d ${D}/boot
    install ${B}/bootx64.efi ${D}/boot/
}

do_deploy_append() {
    install -d ${DEPLOYDIR}/boot-files
    for file in ${BOOT_SCRIPTS}; do
        install -m 0644 ${B}/$file.scr ${DEPLOYDIR}/boot-files
    done

    install -m 0644 ${B}/bootx64.efi ${DEPLOYDIR}/boot-files

    install -d ${DEPLOYDIR}/devicetree/
    install ${B}/u-boot.dtb ${DEPLOYDIR}/devicetree/
}
