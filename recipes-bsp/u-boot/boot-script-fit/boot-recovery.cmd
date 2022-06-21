# try to load recovery image via USB
# since this script is located on USB stick we don't need to start/reset usb
if load usb 0:1 ${loadaddr} Image; then
	echo "booting recovery-image from usb"
	setenv bootargs "$bootargs root=LABEL=RECOVERY ro rootwait reboot=efi,warm no_ima loglevel=4 acpi_enforce_resources=lax;"
	bootm
fi
