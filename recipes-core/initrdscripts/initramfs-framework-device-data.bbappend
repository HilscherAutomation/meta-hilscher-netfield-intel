RDEPENDS_${PN}_append += "efivar"

do_install_append_niot-e-vm-en() {
    echo ${MACHINE} | tr "[a-z]" "[A-Z]" > ${D}/forced_productname
}
FILES_${PN}_append_niot-e-vm-en += "/forced_productname"
