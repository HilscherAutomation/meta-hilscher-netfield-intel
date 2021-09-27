#!/bin/bash

echo "Extracting kernel and bootloader from image"

# Copy bootloader
mkdir -p EFI/BOOT
cp -L ${DEPLOY_DIR_IMAGE}/boot-files/bootx64.efi EFI/BOOT

# Copy kernel
cp ${DEPLOY_DIR_IMAGE}/fitImage-core-image-minimal-*.bin ./fitImage

# Copy boot script
cp ${DEPLOY_DIR_IMAGE}/boot-files/boot-${image_type}.scr ./boot.scr

VERSION_ID=${FIRMWARE_VERSION}
echo ${VERSION_ID} > VERSION

if [ "${image_type}" == "production_scan" ]; then
  # Copy production_scan_image squash fs
  cp ${ROOTFS} rootfs.img
  openssl dgst ${engine_params} -sha512 -sign ${signing_key} -out rootfs.img.sig rootfs.img
fi

# delete temporary extracted files
rm -rf boot_tmp
