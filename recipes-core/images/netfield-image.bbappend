IMAGE_INSTALL:remove = "linux-firmware"
IMAGE_INSTALL:append = " linux-firmware-ibt linux-firmware-iwlwifi-7260 linux-firmware-iwlwifi-7265 linux-firmware-iwlwifi-7265d"
IMAGE_INSTALL:append = " linux-firmware-rtl8723"
IMAGE_INSTALL:append = " linux-firmware-ath10k linux-firmware-qca"
IMAGE_INSTALL:append = " kernel-module-netanalyzer libnetana netanalyzer-firmware netanalyzer-bsl"
IMAGE_INSTALL:append = " uionetx libcifx cifxtun"

IMAGE_INSTALL:remove = "kernel-initramfs kernel-image kernel-image-bzimage"

DEPENDS:remove = "grub-efi"

IMAGE_FSTYPES:append:niot-e-vm-en = " ova wic.qcow2"
# Install open-vm-tools for better VMWare integration
IMAGE_INSTALL:append:niot-e-vm-en = " open-vm-tools"
CONVERSION_CMD:qcow2 = "qemu-img convert -O qcow2 ${IMAGE_NAME}${IMAGE_NAME_SUFFIX}.${type} ${IMAGE_NAME}${IMAGE_NAME_SUFFIX}.${type}.qcow2 && \
                        qemu-img resize ${IMAGE_NAME}${IMAGE_NAME_SUFFIX}.${type}.qcow2 ${OVA_DISKIMAGE_SIZE}"

do_image_wic[depends] += "u-boot:do_deploy"

hd_path_squashfs = "${HDEPLOY_PATH_EXTRAS}/base_image"

do_hilscher_deploy:append() {
        for file in $(find ${IMGDEPLOYDIR} -type l -name "*.squashfs"); do
                cp -a $(readlink -f $file) ${hd_path_squashfs}
        done
}
do_hilscher_deploy[cleandirs] += " ${hd_path_squashfs}/ "
