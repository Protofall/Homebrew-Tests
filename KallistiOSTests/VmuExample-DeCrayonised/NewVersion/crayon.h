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

//Checks if the computer running this code is big endian or not
extern uint8_t crayon_misc_is_big_endian();

extern void crayon_misc_correct_endian(uint8_t *buffer, size_t bytes);	//WIP

extern void crayon_misc_encode_to_buffer(uint8_t *buffer, uint8_t *data, size_t bytes);

#define CRAYON_BOOT_MODE 0	//Load assets from cd dir instead of sd

#if defined(_arch_dreamcast)

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

//Only used for port/slot in dreamcast. x is port, y is slot
typedef struct vec2_s8{
	int8_t x, y;
} vec2_s8_t;

vec2_s8_t crayon_savefile_dreamcast_get_port_and_slot(int8_t savefile_device_id);

uint8_t crayon_memory_mount_romdisk(char *filename, char *mountpoint);

#endif

#endif
