# Generic debug boot script used on the platform device.

# menu index count
setexpr mi 0

# Create a bootmenu based on boot.cfg files
for part in ${plat_system_part}; do
	for conf in boot.cfg aboot.cfg; do
		if load ${plat_dev_if} ${plat_dev}:${part} ${loadaddr} ${conf}; then
			env import ${loadaddr}
			test -z "${description}" && description="unknown"
			test "${conf}" = "boot.cfg" && type=" "
			test "${conf}" = "aboot.cfg" && type="(ALTERNATIVE)"
			setenv bootmenu_${mi} ${plat_dev_if}${plat_dev_}: ${description} ${type} = "
				setenv bootargs ${basebootargs} bootCfg=${plat_dev_linux}${part}/${conf} loglevel=7;
				load ${plat_dev_if} ${plat_dev}:${part} ${loadaddr} ${kernel};
				bootm
			"
			setexpr mi ${mi} + 1
		fi
	done
done

setenv bootmenu_${mi} FastBoot = "run fastboot"

setexpr mi ${mi} + 1
setenv bootmenu_${mi} Console = "run setup_console"

bootmenu 3

exit $?
