FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI_append += "file://C010D000.nxf"

do_install_append() {
    install -d ${D}/opt/cifx/FW
    install ${WORKDIR}/C010D000.nxf ${D}/opt/cifx/FW

    install -d ${D}/opt/cifx/deviceconfig/FW/channel0
    ln -s /opt/cifx/FW/C010D000.nxf ${D}/opt/cifx/deviceconfig/FW/channel0/default.nxf
}

FILES_${PN}_append += "/opt/cifx/deviceconfig/FW/channel0 /opt/cifx/FW"
