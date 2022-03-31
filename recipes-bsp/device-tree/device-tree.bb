SUMMARY = "Hilscher BSP device trees"
DESCRIPTION = "Hilscher BSP device trees from within layer."
SECTION = "bsp"

# the device trees from within the layer are licensed as MIT, kernel includes are GPL
LICENSE = "MIT & GPLv2"
LIC_FILES_CHKSUM = " \
	file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302 \
	file://${COMMON_LICENSE_DIR}/GPL-2.0;md5=801f80980d171dd6425610833a22dbe6 \
"

inherit devicetree

#inherit dts-sign
# Setup public key patching into dts
#DTS_SIGN_ENFORCE="${PLATFORM_SIGN}"
#DTS_SIGN_KEY_DIR="${PLATFORM_KEYDIR}"
#DTS_SIGN_KEY_NAME="${PLATFORM_KEYNAME}"

#do_unpack[vardeps] += "PLATFORM_SIGN PLATFORM_KEYDIR PLATFORM_KEYNAME"

S = "${WORKDIR}/src"

SRC_URI = "file://src/intel.dts"
#DTS_TO_SIGN ?= "${S}/intel.dts"

COMPATIBLE_MACHINE="(niot-e-vm-en)"
