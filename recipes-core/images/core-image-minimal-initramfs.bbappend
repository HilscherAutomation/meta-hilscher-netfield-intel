PACKAGE_INSTALL:append:niot-e-tib100  = " cps100-platform-driver"

PACKAGE_INSTALL:append:niot-e-tijcx-gb = " nife200-platform-driver"
PACKAGE_INSTALL:append:niot-e-tijcx-gb = " nife200-gpio-driver"

PACKAGE_INSTALL:append = " kernel-module-efivarfs"
PACKAGE_INSTALL:append = " intel-microcode-early"

PACKAGE_INSTALL:remove = "linux-firmware-i915"
