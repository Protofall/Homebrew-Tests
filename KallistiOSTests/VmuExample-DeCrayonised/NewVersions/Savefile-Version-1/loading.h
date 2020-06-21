#ifndef MY_LOADING_H
#define MY_LOADING_H

#define CRAYON_BOOT_MODE 0	//Load assets from cd dir instead of sd

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> //For the uintX_t types

#if CRAYON_BOOT_MODE == 1
	//For mounting the sd dir
	#include <dc/sd.h>
	#include <kos/blockdev.h>
	#include <ext2/fs_ext2.h>

	#define MNT_MODE FS_EXT2_MOUNT_READWRITE	//Might manually change it so its not a define anymore

	void unmount_ext2_sd();
	int mount_ext2_sd();
#endif

	uint8_t crayon_memory_mount_romdisk(char *filename, char *mountpoint);

#endif