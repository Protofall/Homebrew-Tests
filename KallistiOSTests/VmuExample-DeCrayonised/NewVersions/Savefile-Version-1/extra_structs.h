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
	other_struct_t var3[20];
} my_savefile_var_t;
*/

//Here's how we represent it now
	//WARNING. Don't manually modified the pointers. Crayon savefile functions will set them itself

uint16_t * var1;
#define var1_type CRAY_TYPE_UINT16
#define var1_length 1	//If you want, these length defines could be unsigned int consts.
const uint16_t var1_default = 0;

float * var2;
#define var2_type CRAY_TYPE_FLOAT
#define var2_length 1
const float var2_default = 5.5;

//Now the "other_struct"
#define var3_length 20

uint8_t * lol[var3_length];
#define lol_type CRAY_TYPE_UINT8
#define lol_length 1
const uint8_t lol_default = 0;

int32_t * hi[var3_length];
#define hi_type CRAY_TYPE_SINT32
#define hi_length 2
const int32_t hi_default = -1;

char * name[var3_length];
#define name_type CRAY_TYPE_CHAR
#define name_length 16
const char name_default = '\0';

//For those unfamiliar with enum, a value with no assigned number is equal to the previous value plus 1
//Also you just use the variable name like a const var, not "savefile_version.SF_Initial" or something
enum savefile_version{
	sf_initial = 1,
	//Add new versions here
	sf_latest_plus_one	//DON'T REMOVE
};

const crayon_savefile_version_t sf_current_version = sf_latest_plus_one - 1;

#endif
