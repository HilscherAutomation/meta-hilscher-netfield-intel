FILESEXTRAPATHS_prepend := "${THISDIR}/${BPN}:"

SRC_URI_append += "file://install_uefi_keys \
"

DEPENDS = "efitools-native"
RDEPENDS_${PN} = "efitools util-linux-mount"

do_install_append() {
  cp ${KEYS_DIR}/uefi/*.auth ./

  install -d ${D}/bin

  #####################
  # UEFI key installation
  #####################
  install -m 500 ${S}/install_uefi_keys ${D}/bin/

  install -d ${D}/etc/uefikeys
  install -m 400 kek.auth ${D}/etc/uefikeys/kek.auth
  install -m 400 db.auth ${D}/etc/uefikeys/db.auth
  install -m 400 pk.auth ${D}/etc/uefikeys/pk.auth
}

FILES_${PN}_append += "/"
