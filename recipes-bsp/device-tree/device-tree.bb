SUMMARY = "Hilscher BSP device trees"
DESCRIPTION = "Hilscher BSP device trees from within layer."
SECTION = "bsp"

# the device trees from within the layer are licensed as MIT, kernel includes are GPL
LICENSE = "MIT & GPL-2.0-only"
LIC_FILES_CHKSUM = " \
	file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302 \
	file://${COMMON_LICENSE_DIR}/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6 \
"

inherit devicetree

S = "${WORKDIR}/src"

# Note:
#   Since x86 platforms do not require a DT, the one provided by this recipe is only used as a dummy to be included in the fitImage.
#   For reasons of secure boot, the bootloader has its own mainline DT to which a public-key is appended.
#   This public-key will then be used for fitImage verifications!
SRC_URI = "file://src/intel.dts"

COMPATIBLE_MACHINE  = "(generic-x64|niot-e-tijcx-gb|niot-e-vm-en)"
