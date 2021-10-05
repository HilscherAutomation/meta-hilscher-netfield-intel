FILESEXTRAPATHS_prepend := "${THISDIR}/${BPN}:"

SRC_URI_append += "file://install_uefi_keys \
    file://pk.auth  \
    file://kek.auth \
    file://db.auth  \
"

#DEPENDS = "efitools-native"
RDEPENDS_${PN} = "efitools util-linux-mount"

do_install_append() {
  install -d ${D}/bin

  #####################
  # UEFI key installation
  #####################
  install -m 500 ${S}/install_uefi_keys ${D}/bin/

  install -d ${D}/etc/uefikeys
  install -m 400 ${WORKDIR}/kek.auth ${D}/etc/uefikeys/kek.auth
  install -m 400 ${WORKDIR}/db.auth ${D}/etc/uefikeys/db.auth
  install -m 400 ${WORKDIR}/pk.auth ${D}/etc/uefikeys/pk.auth
}

FILES_${PN}_append += "/"
