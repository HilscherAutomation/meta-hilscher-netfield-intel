UEFI_SIG_OWNER_GUID="eaaf7a90-1f0f-11e9-b56e-0800200c9a66"
#do_compile_prepend() {
#    cp ${KEYS_DIR}/uefi/pk.key ${B}/PK.key
#    cp ${KEYS_DIR}/uefi/pk.crt ${B}/PK.crt
#    cp ${KEYS_DIR}/uefi/db.crt ${B}/DB.crt
#    cp ${KEYS_DIR}/uefi/kek.key ${B}/KEK.key
#    cp ${KEYS_DIR}/uefi/kek.crt ${B}/KEK.crt
#}

do_compile:append() {
    oe_runmake efi-keytool
}

do_install:append() {
    install ${B}/efi-keytool ${D}${bindir}
}

RDEPENDS:${PN}:remove = "mtools"
