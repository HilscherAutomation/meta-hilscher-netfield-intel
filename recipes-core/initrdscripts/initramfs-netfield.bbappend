FILESEXTRAPATHS:prepend := "${THISDIR}/${BPN}:"

# Required by *platform_init*
SRC_URI:append = " \
    file://install_uefi_keys \
    file://pk.auth  \
    file://kek.auth \
    file://db.auth \
"

RDEPENDS:${PN}-platform-init:append = " efitools util-linux-mount"
do_install:append() {
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
FILES:${PN}-platform-init:append = " ${bindir}/install_uefi_keys ${sysconfdir}/uefikeys/*"


# Required by *device_data*
RDEPENDS:${PN}-device-data:append = " efivar"
do_install:append:niot-e-vm-en() {
    echo ${MACHINE} | tr "[a-z]" "[A-Z]" > ${D}/forced_productname
    echo "FFFFFFFFFFFF" > ${D}/forced_productnumber
}
FILES:${PN}-device-data:append = " forced_productname forced_productnumber"
