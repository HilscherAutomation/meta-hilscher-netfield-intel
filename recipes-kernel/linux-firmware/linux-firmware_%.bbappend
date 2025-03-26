# Bluetooth firmware is not bundled with rtl8723 package per default
FILES:${PN}-rtl8723:append = " /lib/firmware/rtl_bt/rtl8723*.bin"

do_install:append() {
    # Make sure that linux-firmware_rtl8723 does not depend on linux-firmware due to missing symlink file
    # NOTE: Yocto decided to RDEPEND on linux-firmware as rtl8723 contained a symlink rtl8723d_config.bin -> rtl8821c_config.bin
    #       as rtl8821c_config.bin is in linux-firmware
    for f in ${D}/lib/firmware/rtl_bt/rtl8723*.bin; do
        if [ -L "$f" ]; then
            realtarget=$(readlink "$f")
            if ! echo $realtarget | grep -q rtl8723; then
                rm "$f"
                cp ${D}/lib/firmware/rtl_bt/"$realtarget" "$f"
            fi
        fi
    done
}
