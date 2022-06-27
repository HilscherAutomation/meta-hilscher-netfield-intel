# Generic boot script used by a USB recovery stick.

if load ${usb_dev_if} ${usb_dev}:${usb_recovery_part} ${loadaddr} Image; then
	echo "booting recovery-image from usb"
	setenv bootargs "$bootargs root=LABEL=RECOVERY ro rootwait reboot=efi,warm no_ima loglevel=4 acpi_enforce_resources=lax;"
	bootm
fi
