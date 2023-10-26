KERNEL_FEATURES_append += "features/media/media.scc features/media/media-usb-webcams.scc \
                           features/scsi/scsi.scc features/scsi/disk.scc cfg/efi-ext.scc \
                           features/bluetooth/bluetooth.scc features/apparmor/apparmor.scc"
KERNEL_FEATURES_append_niot-e-vm-en += "cfg/vmware-guest.scc cfg/virtio.scc"

SRC_URI_append += "file://enable_hidraw.cfg"
SRC_URI_append_niot-e-vm-en += "file://vm_fusion_lan.cfg"
