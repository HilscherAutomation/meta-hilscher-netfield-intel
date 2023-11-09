FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = " file://C010D000.nxf"

do_install:append() {
    install -d ${D}/opt/cifx/FW
    install ${WORKDIR}/C010D000.nxf ${D}/opt/cifx/FW

    install -d ${D}/opt/cifx/deviceconfig/FW/channel0
    ln -s /opt/cifx/FW/C010D000.nxf ${D}/opt/cifx/deviceconfig/FW/channel0/default.nxf
}

FILES:${PN}:append = " /opt/cifx/deviceconfig/FW/channel0 /opt/cifx/FW"
