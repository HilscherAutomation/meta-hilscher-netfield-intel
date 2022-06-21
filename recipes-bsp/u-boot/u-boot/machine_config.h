/* config of rpi */

/* default is 4 which lead to connection trouble (dhcp/bootp) in some network setups */
#ifdef CONFIG_BOOTP_ID_CACHE_SIZE
	#undef CONFIG_BOOTP_ID_CACHE_SIZE
	#define CONFIG_BOOTP_ID_CACHE_SIZE 10
#endif

/* just dump hab status at the beginning */
#define PLATFORM_INIT \
	" "

/* definition not necesarry since control is done via keyboard, screen, serial... */
#define BOARD_CONFIG_EXTRA_ENV_SETTINGS \
	" "

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

#define LOAD_BOOT_SCRIPT \
	"fatload scsi ${mmcdev}:${mmcpart} ${scriptaddr} ${script} && "START_SCRIPT"; "

#define BOOT_COMMAND  \
	"scsi reset; " \
	"for part in ${mmc_parts}; do " \
		"if test -e scsi ${mmcdev}:${part} ${script}; then " \
			"echo Found U-Boot script ${script}; " \
			MMC_LOAD_BOOT_SCRIPT \
			"if test $? != 0; then " \
				"echo SCRIPT FAILED: continuing...; " \
			"fi; " \
		"fi; " \
	"done;"

#undef MMC_LOAD_BOOT_SCRIPT
#define MMC_LOAD_BOOT_SCRIPT LOAD_BOOT_SCRIPT

#undef MMCBOOT_COMMAND
#define MMCBOOT_COMMAND BOOT_COMMAND
