FILESEXTRAPATHS_prepend := "${THISDIR}/${BPN}:"

# Required by *platform_init*
SRC_URI_append += " \
    file://install_uefi_keys \
    file://pk.auth  \
    file://kek.auth \
    file://db.auth \
"

RDEPENDS_${PN}-platform-init_append += "efitools util-linux-mount"
do_install_append() {
    install -d ${D}${bindir}

    #####################
    # UEFI key installation
    #####################
    install -m 500 ${WORKDIR}/install_uefi_keys ${D}${bindir}/install_uefi_keys

    install -d ${D}${sysconfdir}/uefikeys
    install -m 400 ${WORKDIR}/kek.auth ${D}/${sysconfdir}/uefikeys/kek.auth
    install -m 400 ${WORKDIR}/db.auth ${D}/${sysconfdir}/uefikeys/db.auth
    install -m 400 ${WORKDIR}/pk.auth ${D}/${sysconfdir}/uefikeys/pk.auth
}
FILES_${PN}-platform-init_append += "${bindir}/install_uefi_keys ${sysconfdir}/uefikeys/*"


# Required by *device_data*
RDEPENDS_${PN}-device-data_append += "efivar"
do_install_append_niot-e-vm-en() {
    echo ${MACHINE} | tr "[a-z]" "[A-Z]" > ${D}/forced_productname
}
FILES_${PN}-device-data_append += "forced_productname"
