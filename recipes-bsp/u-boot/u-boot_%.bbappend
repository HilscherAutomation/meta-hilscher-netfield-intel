FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

require recipes-bsp/u-boot/u-boot-netfield.inc

SRC_URI_append += " \
	file://netfield.cfg \
	file://bzimage_size_limit.patch \
	file://allow_larger_kernels.patch \
	file://fix_console_handling.patch \
	file://fat_show_files_without_arch_attr.patch \
	file://Change-the-output-of-shell-command-part-number-from-.patch \
"

# Supporting common netiot-distro-config.h
SRC_URI_append += " \
	file://0001-Add-Include-netiot_distro_config.h-support.patch \
	file://0002-Cleanup-the-x86-common-header-file.patch \
	file://machine_config.h \
"

inherit uefisign
DEPENDS_append += "u-boot-tools-native"

inherit dts-sign
#variables required to patch public key into dts
DTS_SIGN_ENFORCE="${PLATFORM_SIGN}"
DTS_SIGN_KEY_DIR="${PLATFORM_KEYDIR}"
DTS_SIGN_KEY_NAME="${PLATFORM_KEYNAME}"
DTS_TO_SIGN="${S}/arch/x86/dts/${UBOOT_DEVICE_TREE}.dts"

do_configure_prepend() {
	cp ${WORKDIR}/machine_config.h ${S}/include/configs/
}

do_compile_append() {
	ln -s u-boot-payload.efi ${B}/bootx64.efi
	uefisign_files ${B}/bootx64.efi
}

do_install_append() {
	install -d ${D}/boot
	install ${B}/bootx64.efi ${D}/boot/
}

do_deploy_append() {
	install -d ${DEPLOYDIR}/boot-files
	install -m 0644 ${B}/bootx64.efi ${DEPLOYDIR}/boot-files

	install -d ${DEPLOYDIR}/devicetree/
	install ${B}/u-boot.dtb ${DEPLOYDIR}/devicetree/
}
