require intel-corei7-64.conf

SOC_FAMILY = "intel-corei7-64"
include conf/machine/include/soc-family.inc

#BSP_DEPLOYSCRIPT_DIR??="${DYNAMIC_LAYER_DIR_BASE}/meta-intel/scripts/deploy"
BSP_DEPLOYSCRIPT_DIR ??= "${TOPDIR}/../meta-hilscher-netfield-intel/scripts/deploy"

INITRAMFS_FSTYPES = "cpio"

MACHINE_EXTRA_RRECOMMENDS_remove += "linux-firmware"

EFI_PROVIDER = "barebox"

MACHINE_FEATURES_remove += "alsa"
MACHINE_FEATURES_append += "3g"

#############################################################################
IMAGE_CLASSES_append += "hilscher_image_types"
IMAGE_FSTYPES += "squashfs wic.bz2 swu licenses.tar.bz2"

#############################################################################
# Preferred components/settings
#############################################################################
PREFERRED_PROVIDER_virtual/kernel = "linux-stable-rt"
PREFERRED_VERSION_linux-stable-rt = "5.4.%"
PREFERRED_VERSION_kernel-initramfs-bundle = "5.4"

INITRAMFS_IMAGE_BUNDLE = "1"
INITRAMFS_IMAGE ?= "core-image-minimal-initramfs"
HILSCHER_RESCUE_IMAGE ??= "hilscher-rescue-image"

#############################################################################
# Keys
#############################################################################
PLATFORM_SIGN ??= "1"
PLATFORM_KEYDIR ??= "${KEYS_DIR}/uefi"
PLATFORM_KEYNAME ??= "db"

#KEYS_DIR ??= "${DYNAMIC_LAYER_DIR_BASE}/meta-intel/keys/${MACHINE}"
KEYS_DIR .= "/${MACHINE}"
KEYS_IMAGE_SIGN_PRIV ??= "${KEYS_DIR}/uefi/db.key"
KEYS_IMAGE_SIGN_CERT ??= "${KEYS_DIR}/uefi/db.crt"
KEYS_IMAGE_SIGN_CERT_DER ??= "${KEYS_DIR}/uefi/db.crt.der"

#############################################################################
# Contents of boot partition (excluding rootfs)
#############################################################################
IMAGE_BOOT_FILES = "bootx64.efi;efi/boot/bootx64.efi"

#############################################################################
# WIC images
#############################################################################
WKS_FILE = "hilscher-gpt-boot-rescue-system-lvm.wks.in"

WIC_BOOT_PART_CONTENT ??= "${IMAGE_BOOT_FILES}"
WIC_RESCUE_PART_CONTENT ??= ""
WIC_SYSTEM_PART_CONTENT ??= "${IMAGE_LINK_NAME}.squashfs boot.cfg boot-files/bzImage;bzImage initrd-api-part-cfg"

#############################################################################
# SWU images
#############################################################################
SWU_SCRIPTS += "swupdate/helper.lua;helper.lua"

SWU_RSYNC_PART_UPDATE ??= "boot system"
SWU_BOOT_PART_CONTENT ??= "${IMAGE_BOOT_FILES}"
SWU_SYSTEM_PART_CONTENT ??= "${IMAGE_LINK_NAME}.squashfs nboot.cfg boot-files/bzImage;bzImage"

#############################################################################
# System Partitioning
#############################################################################
# Physical device of system partitions
PHYSICAL_SYSTEM_DEVICE ??= "/dev/sda /dev/hda"
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

#############################################################################
# rootfs overlay
#############################################################################
HILSCHER_OVERLAY ??= "/dev/mapper/data-data"
HILSCHER_OVERLAYTARGETS ??= "${NETIOT_OVERLAY_DIRS}"
