#!/bin/sh

################################################################################
# Plugin Section
################################################################################

# Plugin: fat
do_mkfs_vfat() { mkfs.vfat $1 ${2:+-n $2} || return 1; }
do_fsck_vfat() { fsck.vfat -a $1 || return 1; }
do_fsresize_vfat() { return 1; } # currently unsupported

# Plugin: ext2
do_mkfs_ext2() { mkfs.ext2 -O 64bit -F $1 ${2:+-L $2} || return 1; }
do_fsck_ext2() { fsck.ext2 -y -f $1 || return 1; }
do_fsresize_ext2() { resize2fs $1 || return 1; }

# Plugin: ext3
do_mkfs_ext3() { mkfs.ext3 -O 64bit -F $1 ${2:+-L $2} || return 1; }
do_fsck_ext3() { fsck.ext3 -y -f $1 || return 1; }
do_fsresize_ext3() { resize2fs $1 || return 1; }

# Plugin: ext4
do_mkfs_ext4() { mkfs.ext4 -O 64bit -F $1 ${2:+-L $2} || return 1; }
do_fsck_ext4() { fsck.ext4 -y -f $1 || return 1; }
do_fsresize_ext4() { resize2fs $1 || return 1; }

# Plugin: extended
do_mkfs_extended() { return 0; }
do_fsck_extended() { return 0; }
do_fsresize_extended() { return 0; }

################################################################################
# Code Section
################################################################################

toBytes() {
  echo $1 | awk \
      'BEGIN{IGNORECASE = 1}
       function printpower(n,b,p) {printf "%u", n*b^p; next}
       /[0-9]$/{print $1;next};
       /K(iB)?$/{printpower($1,  2, 10)};
       /M(iB)?$/{printpower($1,  2, 20)};
       /G(iB)?$/{printpower($1,  2, 30)};
       /T(iB)?$/{printpower($1,  2, 40)};
       /KB$/{    printpower($1, 10,  3)};
       /MB$/{    printpower($1, 10,  6)};
       /GB$/{    printpower($1, 10,  9)};
       /TB$/{    printpower($1, 10, 12)}'
}

get_cur_dev_node() {
	local dev_node="${dev}${pn}"
	echo "$dev" | grep -q /dev/mmcblk && dev_node="${dev}p${pn}"
	echo "$dev" | grep -q /dev/nvme && dev_node="${dev}p${pn}"
	echo $dev_node
}

get_free_part_info() {
	free_space=$(sfdisk -F ${dev} | head -n1 | cut -d ":" -f2 | cut -d "," -f2 | cut -d " " -f2)
	[ "${free_space}" = "0" ] && return 1
	free_part=$(sfdisk -F ${dev} | tail -n1 | tr -s " " | cut -d " " -f4)
}

create_part() {
	log "Creating partition $pn ($conf_line) ..."

	# Read out information of free partition
	if ! get_free_part_info; then
		log "Error: Creating partition ${pn} failed: Disk space empty!"
		return 1
	fi

	if [ "$part_tabel" = "dos" ]; then
		# Create new DOS partition
		case $fstype in
			'ext4' | 'ext3' | 'ext2') id="83" ;;
			'vfat') id="0c" ;;
			'lvm') id="8e" ;;
			'extended') id="05" ;;
			*) log "Error: Invalid or missing partition type!"; return 1;;
		esac
	elif [ "$part_tabel" = "gpt" ]; then
		# Create new GPT partition
		case $fstype in
			'ext4' | 'ext3' | 'ext2') id="0FC63DAF-8483-4772-8E79-3D69D8477DE4" ;;
			'vfat') id="EBD0A0A2-B9E5-4433-87C0-68B6B72699C7" ;;
			'lvm') id="E6D6D379-F507-44C2-A23C-238F2A3DF928" ;;
			*) log "Error: Invalid or missing partition type!"; return 1;;
		esac
	else
		log "Error: Invalid or missing partition table!"
		return 1
	fi

	[ "$size" = "max" ] && size=""
	start=$(sfdisk -F $dev | tail -n1 | cut -d ' ' -f1)
	part_spec="${start},${size},${id}"
	if ! flock $dev /bin/sh -c "echo $part_spec | sfdisk --no-reread -q -a $dev -W always"; then
		log "Error: Creating partition ${pn} failed!"
		return 1
	fi

	# Format new partition with filesystem
	case "$fstype" in
		'ext4' | 'ext3' | 'ext2' | 'vfat')
			if ! do_mkfs_$fstype $dev_node $label; then
				log "Error: Creating partition ${pn} ($dev_node, $fstype) failed: Filesystem error!"
				return 1
			fi
			;;
		'lvm')
			vgchange -an && pvcreate $dev_node -ff -y -Zy && vgcreate $vgname $dev_node -Zy && vgchange -ay || {
				log "Error: Creating partition ${pn} ($dev_node, $fstype) failed!"
				return 1
			}
			;;
		'extended')
			;;
		*)
			log "Error: Creating partition ${pn} ($dev_node, $fstype) failed: Invalid or missing fstype!"
			log "Error: Creating partition ${pn} ($dev_node, $fstype) failed: Supported fstype: ext4, ext3, ext2, vfat!"
			return 1
			;;
	esac

	log "Creating partition ${pn} ($dev_node, $fstype) successfully done!"

	return 0
}

resize_part() {
	log "Resizing partition $pn ($conf_line) ..."

	# Read out information of free partition
	if ! get_free_part_info; then
		log "Error: Creating partition ${pn} failed: Disk space empty!"
		return 1
	fi

	# Check size against shrinking
	cur_part_size_bytes=$(toBytes $cur_part_size)
	size_bytes=$(toBytes $size)
	if [ $size_bytes -lt $cur_part_size_bytes ]; then
		log "Error: Resizing partition ${pn} failed: Size cannot shrink!"
		return 1
	fi

	# TODO: Check if partition fits on disk

	do_fsck_$fstype $dev_node

	# Resize partition
	[ "$size" = "max" ] && size="+"
	if ! flock $dev /bin/sh -c "echo ,${size} | sfdisk --no-reread -q -N ${pn} ${dev}"; then
		log "Resizing partition ${pn} ($dev_node, $fstype) failed: Partition error!"
		return 1
	fi

	do_fsck_$fstype $dev_node

	# NOTE: We need to mount/unmount the device here to make sure mount time is set
	#       to something before last check time. Both informations are kept in the
	#       superblock of the filesystem. If last_checktime < last_mounttime resizing
	#       via resize2fs will fail with the need to run e2fsck.
	tmp_mp=$(mktemp -d) && mount $dev_node $tmp_mp && umount $tmp_mp && rm -r $tmp_mp || {
		log "Failed to mount ${dev_node}. Resizing will fail - aborting!"
		return 1
	}

	# Sometimes resize2fs complains about missing fsck (especially on intel virtual platforms)
	# so recheck filesystem
	do_fsck_$fstype $dev_node

	# Resize filesystem
	if ! do_fsresize_$fstype $dev_node; then
		log "Resizing partition ${pn} ($dev_node, $fstype) failed: Filesystem error!"
		return 1
	fi
	log "Resizing partition ${pn} ($dev_node, $fstype) successfully done!"

	return 0
}

create_logical_lvm_volume() {
	log "Creating logical LVM volume $lvn ($conf_line) ..."

	if echo ${size} | grep -q "%"; then
		lvcreate -n $lvname -l ${size} $vgname -Zn
	else
		lvcreate -n $lvname -L ${size} $vgname -Zn
	fi

	vgchange -ay
	vgscan --mknodes

	# Format logical volume
	if ! yes | do_mkfs_$fstype /dev/mapper/$vgname-$lvname $label; then
		log "Creating logical LVM volume ${lvn} (/dev/mapper/$vgname-$lvname, $fstype) failed!"
		return 1
	fi

	log "Creating logical LVM volume ${lvn} (/dev/mapper/$vgname-$lvname, $fstype) successfully done!"
}

do_main() {
	sgdisk=$(which sgdisk)
	[ -n "$sgdisk" ] && sgdisk -e $dev

	# Get a comma only separated partition configuration
	conf="$(cat part.cfg | tr -s ', ' ',')"
	cur_parts=$(sfdisk -o Device,Start,End,Sectors,Size,Type -lq ${dev} | sed "1 d" | tr -s " ")
	cur_parts_count=$(echo "$cur_parts" | wc -l)

	part_tabel="$(sfdisk -d ${dev} | grep label: | cut -d' ' -f2)"

	for conf_line in $conf; do
		part=$(get_key_val part $conf_line)
		[ -z "$part" ] && continue

		log "========================================"

		size=$(get_key_val size $conf_line)
		fstype=$(get_key_val fstype $conf_line)
		label=$(get_key_val label $conf_line)
		vgname=$(get_key_val vgname $conf_line)

		[ "$part" == "lvm" -a -z "$fstype" ] && fstype="lvm" # <= Hack to prepare physical LVM volume
		if [ "$part" == "extended" ]; then
			[ "$part_tabel" != "dos" ] && { echo "Skip extended partitions for $part_tabel partition-tables."; continue; }
			[ -z "$fstype" ] && fstype="extended" # <= Hack to prepare extended partitions
		fi

		let pn++

		# Get device node of current partition
		dev_node=$(get_cur_dev_node)

		cur_part_conf=$(echo "$cur_parts" | head -n${pn} | tail -n1)
		# Check partition availability
		if [ ${cur_parts_count} -lt ${pn} ]; then
			create_part || return 1
			continue
		fi

		# Check partition filesystem
		cur_part_fstype=$(lsblk -o NAME,FSTYPE ${dev_node} | sed "1 d" | head -n1 | tr -s " " | cut -d " " -f2 | sed "s/LVM2_member/lvm/")
		if [ "$cur_part_fstype" != "$fstype" ]; then
			log "Error: Partition $dev_node: fstype mismatch ($fstype!=$cur_part_fstype)"
			return 1
		fi

		# Check partition size
		cur_part_size=$(echo $cur_part_conf | cut -d " " -f5)
		if [ "$cur_part_size" != "$size" ]; then
			resize_part || return 1
			continue
		fi

		log "Partition $pn ($dev_node) okay!"
	done

	for conf_line in $conf; do
		lvname=$(get_key_val lvname $conf_line)
		[ -z "$lvname" ] && continue
		log "========================================"

		let lvn++
		vgname=$(get_key_val vgname $conf_line)
		size=$(get_key_val size $conf_line)
		fstype=$(get_key_val fstype $conf_line)
		label=$(get_key_val label $conf_line)

		# Create logical LVM volume
		create_logical_lvm_volume || return 1
	done

	log "========================================"

	return 0
}

do_init() {
	# Check for part.cfg
	[ -r part.cfg ] || {
		log "Invalid or missing part.cfg"
		return 1
	}

	# Retrieve physical device to be modified.
	dev_list=$(grep ^device part.cfg | cut -d'=' -f2)
	for dev in $dev_list; do
		[ -b $dev ] && break
	done

	[ ! $dev ] && {
		log "System device not found in $dev_list"
		return 1
	}
	log "Using system device $dev"

	# Unmount all partitions from the device to be modified ...
	devmounts=$(mktemp)
	grep ^$dev /proc/mounts > $devmounts
	while read line; do
		umount $(cut -d' ' -f2 <<< $line);
	done < $devmounts
}

do_cleanup() {
	# Remount all previously unmounted partitions of the device to be modified.
	if [ -r $devmounts ]; then
		while read line; do
			# NOTE:
			# The device mount takes place in two steps, first as read-only and then as read/write.
			# This is to avoid mount errors for devices already mounted read-only.
			mount $(cut -d' ' -f1 <<< $line) $(cut -d' ' -f2 <<< $line) -t $(cut -d' ' -f3 <<< $line) -o ro
			mount $(cut -d' ' -f1 <<< $line) $(cut -d' ' -f2 <<< $line) -t $(cut -d' ' -f3 <<< $line) -o remount,$(cut -d' ' -f4 <<< $line)
		done < $devmounts
		rm $devmounts
	fi

	# Copy logfile to persistent storage location.
	cp $logfile $apifile.log
}

log() {
	echo "$(basename $apifile): $@"
	echo "$@" >> $logfile
}

get_key_val() {
	echo $2 | tr ', ' '\n' | grep "^$1=" | cut -s -d'=' -f2
}

################################################################################

APP_NAME="Initial-Device-Partition-Manager"

apifile=$(get_key_val apifile $*)
logfile=/tmp/$(basename $apifile).log

log "Starting $APP_NAME ..."

do_init && do_main $@
exit_code=$?

case $exit_code in
	0)
		do_cleanup
		# NOTE: To be compatible with 2.3.x there is a second very same version of the
		#       runscript with the old naming scheme (xxx-initrd-api) - delete both.
		rm $(dirname $apifile)/initrd-api-part-cfg
		rm $(dirname $apifile)/part-cfg-initrd-api
		log "Exiting $APP_NAME successfully!"
		exit 0
		;;
	*)
		do_cleanup
		log "Exiting $APP_NAME erroneous ($exit_code)!"
		exit $exit_code
		;;
esac

# This code should never be reached!
exit 1
