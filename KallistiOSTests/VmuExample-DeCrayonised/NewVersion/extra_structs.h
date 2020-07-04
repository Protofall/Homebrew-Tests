#ifndef MY_SAVEFILE_VARS_H
#define MY_SAVEFILE_VARS_H

#include "savefile.h"


//FYI the savefile should be 2 blocks long

//The commented code below is what your savefile would have looked like in the old system

/*
typedef struct other_struct{
	uint8_t lol;
	int32_t hi[2];
	char name[16];
} other_struct_t;

typedef struct my_savefile_var{
	uint16_t var1;
	float var2;
	uint8_t var3;
	other_struct_t var4[10];
} my_savefile_var_t;
*/

//Here's how we represent it now
	//WARNING. Don't manually modified the pointers. Crayon savefile functions will set them itself
	//NOTE: static const vars will have a copy in each object file. If you want to only have one copy
		//just remove the static const and set each var's value in setup_savefile()

uint16_t * sf_var1;
#define sf_var1_type CRAY_TYPE_UINT16
#define sf_var1_length 1	//If you want, these length defines could be unsigned int consts.
static const uint16_t sf_var1_default = 300;

float * sf_var2;
#define sf_var2_type CRAY_TYPE_FLOAT
#define sf_var2_length 1
static const float sf_var2_default = 5.5;

uint8_t * sf_var3;
#define sf_var3_type CRAY_TYPE_UINT8
#define sf_var3_length 1
static const uint8_t sf_var3_default = 27;

//Now the "other_struct"
#define sf_var4_length 10

uint8_t * sf_lol[sf_var4_length];
#define sf_lol_type CRAY_TYPE_UINT8
#define sf_lol_length 1
static const uint8_t sf_lol_default = 0;

int32_t * sf_hi[sf_var4_length];
#define sf_hi_type CRAY_TYPE_SINT32
#define sf_hi_length 2
static const int32_t sf_hi_default = -1;

char * sf_name[sf_var4_length];
#define sf_name_type CRAY_TYPE_CHAR
#define sf_name_length 16
static const char sf_name_default = 'A';

//For those unfamiliar with enum, a value with no assigned number is equal to the previous value plus 1
//Also you just use the variable name like a constant, not "savefile_version.sf_initial" or something
enum savefile_version{
	sf_initial = 1,
	//Add new versions here
	sf_latest_plus_one	//DON'T REMOVE
};

static const crayon_savefile_version_t sf_current_version = sf_latest_plus_one - 1;

#endif
