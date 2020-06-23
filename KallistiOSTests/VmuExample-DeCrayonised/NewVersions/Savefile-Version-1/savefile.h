#ifndef SAVE_FILE_H
#define SAVE_FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> //For the uintX_t types

#include <dc/maple/vmu.h>
#include <dc/vmu_pkg.h>
#include <dc/vmufs.h>
#include <kos/fs.h>

//This is never accessed directly, but it will contain all of you variables that will get saved
typedef struct crayon_savefile_data{
	uint8_t * u8;
	uint16_t * u16;
	uint32_t * u32;
	int8_t * s8;
	int16_t * s16;
	int32_t * s32;
	float * floats;
	double * doubles;
	char * chars;

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
	struct crayon_savefile_history * transfer_var;

	//Add some unions here for the pointer to the data and the default value
	union{
		uint8_t * u8;
		uint16_t * u16;
		uint32_t * u32;
		int8_t * s8;
		int16_t * s16;
		int32_t * s32;
		float * floats;
		double * doubles;
		char * chars;
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

	struct crayon_savefile_history * next;
} crayon_savefile_history_t;

//The struct that contains all of a save file info. This is useful for passing
//by reference into a function and if you want to modify the save file data easily
//or even use different save files for one game
typedef struct crayon_savefile_details{
	crayon_savefile_version_t version;

	crayon_savefile_data_t save_data;
	size_t save_size;	//Might move this inside the data var

	unsigned char * icon;			//uint8_t
	unsigned short * icon_palette;	//uint16_t
	uint8_t icon_anim_count;	//Decided to not go with a uint16_t (Largest VMU supports) because
								//when would you ever want more than 255 frames of animation here?
								//Also BIOS will only work with up to 3 icons
	uint16_t icon_anim_speed;	//Set to "0" for no animation (1st frame only)

	//Appears on the VMU screen on the left in the DC BIOS
	uint8_t * eyecatch_data;
	uint8_t eyecatch_type;		//PAL4BPP (4 Blocks), PAL8BPP (8.875 Blocks), plain ARGB4444 (15.75 Blocks)

	char desc_long[32];
	char desc_short[16];
	char app_id[16];
	char save_name[26];			//Name is 32 chars long max I think and its prefixed with "/vmu/XX/"

	uint8_t valid_memcards;		//Memory cards with enough space for a save file or an existing save
	uint8_t valid_saves;		//All VMUs with a savefile
	uint8_t valid_vmu_screens;	//All VMU screens (Only applies for DC)

	int8_t save_port;
	int8_t save_slot;

	crayon_savefile_history_t * history;
	crayon_savefile_history_t * history_tail;	//Just used to speed stuff the history building process
} crayon_savefile_details_t;


//---------------------Internal use------------------------


uint8_t crayon_savefile_check_for_save(crayon_savefile_details_t * details, int8_t save_port, int8_t save_slot);	//1 if save DNE. 0 if it does
uint8_t crayon_savefile_check_for_device(int8_t port, int8_t slot, uint32_t function);	//0 if device is valid
uint16_t crayon_savefile_bytes_to_blocks(size_t bytes);	//Takes a byte count and returns no. blocks needed to save it
int16_t crayon_savefile_get_save_block_count(crayon_savefile_details_t * details);	//Returns the number of blocks your save file will need (Uncompressed)

void __attribute__((weak)) crayon_savefile_serialise(crayon_savefile_data_t * sf_data, uint8_t * pkg_data);
void __attribute__((weak)) crayon_savefile_deserialise(crayon_savefile_data_t * sf_data, uint8_t * pkg_data, uint32_t pkg_size);


//---------------Both internal and external----------------


uint8_t crayon_savefile_get_memcard_bit(uint8_t memcard_bitmap, int8_t save_port, int8_t save_slot);	//Returns boolean
void crayon_savefile_set_memcard_bit(uint8_t * memcard_bitmap, int8_t save_port, int8_t save_slot);	//Updates memcard_bitmap


//------------------Called externally----------------------

//Make sure to call this after making a new savefile struct otherwise you can get strange results
	//Also note the return value is 1 when the number of icons is greater than 3. The DC BIOS can't render icons
	//with 4 or more frames. If you call this AFTER loading an eyecatcher it will override the eyecatch_type variable
	//with zero. Call init FIRST
void crayon_savefile_init_savefile_details(crayon_savefile_details_t * details, const char * desc_long,
	const char * desc_short, const char * app_id, const char * save_name, crayon_savefile_version_t version);

uint8_t crayon_savefile_add_icon(crayon_savefile_details_t * details, char * image, char * palette,
	uint8_t icon_anim_count, uint16_t icon_anim_speed);
uint8_t crayon_savefile_add_eyecatcher(crayon_savefile_details_t * details, char * eyecatch_path);

//Default value is just one element because having default arrays (Eg for c-strings) would be too complex in C
//So instead all elements are set to the value pointed to by default_value
crayon_savefile_history_t * crayon_savefile_add_variable(crayon_savefile_details_t * details, void * data_ptr,
	uint8_t data_type, uint16_t length, const void * default_value, crayon_savefile_version_t version);

//Pass in the pointer to the variable node we want to delete
crayon_savefile_history_t * crayon_savefile_remove_variable(crayon_savefile_details_t * details,
	crayon_savefile_history_t * target_node, uint8_t remove_command, crayon_savefile_history_t * transfer_var,
	crayon_savefile_version_t version);

//Once the history is fully constructed, we can then build our actual savefile with this fuction
void crayon_savefile_solidify(crayon_savefile_details_t * details);

//This function will update the valid memcards and/or the current savefile bitmaps
#define CRAY_SAVEFILE_UPDATE_MODE_MEMCARD_PRESENT (1 << 0)
#define CRAY_SAVEFILE_UPDATE_MODE_SAVE_PRESENT (1 << 1)
#define CRAY_SAVEFILE_UPDATE_MODE_BOTH CRAY_SAVEFILE_UPDATE_MODE_SAVE_PRESENT | CRAY_SAVEFILE_UPDATE_MODE_MEMCARD_PRESENT
void crayon_savefile_update_valid_saves(crayon_savefile_details_t * details, uint8_t modes);

//Returns an 8 bit var for each VMU (a1a2b1b2c1c2d1d2)
//First function is a macro so you don't need to remember the function name
#define crayon_savefile_get_valid_screens()  crayon_savefile_get_valid_function(MAPLE_FUNC_LCD)
uint8_t crayon_savefile_get_valid_function(uint32_t function);	//Can be used to find all devices with function X

//Returns 0 on success and 1 or more if failure. Handles loading and saving of uncompressed savefiles
uint8_t crayon_savefile_load(crayon_savefile_details_t * details);
uint8_t crayon_savefile_save(crayon_savefile_details_t * details);

void crayon_savefile_free(crayon_savefile_details_t * details);	//Calling this calls the other frees
void crayon_savefile_free_icon(crayon_savefile_details_t * details);
void crayon_savefile_free_eyecatcher(crayon_savefile_details_t * details);


//------------Not savefile, but VMU related---------------

void crayon_vmu_display_icon(uint8_t vmu_bitmap, void * icon);

//Add a savefile deletion function using this KOS function probably 
// int vmufs_delete(maple_device_t * dev, const char * fn)

#endif
