# Bluetooth firmware is not bundled with rtl8723 package per default
FILES_${PN}-rtl8723_append += "/lib/firmware/rtl_bt/rtl8723*.bin"
