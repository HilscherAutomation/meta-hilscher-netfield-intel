# include the user space intel microcode loading support in the generated images.
MACHINE_ESSENTIAL_EXTRA_RDEPENDS_append = "${@bb.utils.contains('MACHINE_FEATURES', 'intel-ucode', ' intel-microcode', '', d)} thermald"

# recommended extra packages common to all intel machines
MACHINE_EXTRA_RRECOMMENDS_append = " kernel-modules linux-firmware"
