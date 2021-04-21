require intel-corei7-64.conf

SOC_FAMILY = "intel-corei7-64"
include conf/machine/include/soc-family.inc

IMAGE_FSTYPES="squashfs wic.bz2"

#KEYS_DIR?="${DYNAMIC_LAYER_DIR_BASE}/meta-intel/keys/${MACHINE}"
KEYS_DIR .= "/${MACHINE}"
KEYS_IMAGE_SIGN_PRIV?="${KEYS_DIR}/uefi/db.key"
KEYS_IMAGE_SIGN_CERT?="${KEYS_DIR}/uefi/db.crt"
KEYS_IMAGE_SIGN_CERT_DER?="${KEYS_DIR}/uefi/db.crt.der"

#BSP_DEPLOYSCRIPT_DIR?="${DYNAMIC_LAYER_DIR_BASE}/meta-intel/scripts/deploy"
BSP_DEPLOYSCRIPT_DIR ?= "${TOPDIR}/../meta-hilscher-netfield-intel/scripts/deploy"

INITRAMFS_FSTYPES = "cpio"
INITRAMFS_IMAGE_BUNDLE = "1"
INITRAMFS_IMAGE = "core-image-minimal-initramfs"

PREFERRED_PROVIDER_virtual/kernel = "linux-stable-rt"
PREFERRED_VERSION_linux-stable-rt = "5.4.%"
PREFERRED_VERSION_kernel-initramfs-bundle = "5.4"

MACHINE_EXTRA_RRECOMMENDS_remove += "linux-firmware"

EFI_PROVIDER = "barebox"

MACHINE_FEATURES_remove += "alsa"
MACHINE_FEATURES_append += "3g"

IMAGE_BOOT_FILES="bootx64.efi;efi/boot/bootx64.efi"

#-----------------------------------------------
require conf/machine/hilscher-machine-vars.conf

PLATFORM_SIGN ?= "1"
PLATFORM_KEYDIR ?= "${KEYS_DIR}/uefi"
PLATFORM_KEYNAME ?= "db"

PART_SCHEME ?= "boot-rescue-system-lvm"
WKS_FILE = "${PART_SCHEME_DIRS}/${PART_SCHEME}/hilscher-wic-image.wks.in"
HILSCHER_RESCUE_IMAGE ?= "hilscher-rescue-image"
WIC_EXTRA_LINES = "bootloader --ptable gpt"
WIC_BOOT_PART_BASE_CONTENT ?= "${IMAGE_BOOT_FILES}"
WIC_BOOT_PART_EXTRA_CONTENT ?= ""
WIC_RESCUE_PART_EXTRA_CONTENT ?= "boot.cfg boot-files/bzImage;bzImage initrd-api-part-cfg;initrd-api"
WIC_SYSTEM_PART_EXTRA_CONTENT ?= "boot.cfg boot-files/bzImage;bzImage"
SWU_SYSTEM_PART_EXTRA_CONTENT ?= "nboot.cfg boot-files/bzImage;bzImage"

# Size of boot partition
IMAGE_PART_BOOT_SIZE ??= "16M"
# Size of rescue partition
IMAGE_PART_RESCUE_SIZE ??= "128M"
# Size of system partition
IMAGE_PART_SYSTEM_SIZE ??= "1G"
# Size of data partition
IMAGE_PART_DATA_SIZE ??= "max"
# Size of logical volume "data" in userdata partition
PART_DATA_LV_DATA_SIZE ??= "60%VG"
# Size of logical volume "backup" in userdata partition
PART_DATA_LV_BACKUP_SIZE ??= "35%VG"
# Size of logical volume "snapshot" in userdata partition
PART_DATA_LV_SNAPSHOT_SIZE ??= "5%VG"

IMAGE_CLASSES_append += "hilscher_image_types"

HILSCHER_OVERLAY ?= "/dev/mapper/data-data"
HILSCHER_OVERLAYTARGETS ?= "${NETIOT_OVERLAY_DIRS}"
#-----------------------------------------------
