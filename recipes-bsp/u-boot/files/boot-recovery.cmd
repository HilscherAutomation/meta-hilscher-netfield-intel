echo "booting recovery-image from usb"

setenv bootargs "$bootargs root=LABEL=RECOVERY ro rootwait reboot=efi,warm no_ima loglevel=4 acpi_enforce_resources=lax;
usb dev 0

fatload usb 0:1 $loadaddr bzImage
zboot $loadaddr
