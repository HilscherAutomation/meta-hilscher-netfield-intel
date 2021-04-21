# For some reason this recipes has a direct dependency into core-image-minimal-initramfs
# and triggers a rebuild on core-image-minimal-initramfs and kernel bundle
# after switching machines back and forth
# Hash for dependent task imagescore-image-minimal-initramfs.bb.do_image_complete changed from a to b
#  Hash for dependent task imagescore-image-minimal-initramfs.bb.do_image changed from c to d
#    Hash for dependent task systemdsystemd-serialgetty.bb.do_populate_lic changed from e to f
#      basehash changed from g to h
#      Variable MACHINE value changed from 'hilscher-ntib100' to 'hilscher-ntijcxgb'
#      Variable SSTATE_MANMACH value changed from 'hilscher_ntib100' to 'hilscher_ntijcxgb'
do_populate_lic[vardepsexclude] += "MACHINE SSTATE_MANMACH"
