FILESEXTRAPATHS_prepend := "${THISDIR}/${BPN}:"

SRC_URI_append += " \
    ${@bb.utils.contains('PLATFORM_SIGN', '1', 'file://install_uefi_keys', '', d)} \
    ${@bb.utils.contains('PLATFORM_SIGN', '1', 'file://pk.auth', '', d)}  \
    ${@bb.utils.contains('PLATFORM_SIGN', '1', 'file://kek.auth', '', d)} \
    ${@bb.utils.contains('PLATFORM_SIGN', '1', 'file://db.auth', '', d)}  \
"

#DEPENDS = "efitools-native"
RDEPENDS_${PN} = "${@bb.utils.contains('PLATFORM_SIGN', '1', 'efitools util-linux-mount', '', d)}"

do_install_append() {
  if [ "${PLATFORM_SIGN}" = "1" ]; then
    install -d ${D}/bin

    #####################
    # UEFI key installation
    #####################
    install -m 500 ${S}/install_uefi_keys ${D}/bin/

    install -d ${D}/etc/uefikeys
    install -m 400 ${WORKDIR}/kek.auth ${D}/etc/uefikeys/kek.auth
    install -m 400 ${WORKDIR}/db.auth ${D}/etc/uefikeys/db.auth
    install -m 400 ${WORKDIR}/pk.auth ${D}/etc/uefikeys/pk.auth
  fi
}

FILES_${PN}_append += "/"
