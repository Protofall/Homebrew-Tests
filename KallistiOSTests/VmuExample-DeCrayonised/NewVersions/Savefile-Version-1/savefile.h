#ifndef SAVEFILE_CRAYON_H
#define SAVEFILE_CRAYON_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> //For the uintX_t types

#ifdef _arch_dreamcast
#include <dc/maple/vmu.h>
#include <dc/vmu_pkg.h>
#include <dc/vmufs.h>
#include <kos/fs.h>
#endif

#include "crayon.h"

//This is never accessed directly, but it will contain all of you variables that will get saved
typedef struct crayon_savefile_data{
	uint8_t *u8;
	uint16_t *u16;
	uint32_t *u32;
	int8_t *s8;
	int16_t *s16;
	int32_t *s32;
	float *floats;
	double *doubles;
	char *chars;

	uint16_t lengths[9];	//The lengths are in the order they appear above
} crayon_savefile_data_t;

//When you remove a var, these determine how we handle it
	//0 is forget the old value, all the others combine it with an existing var
	//5 is to override the value of the dest var with the value of the deleted var
		//(Good if you want to change a var size)
#define CRAY_SF_REMOVE_CMD_DEFAULT  0
#define CRAY_SF_REMOVE_CMD_ADD      1
#define CRAY_SF_REMOVE_CMD_SUBTRACT 2
#define CRAY_SF_REMOVE_CMD_MULTIPLY 3
#define CRAY_SF_REMOVE_CMD_DIVIDE   4
#define CRAY_SF_REMOVE_CMD_REPLACE  5

//Var types the user passes into functions
#define CRAY_TYPE_UINT8  0
#define CRAY_TYPE_UINT16 1
#define CRAY_TYPE_UINT32 2
#define CRAY_TYPE_SINT8  3
#define CRAY_TYPE_SINT16 4
#define CRAY_TYPE_SINT32 5
#define CRAY_TYPE_FLOAT  6
#define CRAY_TYPE_DOUBLE 7
#define CRAY_TYPE_CHAR   8

#define crayon_savefile_version_t uint16_t	//If you know you won't have many versions, change this to a uint8_t
											//But don't change it mid way through a project

typedef struct crayon_savefile_history{
	uint8_t data_type;
	uint16_t data_length;
	crayon_savefile_version_t version_added;
	crayon_savefile_version_t version_removed;	//0 is not removed
	uint8_t removal_command;
	struct crayon_savefile_history *transfer_var;

	//Add some unions here for the pointer to the data and the default value
	union{
		uint8_t **u8;
		uint16_t **u16;
		uint32_t **u32;
		int8_t **s8;
		int16_t **s16;
		int32_t **s32;
		float **floats;
		double **doubles;
		char **chars;
	} data_ptr;

	union{
		uint8_t u8;
		uint16_t u16;
		uint32_t u32;
		int8_t s8;
		int16_t s16;
		int32_t s32;
		float floats;
		double doubles;
		char chars;
	} default_value;

	struct crayon_savefile_history *next;
} crayon_savefile_history_t;

#define CRAY_SF_NUM_DETAIL_STRINGS 4
#define CRAY_SF_STRING_FILENAME    0
#define CRAY_SF_STRING_APP_ID      1
#define CRAY_SF_STRING_SHORT_DESC  2
#define CRAY_SF_STRING_LONG_DESC   3

#if defined( _arch_dreamcast)
	#define CRAY_SF_NUM_SAVE_DEVICES 8
	#define CRAY_SF_MEMCARD MAPLE_FUNC_MEMCARD
	#define CRAY_SF_HDR_SIZE sizeof(vmu_hdr_t)
#elif defined(_arch_pc)
	#define CRAY_SF_NUM_SAVE_DEVICES 1
	#define CRAY_SF_MEMCARD 1
	//The size of the app_id, short desc and long desc detail struct strings plus payload size
		//Later I might modify it to take the SF icon in RGBA8888 format binary
	#define CRAY_SF_HDR_SIZE (16 + 16 + 32 + sizeof(uint32_t))
#endif

//For DREAMCAST
	// Divide the device id by 2 (round down) to get the port number
	// Even device ids are slot 1 and odd is slot 2

//The struct that contains all of a save file info. This is useful for passing
//by reference into a function and if you want to modify the save file data easily
//or even use different save files for one game
typedef struct crayon_savefile_details{
	crayon_savefile_version_t version;

	crayon_savefile_data_t save_data;
	size_t save_size;	//Might move this inside the data var

	unsigned char *icon_data;		//uint8_t
	unsigned short *icon_palette;	//uint16_t
	uint8_t icon_anim_count;	//Decided to not go with a uint16_t (Largest VMU supports) because
								//when would you ever want more than 255 frames of animation here?
								//Also BIOS will only work with up to 3 icons
	uint16_t icon_anim_speed;	//Set to "0" for no animation (1st frame only)

	//Appears on the VMU screen on the left in the DC BIOS
	uint8_t *eyecatcher_data;
	uint8_t eyecatcher_type;		//PAL4BPP (4 Blocks), PAL8BPP (8.875 Blocks), plain ARGB4444 (15.75 Blocks)

	//All the strings we have. Different systems might have a different number of strings
	char *strings[CRAY_SF_NUM_DETAIL_STRINGS];

	uint8_t valid_memcards;		//Memory cards with enough space for a save file or an existing save
	uint8_t valid_saves;		//All VMUs with a savefile
	uint8_t valid_vmu_screens;	//All VMU screens (Only applies for DC)

	//This tells us what storage system we save to
	//On Dreamcast this corresponds to one of the 8 memory cards
	//On PC there is only one, the savefile folder
	//And technically if the saturn was supported, it would have internal storage + save cartridge
	int8_t save_device_id;

	crayon_savefile_history_t *history;
	crayon_savefile_history_t *history_tail;	//Just used to speed stuff the history building process
} crayon_savefile_details_t;


//---------------------DREAMCAST SPECIFIC------------------------


vec2_s8_t crayon_savefile_dreamcast_get_port_and_slot(int8_t savefile_device_id);	//x is port, y is slot

void crayon_vmu_display_icon(uint8_t vmu_bitmap, void *icon);


//---------------------Internal use------------------------


uint8_t crayon_savefile_check_savedata(crayon_savefile_details_t *details, int8_t save_device_id);	//1 if save DNE. 0 if it does
uint8_t crayon_savefile_check_device_for_function(uint32_t function, int8_t save_device_id);	//0 if device is valid
uint16_t crayon_savefile_bytes_to_blocks(size_t bytes);	//Takes a byte count and returns no. blocks needed to save it
int16_t crayon_savefile_get_save_block_count(crayon_savefile_details_t *details);	//Returns the number of blocks your save file will need (Uncompressed)

uint16_t crayon_savefile_detail_string_length(uint8_t string_id);

void __attribute__((weak)) crayon_savefile_serialise(crayon_savefile_data_t *sf_data, uint8_t *pkg_data);
void __attribute__((weak)) crayon_savefile_deserialise(crayon_savefile_data_t *sf_data, uint8_t *pkg_data, uint32_t pkg_size);

uint16_t crayon_savefile_device_free_blocks(int8_t port, int8_t slot);

//---------------Both internal and external----------------


uint8_t crayon_savefile_get_memcard_bit(uint8_t memcard_bitmap, uint8_t save_device_id);	//Returns boolean
void crayon_savefile_set_memcard_bit(uint8_t *memcard_bitmap, uint8_t save_device_id);	//Updates memcard_bitmap


//------------------Called externally----------------------

//Make sure to call this on a new savefile details struct otherwise you can get strange results
	//Note that you should also add the icon/eyecatcher if you want and set all the strings
	//as well as the variable history
uint8_t crayon_savefile_init_savefile_details(crayon_savefile_details_t *details, const char *save_name,
	crayon_savefile_version_t version);

#define crayon_savefile_set_app_id(details, string) \
crayon_savefile_set_string(details, string, CRAY_SF_STRING_APP_ID);
#define crayon_savefile_set_short_desc(details, string)  \
crayon_savefile_set_string(details, string, CRAY_SF_STRING_SHORT_DESC);
#define crayon_savefile_set_long_desc(details, string)  \
crayon_savefile_set_string(details, string, CRAY_SF_STRING_LONG_DESC);

uint8_t crayon_savefile_set_string(crayon_savefile_details_t *details, const char *string, uint8_t string_id);

//The return value is 1 when the number of icons is greater than 3. The DC BIOS can't render icons
	//with 4 or more frames.
uint8_t crayon_savefile_add_icon(crayon_savefile_details_t *details, const char *image, const char *palette,
	uint8_t icon_anim_count, uint16_t icon_anim_speed);

uint8_t crayon_savefile_add_eyecatcher(crayon_savefile_details_t *details, const char *eyecatch_path);

//Default value is just one element because having default arrays (Eg for c-strings) would be too complex in C
//So instead all elements are set to the value pointed to by default_value
crayon_savefile_history_t *crayon_savefile_add_variable(crayon_savefile_details_t *details, void *data_ptr,
	uint8_t data_type, uint16_t length, const void *default_value, crayon_savefile_version_t version);

//Pass in the pointer to the variable node we want to delete
crayon_savefile_history_t *crayon_savefile_remove_variable(crayon_savefile_details_t *details,
	crayon_savefile_history_t *target_node, uint8_t remove_command, crayon_savefile_history_t *transfer_var,
	crayon_savefile_version_t version);

//Once the history is fully constructed, we can then build our actual savefile with this fuction
void crayon_savefile_solidify(crayon_savefile_details_t *details);

//This function will update the valid memcards and/or the current savefile bitmaps
#define CRAY_SAVEFILE_UPDATE_MODE_MEMCARD_PRESENT (1 << 0)
#define CRAY_SAVEFILE_UPDATE_MODE_SAVE_PRESENT (1 << 1)
#define CRAY_SAVEFILE_UPDATE_MODE_BOTH CRAY_SAVEFILE_UPDATE_MODE_SAVE_PRESENT | CRAY_SAVEFILE_UPDATE_MODE_MEMCARD_PRESENT
void crayon_savefile_update_valid_saves(crayon_savefile_details_t *details, uint8_t modes);

//Returns an 8 bit var for each VMU (a1a2b1b2c1c2d1d2)
//First function is a macro so you don't need to remember the function name
#if defined(_arch_dreamcast)
#define crayon_savefile_get_valid_screens()  crayon_savefile_get_valid_function(MAPLE_FUNC_LCD)
#else
#define crayon_savefile_get_valid_screens()  0
#endif

//Returns a bitmap of all devices that have this function
uint8_t crayon_savefile_get_valid_function(uint32_t function);	//Can be used to find all devices with function X

//Returns 0 on success and 1 or more if failure. Handles loading and saving of uncompressed savefiles
uint8_t crayon_savefile_load_savedata(crayon_savefile_details_t *details);
uint8_t crayon_savefile_save_savedata(crayon_savefile_details_t *details);

//This will delete the saved savefile 
uint8_t crayon_savefile_delete_savedata(crayon_savefile_details_t *details);	//UNFINISHED

void crayon_savefile_free(crayon_savefile_details_t *details);	//Calling this calls the other frees
void crayon_savefile_free_icon(crayon_savefile_details_t *details);
void crayon_savefile_free_eyecatcher(crayon_savefile_details_t *details);

#endif
