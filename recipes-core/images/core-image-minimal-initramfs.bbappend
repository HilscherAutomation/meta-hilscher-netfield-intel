PACKAGE_INSTALL_append_niot-e-tib100  += "cps100-platform-driver"

PACKAGE_INSTALL_append_niot-e-tijcx-gb += "nife200-platform-driver"
PACKAGE_INSTALL_append_niot-e-tijcx-gb += "nife200-gpio-driver"

PACKAGE_INSTALL_append += "kernel-module-efivarfs"
PACKAGE_INSTALL_append += "intel-microcode-early"

PACKAGE_INSTALL_remove += "linux-firmware-i915"
