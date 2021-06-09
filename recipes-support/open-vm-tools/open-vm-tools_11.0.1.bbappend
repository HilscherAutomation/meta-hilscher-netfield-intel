FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI_append += "file://fix_udev_warnings.patch"
