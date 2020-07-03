#ifndef CRAYON_H
#define CRAYON_H

//This file contains functions and defines normally found in crayon

#if defined(__APPLE__) || defined(__linux__) || defined(_WIN32)
	#define _arch_pc
#endif

#if !(defined(_arch_dreamcast) || defined(_arch_pc))
	#error "UNSUPPORTED ARCHITECTURE/PLATFORM"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> //For the uintX_t types

//Only used for slot/port in dreamcast. x is port, y is slot
typedef struct vec2_s8{
	int8_t x, y;
} vec2_s8_t;

//Checks if the computer running this code is big endian or not
extern uint8_t crayon_misc_is_big_endian();

#define CRAYON_BOOT_MODE 0	//Load assets from cd dir instead of sd

#ifdef _arch_dreamcast
#include <kos/fs_romdisk.h> //For romdisk swapping

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

#endif
