IMAGE_INSTALL_remove += "linux-firmware"
IMAGE_INSTALL_append += "linux-firmware-ibt linux-firmware-iwlwifi-7260 linux-firmware-iwlwifi-7265 linux-firmware-iwlwifi-7265d"
IMAGE_INSTALL_append += "linux-firmware-rtl8723"
IMAGE_INSTALL_append += "kernel-module-netanalyzer libnetana netanalyzer-firmware netanalyzer-bsl"
IMAGE_INSTALL_append += "uionetx libcifx cifxtun"

IMAGE_INSTALL_remove += "kernel-initramfs kernel-image kernel-image-bzimage"

DEPENDS_remove = "grub-efi"

IMAGE_FSTYPES_append_niot-e-vm-en += "ova wic.qcow2"
# Install open-vm-tools for better VMWare integration
IMAGE_INSTALL_append_niot-e-vm-en += "open-vm-tools"
CONVERSION_CMD_qcow2 = "qemu-img convert -O qcow2 ${IMAGE_NAME}${IMAGE_NAME_SUFFIX}.${type} ${IMAGE_NAME}${IMAGE_NAME_SUFFIX}.${type}.qcow2 && \
                        qemu-img resize ${IMAGE_NAME}${IMAGE_NAME_SUFFIX}.${type}.qcow2 ${OVA_DISKIMAGE_SIZE}"

do_image_wic[depends] += "barebox:do_deploy kernel-initramfs-bundle:do_deploy"
