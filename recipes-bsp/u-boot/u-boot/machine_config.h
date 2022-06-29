/* config of intel */

/* default is 4 which lead to connection trouble (dhcp/bootp) in some network setups */
#ifdef CONFIG_BOOTP_ID_CACHE_SIZE
	#undef CONFIG_BOOTP_ID_CACHE_SIZE
	#define CONFIG_BOOTP_ID_CACHE_SIZE 10
#endif

/* Platform specific initialization */
#define PLATFORM_INIT \
	"scsi reset; " \
	"part number $plat_dev_if $plat_dev boot plat_boot_part; " \
	"part number $plat_dev_if $plat_dev system plat_system_part; " \
	"usb reset; " \
	"part number $usb_dev_if $usb_dev recovery usb_recovery_part; "

/* Platform specific environment settings */
#define BOARD_CONFIG_EXTRA_ENV_SETTINGS \
	"basebootargs=rootflags=noatime overlayflags=noatime,discard ro rootwait rootdelay=1 roottimeout=10 reboot=efi,warm no_ima loglevel=4 acpi_enforce_resources=lax\0" \
	"plat_dev_if=scsi\0" \
	"plat_dev=0\0" \
	"plat_dev_linux=/dev/sda\0" \
	"usb_dev_if=usb\0" \
	"usb_dev=0\0" \

// ---------------------------

#undef CONFIG_LOADADDR
#define CONFIG_LOADADDR 0x05000000

#undef CONFIG_SYS_LOAD_ADDR
#define CONFIG_SYS_LOAD_ADDR 0x05000000

// Defined like in x86-common.h
#undef INITRD_HIGH
#define INITRD_HIGH "0xffffffffffffffff"

// Defined like in x86-common.h
#undef FDT_HIGH
#define FDT_HIGH "0xffffffffffffffff"

// PLAT_BOOT_COMMAND
//
// This defines a generic platform bootcmd which can also be used by other platforms.
//
// Note:
//   See also PLATFORM_INIT and BOARD_CONFIG_EXTRA_ENV_SETTINGS!
#undef MMC_LOAD_BOOT_SCRIPT
#undef MMCBOOT_COMMAND
#define MMCBOOT_COMMAND PLAT_BOOT_COMMAND
#define PLAT_BOOT_COMMAND  \
	"if test -e ${plat_dev_if} ${plat_dev}:${plat_boot_part} ${script}; then " \
		"echo Found U-Boot script ${script}; " \
		"load ${plat_dev_if} ${plat_dev}:${plat_boot_part} ${scriptaddr} ${script} && "START_SCRIPT"; " \
		"if test $? != 0; then " \
			"echo SCRIPT FAILED: continuing...; " \
		"fi; " \
	"fi; "

// USBBOOT_COMMAND
//
// This defines a generic usb bootcmd which can also be used by other platforms.
//
// Note:
//   See also PLATFORM_INIT and BOARD_CONFIG_EXTRA_ENV_SETTINGS!
#undef USB_LOAD_BOOT_SCRIPT
#undef USBBOOT_COMMAND
#define USBBOOT_COMMAND  \
	"if test -e ${usb_dev_if} ${usb_dev}:${usb_recovery_part} ${script}; then " \
		"echo Found U-Boot script ${script}; " \
		"load ${usb_dev_if} ${usb_dev}:${usb_recovery_part} ${scriptaddr} ${script} && "START_SCRIPT"; " \
		"if test $? != 0; then " \
			"echo SCRIPT FAILED: continuing...; " \
		"fi; " \
	"fi; "
