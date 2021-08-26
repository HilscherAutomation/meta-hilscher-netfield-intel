#!/bin/bash

echo "Extracting kernel and bootloader from image"

# Copy bootloader
mkdir -p EFI/BOOT
mkdir -p boot/loader/entries
cp -L ${DEPLOY_DIR_IMAGE}/bootx64.efi EFI/BOOT

# Copy kernel
mkdir -p boot
cp ${DEPLOY_DIR_IMAGE}/boot-files/bzImage* boot/

VERSION_ID=${FIRMWARE_VERSION}
echo ${VERSION_ID} > VERSION

if [ "${image_type}" == "update" ]; then
  entry_title="USB: Hilscher IoT Platform update - V${VERSION_ID}"
  root_options="root=LABEL=UPDATE"
  add_options=""
elif [ "${image_type}" == "recovery" ]; then
  entry_title="USB: Hilscher IoT Platform recovery - V${VERSION_ID}"
  root_options="root=LABEL=RECOVERY"
  add_options=""
elif [ "${image_type}" == "production" ]; then
  entry_title="USB: Hilscher IoT Platform production - V${VERSION_ID}"
  root_options="root=LABEL=RECOVERY"
  add_options="production"
elif [ "${image_type}" == "production_scan" ]; then
  entry_title="Production: Scan Device - V${VERSION_ID}"
  root_options="root=LABEL=RECOVERY/rootfs.img rootflags=ro"
  add_options="production"

  # Copy production_scan_image squash fs
  cp ${ROOTFS} rootfs.img
  openssl dgst ${engine_params} -sha512 -sign ${signing_key} -out rootfs.img.sig rootfs.img
fi

if [ "${grub_default}" = "1" ]; then
  console_options="console=${serial_port},115200n8"
  title_add="(console on UART)"
else
  console_options=""
  title_add=""
fi

cat <<EOF >> boot/loader/entries/boot1.conf
title ${entry_title} ${title_add}
version V${VERSION_ID}
options ${root_options} rootwait ${add_options} ${console_options} reboot=efi,warm no_ima loglevel=4 acpi_enforce_resources=lax
linux bzImage
EOF

openssl dgst ${engine_params} -sha512 -sign ${signing_key} -out boot/loader/entries/boot1.conf.sig boot/loader/entries/boot1.conf

# delete temporary extracted files
rm -rf boot_tmp
