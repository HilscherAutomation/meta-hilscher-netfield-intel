FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = " file://fix_udev_warnings.patch"
