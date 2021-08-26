OVA_DISKIMAGE_SIZE ??= "20G"
OVA_CORE_NUMBER ??= "4"
OVA_RAM_SIZE ??= "2048"
OVA_PRODUCT ??= "netFIELD OS"
OVA_VENDOR ??= "Hilscher Gesellschaft fuer Systemautomation mbH"
OVA_VERSION ??= "${FIRMWARE_VERSION}"
OVA_VENDOR_URL ??= "https://www.hilscher.com"

OVA_TEMPLATE ??= "${NETFIELD_INTEL_BASE}/files/ovf.in"

inherit sign-wrapper

IMAGE_CMD_ova () {
    # create working directory
    rm -rf  ${WORKDIR}/ova-image
    mkdir -p ${WORKDIR}/ova-image

    REAL_IMAGE=$(readlink -f ${IMGDEPLOYDIR}/${IMAGE_BASENAME}-${MACHINE}.wic)

    # Create disk image
    cp ${REAL_IMAGE} ${WORKDIR}/ova-image/img.resized
    qemu-img resize ${WORKDIR}/ova-image/img.resized ${OVA_DISKIMAGE_SIZE}
    qemu-img convert -O vmdk -o subformat=streamOptimized ${WORKDIR}/ova-image/img.resized ${WORKDIR}/ova-image/${IMAGE_NAME}-disk1.vmdk.pre
    vmdk-convert ${WORKDIR}/ova-image/${IMAGE_NAME}-disk1.vmdk.pre ${WORKDIR}/ova-image/${IMAGE_NAME}-disk1.vmdk
    rm ${WORKDIR}/ova-image/img.resized ${WORKDIR}/ova-image/${IMAGE_NAME}-disk1.vmdk.pre

    # Create machine description
    sed -e "s|@@NAME@@|${OVA_PRODUCT}|g" \
        -e "s|@@VMDK_NAME@@|${IMAGE_NAME}|g" \
        -e "s|@@VMDK_FILE_SIZE@@|$(du -b ${WORKDIR}/ova-image/${IMAGE_NAME}-disk1.vmdk | cut -f1)|g" \
        -e "s|@@VMDK_CAPACITY@@|$(numfmt --from=iec ${OVA_DISKIMAGE_SIZE})|g" \
        -e "s|@@NUM_CPUS@@|${OVA_CORE_NUMBER}|g" \
        -e "s|@@MEM_SIZE@@|${OVA_RAM_SIZE}|g" \
        -e "s|@@OVA_PRODUCT@@|${OVA_PRODUCT}|g" \
        -e "s|@@OVA_VENDOR@@|${OVA_VENDOR}|g" \
        -e "s|@@OVA_VENDOR_URL@@|${OVA_VENDOR_URL}|g" \
        -e "s|@@OVA_VERSION@@|${OVA_VERSION}|g" \
        -e "s|@@VBOX_MACHINE_UUID@@|$(uuidgen)|g" \
        -e "s|@@VBOX_IMAGE_UUID@@|$(uuidgen)|g" \
        ${OVA_TEMPLATE} >  ${WORKDIR}/ova-image/${IMAGE_NAME}.ovf

    # Create manifest
    for F in ${IMAGE_NAME}.ovf ${IMAGE_NAME}-disk1.vmdk; do
        SHA256SUM=$(sha256sum ${WORKDIR}/ova-image/${F} | cut -d ' ' -f1)
        echo "SHA256(${F})= ${SHA256SUM}" >> ${WORKDIR}/ova-image/${IMAGE_NAME}.mf
    done

    # Sign ovf manually
    setup_sign_wrapper_env "${PLATFORM_KEYNAME}"
    export OPENSSL_SIGN_WRAPPER_ADD_OPTIONS="-hex"
    openssl_sign_wrapper "${PLATFORM_KEYNAME}" "sha256" ${WORKDIR}/ova-image/${IMAGE_NAME}.mf
    sed -e 's/RSA-SHA256/SHA256/' \
        -e 's;${WORKDIR}/ova-image/;;' \
        ${WORKDIR}/ova-image/${IMAGE_NAME}.mf.sig \
        > ${WORKDIR}/ova-image/${IMAGE_NAME}.cert
    rm ${WORKDIR}/ova-image/${IMAGE_NAME}.mf.sig

    sign_wrapper_copy_certificate ${B}/tmpcert "pem"
    cat ${B}/tmpcert >> ${WORKDIR}/ova-image/${IMAGE_NAME}.cert
    rm ${B}/tmpcert

    rm -f ${IMGDEPLOYDIR}/${IMAGE_BASENAME}*.ova

    # Create ova
    tar --mode=0644 --owner=65534 --group=65534 -cf \
        ${IMGDEPLOYDIR}/${IMAGE_NAME}.ova \
        -C ${WORKDIR}/ova-image ${IMAGE_NAME}.ovf ${IMAGE_NAME}.mf ${IMAGE_NAME}.cert ${IMAGE_NAME}-disk1.vmdk

    ln -sf ${IMAGE_NAME}.ova ${IMGDEPLOYDIR}/${IMAGE_LINK_NAME}.ova
}

IMAGE_TYPEDEP_ova = "wic"
IMAGE_TYPES_append += "ova"
do_image_ova[depends] += "qemu-system-native:do_populate_sysroot"
do_image_ova[depends] += "open-vmdk-native:do_populate_sysroot"
do_image_ova[depends] += "util-linux-native:do_populate_sysroot"
do_image_ova[file-checksums] += "${OVA_TEMPLATE}:True"
