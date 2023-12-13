/* config of intel */

/* default is 4 which lead to connection trouble (dhcp/bootp) in some network setups */
#ifdef CONFIG_BOOTP_ID_CACHE_SIZE
	#undef CONFIG_BOOTP_ID_CACHE_SIZE
	#define CONFIG_BOOTP_ID_CACHE_SIZE 10
#endif

/* Platform specific initialization */
#define PLATFORM_INIT \
	"setenv basebootargs rootflags=noatime overlayflags=noatime,discard ro rootwait rootdelay=1 roottimeout=10 reboot=efi,warm no_ima loglevel=4 acpi_enforce_resources=lax; " \
	"setenv fdt_addr ${loadaddr}; " \
	"scsi reset; " \
	"part number $plat_dev_if $plat_dev boot plat_boot_part; " \
	"part number $plat_dev_if $plat_dev system plat_system_part; " \
	"usb reset; " \
	"part number $usb_dev_if $usb_dev recovery usb_recovery_part; "

/* Platform specific environment settings */
#define BOARD_CONFIG_EXTRA_ENV_SETTINGS \
	"basebootargs=dummy - see platform_init\0" \
	"plat_dev_if=scsi\0" \
	"plat_dev=0\0" \
	"plat_dev_linux=/dev/sda\0" \
	"usb_dev_if=usb\0" \
	"usb_dev=0\0" \

// ---------------------------

#undef CONFIG_SYS_LOAD_ADDR
#define CONFIG_SYS_LOAD_ADDR 0x05000000

// Defined like in x86-common.h
#undef INITRD_HIGH
#define INITRD_HIGH "0xffffffffffffffff"

// Defined like in x86-common.h
#undef FDT_HIGH
#define FDT_HIGH "0xffffffffffffffff"
