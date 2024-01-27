PACKAGES:append = " ${PN}-early"

do_install:prepend() {
        ${STAGING_DIR_NATIVE}${sbindir_native}/iucode_tool \
                ${UCODE_FILTER_PARAMETERS} \
                --overwrite \
                --write-to=${WORKDIR}/microcode_${PV}.bin \
                ${S}/intel-ucode/* ${S}/intel-ucode-with-caveats/*
}

do_install:append() {
	install -d ${D}/kernel/x86/microcode/
	install ${WORKDIR}/microcode_${PV}.bin ${D}/kernel/x86/microcode/GenuineIntel.bin
}

FILES:${PN}-early = "/kernel"
