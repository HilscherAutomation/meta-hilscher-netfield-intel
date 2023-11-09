# Bluetooth firmware is not bundled with rtl8723 package per default
FILES:${PN}-rtl8723:append = " /lib/firmware/rtl_bt/rtl8723*.bin"
