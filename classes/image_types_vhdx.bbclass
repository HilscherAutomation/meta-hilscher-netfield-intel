OVA_DISKIMAGE_SIZE ??= "20G"

inherit sign-wrapper

IMAGE_CMD:vhdx () {
    # create working directory
    rm -rf  ${WORKDIR}/vhdx-image
    mkdir -p ${WORKDIR}/vhdx-image

    REAL_IMAGE=$(readlink -f ${IMGDEPLOYDIR}/${IMAGE_LINK_NAME}.wic)

    # Create disk image
    cp ${REAL_IMAGE} ${WORKDIR}/vhdx-image/img.resized
    qemu-img resize ${WORKDIR}/vhdx-image/img.resized ${OVA_DISKIMAGE_SIZE}
    qemu-img convert -O vhdx ${WORKDIR}/vhdx-image/img.resized ${IMGDEPLOYDIR}/${IMAGE_NAME}.vhdx
    ln -sf ${IMAGE_NAME}.vhdx ${IMGDEPLOYDIR}/${IMAGE_LINK_NAME}.vhdx

    rm ${WORKDIR}/vhdx-image/img.resized
}

IMAGE_TYPEDEP:vhdx = "wic"
IMAGE_TYPES:append = " vhdx"
do_image_vhdx[depends] += "qemu-system-native:do_populate_sysroot"
