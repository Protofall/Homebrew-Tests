#include "crayon.h"

extern uint8_t crayon_misc_is_big_endian(){
	int a = 1;
	return !((char*)&a)[0];
}

extern void crayon_misc_correct_endian(uint8_t *buffer, size_t bytes){
	printf("ENDIANESS CORRECTOR INCOMPLETE\n");
	return;
}

extern void crayon_misc_encode_to_buffer(uint8_t *buffer, uint8_t *data, size_t bytes){
	while(bytes--){*buffer++ = *data++;}

	return;
}

#ifdef _arch_dreamcast

#if CRAYON_BOOT_MODE == 1

void unmount_ext2_sd(){
	fs_ext2_unmount("/sd");
	fs_ext2_shutdown();
	sd_shutdown();
}

int mount_ext2_sd(){
	kos_blockdev_t sd_dev;
	uint8 partition_type;

	// Initialize the sd card if its present
	if(sd_init()){
		return 1;
	}

	// Grab the block device for the first partition on the SD card. Note that
	// you must have the SD card formatted with an MBR partitioning scheme
	if(sd_blockdev_for_partition(0, &sd_dev, &partition_type)){
		return 2;
	}

	// Check to see if the MBR says that we have a Linux partition
	if(partition_type != 0x83){
		return 3;
	}

	// Initialize fs_ext2 and attempt to mount the device
	if(fs_ext2_init()){
		return 4;
	}

	//Mount the SD card to the sd dir in the VFS
	if(fs_ext2_mount("/sd", &sd_dev, MNT_MODE)){
		return 5;
	}
	return 0;
}

#endif

vec2_s8_t crayon_savefile_dreamcast_get_port_and_slot(int8_t save_device_id){
	vec2_s8_t values = {-1,-1};
	if(save_device_id < 0 || save_device_id >= 8){return values;}
	
	if(save_device_id % 2 == 0){
		values.y = 1;
	}
	else{
		values.y = 2;
	}

	values.x = save_device_id / 2;

	return values;
}

uint8_t crayon_memory_mount_romdisk(char *filename, char *mountpoint){
	void *buffer;

	ssize_t size = fs_load(filename, &buffer); // Loads the file "filename" into RAM

	if(size == -1){
		return 1;
	}
	
	fs_romdisk_mount(mountpoint, buffer, 1); // Now mount that file as a romdisk, buffer will be freed when romdisk is unmounted
	return 0;
}

#endif
