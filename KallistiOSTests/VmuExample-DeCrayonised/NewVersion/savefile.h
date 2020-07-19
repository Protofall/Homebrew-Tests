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

//Var types the user passes into functions
enum {
	CRAY_TYPE_DOUBLE = 0,
	CRAY_TYPE_FLOAT,
	CRAY_TYPE_UINT32,
	CRAY_TYPE_SINT32,
	CRAY_TYPE_UINT16,
	CRAY_TYPE_SINT16,
	CRAY_TYPE_UINT8,
	CRAY_TYPE_SINT8,
	CRAY_TYPE_CHAR,
	CRAY_NUM_TYPES	//This will be the number of types we have available
};

typedef union crayon_savefile_old_variables{
	uint8_t data_type;
	// void *ptr; //(since the user will probably use the built-in function instead)
	//WHY AM I USING A UNION-OF-POINTERS INSTEAD OF A VOID POINTER. Both are 8 bytes in size, right?
	union{
		double *t_double;
		float *t_float;
		uint32_t *t_u32;
		int32_t *t_s32;
		uint16_t *t_u16;
		int16_t *t_s16;
		uint8_t *t_u8;
		int8_t *t_s8;
		char *t_char;
	};
} crayon_savefile_old_variable_t;

//This is never accessed directly, but it will contain all of you variables that will get saved
typedef struct crayon_savefile_data{
	double *doubles;
	float *floats;
	uint32_t *u32;
	int32_t *s32;
	uint16_t *u16;
	int16_t *s16;
	uint8_t *u8;
	int8_t *s8;
	char *chars;

	uint32_t lengths[CRAY_NUM_TYPES];	//The lengths are in the order they appear above
} crayon_savefile_data_t;

#define crayon_savefile_version_t uint32_t	//If you know you won't have many versions, change this to a uint8_t
											//But don't change it mid way through a project

typedef struct crayon_savefile_history{
	uint32_t id;

	uint8_t data_type;
	uint32_t data_length;
	crayon_savefile_version_t version_added;
	crayon_savefile_version_t version_removed;	//0 is still present in the current version

	//Add some unions here for the pointer to the data and the default value
	union{
		double **t_double;
		float **t_float;
		uint32_t **t_u32;
		int32_t **t_s32;
		uint16_t **t_u16;
		int16_t **t_s16;
		uint8_t **t_u8;
		int8_t **t_s8;
		char **t_char;
	} data_ptr;

	struct crayon_savefile_history *next;
} crayon_savefile_history_t;

//Only Dreamcast has the descriptions
enum {
	CRAY_SF_STRING_FILENAME = 0,
	CRAY_SF_STRING_APP_ID,
	#if defined( _arch_dreamcast)
	CRAY_SF_STRING_SHORT_DESC,
	CRAY_SF_STRING_LONG_DESC,
	#endif
	CRAY_SF_NUM_DETAIL_STRINGS	// 4 on Dreamcast, 2 on PC
};

#if defined( _arch_dreamcast)

	#define CRAY_SF_NUM_SAVE_DEVICES 8
	#define CRAY_SF_HDR_SIZE sizeof(vmu_hdr_t)

#elif defined(_arch_pc)

	#define CRAY_SF_NUM_SAVE_DEVICES 1

	#define CRAY_SF_HDR_SIZE (16 + 16)	//Must be a multiple of 4 bytes long
	// #define CRAY_SF_HDR_SIZE (16 + 16 + 32 + sizeof(uint32_t))	//Must be a multiple of 4 bytes long
	typedef struct crayon_savefile_hdr{
		char name[16];	//Not actually used, but useful if you read a savefile with a hex editor
						// SHOULD BE "CRAYON SAVEFILE" with a null terminator at the end
		char app_id[16];
		// char short_desc[16];
		// char long_desc[32];
		// uint32_t data_size;
	} crayon_savefile_hdr_t;

#endif

//For DREAMCAST
	// Divide the device id by 2 (round down) to get the port number
	// Even device ids are slot 1 and odd is slot 2

//The struct that contains all of a save file info. This is useful for passing
//by reference into a function and if you want to modify the save file data easily
//or even use different save files for one game
typedef struct crayon_savefile_details{
	crayon_savefile_version_t latest_version;

	crayon_savefile_data_t savedata;
	uint32_t savedata_size;	//Might move this inside the data var

	//All the strings we have. Different systems might have a different number of strings
	char *strings[CRAY_SF_NUM_DETAIL_STRINGS];

	uint8_t present_devices;	//Shows any device thats present that has enough space to save too
	uint8_t present_savefiles;	//Shows any device with any savefile from any version
	uint8_t current_savefiles;	//Only shows savefiles in the current format
	crayon_savefile_version_t savefile_versions[CRAY_SF_NUM_SAVE_DEVICES];	//Stores the versions of savefiles.


	//Move "valid_vmu_screens" into the peripherals section
	uint8_t valid_vmu_screens;	//All VMU screens (Only applies for DC)

	//This tells us what storage system we save to
	//On Dreamcast this corresponds to one of the 8 memory cards
	//On PC there is only one, the savefile folder
	//And technically if the saturn was supported, it would have internal storage + save cartridge (2)
	int8_t save_device_id;

	crayon_savefile_history_t *history;
	crayon_savefile_history_t *history_tail;	//Just used to speed stuff the history building process
	uint32_t num_vars;

	void (*default_values_func)();
	uint8_t (*update_savefile_func)(crayon_savefile_old_variable_t*,
	crayon_savefile_version_t, crayon_savefile_version_t);

	#if defined(_arch_dreamcast)

	uint8_t *icon_data;
	uint16_t *icon_palette;
	uint8_t icon_anim_count;	//Decided to not go with a uint16_t (Largest VMU supports) because
								//when would you ever want more than 255 frames of animation here?
								//Also BIOS will only work with up to 3 icons
	uint16_t icon_anim_speed;	//Set to "0" for no animation (1st frame only)

	//Appears on the VMU screen on the left in the DC BIOS
	uint8_t *eyecatcher_data;
	uint8_t eyecatcher_type;		//PAL4BPP (4 Blocks), PAL8BPP (8.875 Blocks), plain ARGB4444 (15.75 Blocks)

	#endif
} crayon_savefile_details_t;


//---------------------DREAMCAST SPECIFIC------------------------


void crayon_vmu_display_icon(uint8_t vmu_bitmap, void *icon);

//Returns an 8 bit var for each VMU (a1a2b1b2c1c2d1d2)
//First function is a macro so you don't need to remember the function name
#if defined(_arch_dreamcast)
#define crayon_savefile_get_valid_screens()  crayon_savefile_get_valid_function(MAPLE_FUNC_LCD)
#else
#define crayon_savefile_get_valid_screens()  0
#endif

#if defined(_arch_dreamcast)

//0 if device is valid
uint8_t crayon_savefile_check_device_for_function(uint32_t function, int8_t save_device_id);

//Returns a bitmap of all vmus with a screen
uint8_t crayon_savefile_dreamcast_get_screen_bitmap();	//Can be used to find all devices with function X

#endif


//---------------------Internal use------------------------


//Takes a byte count and returns no. blocks needed to save it
uint16_t crayon_savefile_bytes_to_blocks(uint32_t bytes);

//Returns the number of bytes your save file will need
uint32_t crayon_savefile_get_savefile_size(crayon_savefile_details_t *details);

uint16_t crayon_savefile_detail_string_length(uint8_t string_id);

void crayon_savefile_serialise(crayon_savefile_details_t *details, uint8_t *buffer);
uint8_t crayon_savefile_deserialise(crayon_savefile_details_t *details, uint8_t *data, uint32_t data_length);

uint32_t crayon_savefile_get_device_free_space(int8_t device_id);

//Returns a pointer on success, returns NULL if either the the save_device_id is OOB or failed malloc
char *crayon_savefile_get_full_path(crayon_savefile_details_t *details, int8_t save_device_id);

//Only really meant for debugging on PC. It just prints all the values in the savedata struct
void __crayon_savefile_print_savedata(crayon_savefile_data_t *savedata);

void crayon_savefile_buffer_to_savedata(crayon_savefile_data_t *data, uint8_t *buffer);

uint8_t crayon_savefile_set_string(crayon_savefile_details_t *details, const char *string, uint8_t string_id);


//---------------Both internal and external----------------


uint8_t crayon_savefile_get_device_bit(uint8_t device_bitmap, uint8_t save_device_id);	//Returns boolean
void crayon_savefile_set_device_bit(uint8_t *device_bitmap, uint8_t save_device_id);	//Updates device_bitmap

//Returns 0 if a savefile on that device exists with no issues, 1 if there's an error/DNE/version is newer than latest
uint8_t crayon_savefile_check_savedata(crayon_savefile_details_t *details, int8_t save_device_id);


//------------------Called externally----------------------


uint8_t crayon_savefile_set_path(char *path);	//On Dreamcast this is always "/vmu/" and it will ignore the param

//Make sure to call this on a new savefile details struct otherwise you can get strange results if
	//you use it without this. The last two parameters are function pointers to user defined default
	//values and how to handling going up a version
//Note that you should also add the icon/eyecatcher if you want and set all the strings as well as
	//the variable history.
uint8_t crayon_savefile_init_savefile_details(crayon_savefile_details_t *details, const char *save_name,
	crayon_savefile_version_t latest_version, void (*default_values_func)(),
	uint8_t (*update_savefile_func)(crayon_savefile_old_variable_t*, crayon_savefile_version_t,
	crayon_savefile_version_t));

#define crayon_savefile_set_app_id(details, string) \
crayon_savefile_set_string(details, string, CRAY_SF_STRING_APP_ID);

#if defined(_arch_dreamcast)

#define crayon_savefile_set_short_desc(details, string)  \
crayon_savefile_set_string(details, string, CRAY_SF_STRING_SHORT_DESC);
#define crayon_savefile_set_long_desc(details, string)  \
crayon_savefile_set_string(details, string, CRAY_SF_STRING_LONG_DESC);

#else

#define crayon_savefile_set_short_desc(details, string) 0;
#define crayon_savefile_set_long_desc(details, string) 0;

#endif

//The return value is 1 when the number of icons is greater than 3. The DC BIOS can't render icons
	//with 4 or more frames.
uint8_t crayon_savefile_add_icon(crayon_savefile_details_t *details, const char *image, const char *palette,
	uint8_t icon_anim_count, uint16_t icon_anim_speed);

uint8_t crayon_savefile_add_eyecatcher(crayon_savefile_details_t *details, const char *eyecatch_path);

//Return type is the id of the variable. Starts at 1, if the function returns 0 then an error occured
//Note that if a variable still exists, for version_removed we set it to zero
int32_t crayon_savefile_add_variable(crayon_savefile_details_t *details, void *data_ptr, uint8_t data_type, 
	uint32_t length, crayon_savefile_version_t version_added, crayon_savefile_version_t version_removed);

void *crayon_savefile_get_variable_ptr(crayon_savefile_old_variable_t *array, uint32_t index);

//Once the history is fully constructed, we can then build our actual savefile with this fuction
uint8_t crayon_savefile_solidify(crayon_savefile_details_t *details);

//This function will update the valid devices and/or the current savefile bitmaps
void crayon_savefile_update_valid_saves(crayon_savefile_details_t *details);

//Returns 0 on success and 1 or more if failure. Handles loading and saving of uncompressed savefiles
uint8_t crayon_savefile_load_savedata(crayon_savefile_details_t *details);
uint8_t crayon_savefile_save_savedata(crayon_savefile_details_t *details);

//This will delete the saved savefile 
uint8_t crayon_savefile_delete_savedata(crayon_savefile_details_t *details);	//UNFINISHED

void crayon_savefile_free_base_path();

void crayon_savefile_free(crayon_savefile_details_t *details);	//Calling this calls the other frees
void crayon_savefile_free_icon(crayon_savefile_details_t *details);
void crayon_savefile_free_eyecatcher(crayon_savefile_details_t *details);
void crayon_savefile_free_savedata(crayon_savefile_data_t *savedata);

#endif
