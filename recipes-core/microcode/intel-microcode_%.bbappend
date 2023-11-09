PACKAGES:append = " ${PN}-early"

do_install:append() {
	install -d ${D}/kernel/x86/microcode/
	install ${WORKDIR}/microcode_${PV}.bin ${D}/kernel/x86/microcode/GenuineIntel.bin
}

FILES:${PN}-early = "/kernel"
