SUMMARY = "Barebox (formerly known as u-boot-v2) is a bootloader that inherits the best of U-Boot and the Linux kernel"
HOMEPAGE = "http://barebox.org/"
SECTION = "bootloaders"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM ?= "file://COPYING;md5=f5125d13e000b9ca1f0d3364286c4192"

inherit kernel-arch deploy cml1

PROVIDES = "virtual/bootloader barebox"

SRC_URI = "git://github.com/barebox/barebox;protocol=https;branch=master \
           file://defconfig"

S = "${WORKDIR}/git"

PACKAGE_ARCH = "${MACHINE_ARCH}"
PACKAGES = "${PN}"

EXTRA_OEMAKE = "CROSS_COMPILE=${TARGET_PREFIX}"

export KBUILD_BUILD_USER ?= "oe-user"
export KBUILD_BUILD_HOST ?= "oe-host"

DEPENDS += "kern-tools-native bison-native"

# returns all the elements from the src uri that are .cfg files
def find_cfgs(d):
    sources=src_patches(d, True)
    sources_list=[]
    for s in sources:
        if s.endswith('.cfg'):
            sources_list.append(s)

    return sources_list

do_configure:prepend() {
    cp ${WORKDIR}/defconfig ${B}/.config
    oe_runmake oldconfig

    # Apply possible configuration fragments
    merge_config.sh -m .config ${@" ".join(find_cfgs(d))}
}

do_compile:prepend() {
  unset LDFLAGS
}

do_install() {
  install -d ${D}/boot/efi/boot
  install ${B}/barebox.efi ${D}/boot/efi/boot/bootx64.efi
}

BUILD_ID="${DATETIME}"
BUILD_ID[vardepsexclude] = "DATETIME"

do_deploy () {
  install ${D}/boot/efi/boot/bootx64.efi ${DEPLOYDIR}/${PN}-${PV}-${BUILD_ID}.efi
  ln -sf ${PN}-${PV}-${BUILD_ID}.efi ${DEPLOYDIR}/bootx64.efi
}
addtask deploy before do_build after do_install

FILES:${PN} = "/boot"
