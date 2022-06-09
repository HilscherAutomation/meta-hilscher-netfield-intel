# Create a bootmenu based on boot.cfg files

# menu index count
setexpr mi 0

# Check for SATA/scsi devices.
scsi scan &&
if scsi dev; then
	scsidev_found="1"
	scsidevs="0"
	scsiparts="4 3 2"
fi

setenv basebootargs rootflags=noatime overlayflags=noatime,discard ro rootwait rootdelay=1 roottimeout=10 reboot=efi,warm no_ima loglevel=7 acpi_enforce_resources=lax;

for conf in boot.cfg aboot.cfg rboot.cfg; do
	if test ${scsidev_found} = "1"; then
		for dev in ${scsidevs}; do
			for part in ${scsiparts}; do
				if load scsi ${dev}:${part} ${loadaddr} ${conf}; then
					env import ${loadaddr}
					test -z "${description}" && description="unknown"
					test "${conf}" = "boot.cfg" && type=" "
					test "${conf}" = "aboot.cfg" && type="(ALTERNATIVE)"
					test "${conf}" = "rboot.cfg" && type="(RESCUE)"
					setenv bootmenu_${mi} scsi${dev}: ${description} ${type} = "
						setenv bootargs $basebootargs bootCfg=/dev/sda${part}/${conf};
						load scsi ${dev}:${part} ${loadaddr} ${kernel};
						bootm
					"
					setexpr mi ${mi} + 1
				fi
			done
		done
	fi
done

setenv bootmenu_${mi} FastBoot = "run fastboot"
setexpr mi ${mi} + 1
setenv bootmenu_${mi} Console = "run setup_console"

bootmenu 3

exit $?
