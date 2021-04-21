SUMMARY = "Switch display output if a display is hotplugged"
PACKAGES = "${PN}"
LICENSE = "CLOSED"

PR = "r0"

SRC_URI = "file://display-hotplug.service \
           file://display-hotplug.sh \
"
RDEPENDS_${PN} += "xrandr"

do_install() {
    if [ "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'systemd', '', d)}" = "systemd" ]; then
	install -d ${D}${systemd_unitdir}/system/
        install -m 0644 ${WORKDIR}/${BPN}.service ${D}${systemd_unitdir}/system/
        install -m 0755 ${WORKDIR}/${BPN}.sh ${D}${systemd_unitdir}/
    fi
}

inherit systemd

SYSTEMD_SERVICE_${PN} = "${BPN}.service"

FILES_${PN} += "${systemd_unitdir}"
