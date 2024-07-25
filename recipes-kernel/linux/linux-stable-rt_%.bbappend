KERNEL_FEATURES:append = " features/scsi/scsi.scc features/scsi/disk.scc cfg/efi-ext.scc \
                           features/bluetooth/bluetooth.scc features/apparmor/apparmor.scc \
                           features/media/media.scc features/media/media-usb-webcams.scc"
KERNEL_FEATURES:append:netfield-compact-x86c = " features/net/stmicro/stmmac.scc"
KERNEL_FEATURES:append:niot-e-vm-en = " cfg/vmware-guest.scc cfg/virtio.scc"


SRC_URI:append = " \
    file://enable_hidraw.cfg \
    file://iscsi_fix.cfg     \
    file://efi_stub.cfg      \
    file://kpti.cfg          \
    file://retpoline.cfg     \
    file://uio.cfg           \
    file://gpio_sysfs.cfg    \
"
SRC_URI:append:niot-e-tijcx-gb = " \
    file://baytrail.cfg  \
    file://rtl8723be.cfg \
    file://ath10k.cfg    \
"
SRC_URI:append:netfield-compact-x86c = " file://intel_gbe.cfg"
SRC_URI:append:niot-e-vm-en = " file://vm_fusion_lan.cfg file://enable_hyperv.cfg"
SRC_URI:append:netfield-unity = " file://nct6775.cfg"

inherit uefisign sign-wrapper

# Sign kernel image
do_compile:append() {
        uefisign_files ${KERNEL_OUTPUT_DIR}/bzImage
}

# Sign bundled kernel image
do_bundle_initramfs:append() {
        uefisign_files ${KERNEL_OUTPUT_DIR}/bzImage.initramfs
}

do_hilscher_deploy() {
        kernel=$(find ${DEPLOYDIR} -type l -name "bzImage-initramfs-${MACHINE}.bin")
        cp -a $(readlink -f $kernel) "${hd_path}/"
}
