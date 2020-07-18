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
	uint8_t myspace;
	double speedrun_times[2];
} my_savefile_var_t;
*/

//Here's how we represent it now
	//WARNING. Don't manually modified the pointers. Crayon savefile functions will set them itself
	//NOTE: static const vars will have a copy in each object file. If you want to only have one copy
		//just remove the static const and set each var's value in setup_savefile()

//v0 VARS

uint16_t *sf_var1;
#define sf_var1_type CRAY_TYPE_UINT16
#define sf_var1_length 1	//If you want, these length defines could be unsigned int consts.

float *sf_var2;
#define sf_var2_type CRAY_TYPE_FLOAT
#define sf_var2_length 1

uint8_t *sf_var3;
#define sf_var3_type CRAY_TYPE_UINT8
#define sf_var3_length 1

//Now the "other_struct"
#define sf_var4_length 10

uint8_t *sf_lol[sf_var4_length];
#define sf_lol_type CRAY_TYPE_UINT8
#define sf_lol_length 1

int32_t *sf_hi[sf_var4_length];
#define sf_hi_type CRAY_TYPE_SINT32
#define sf_hi_length 2

char *sf_name[sf_var4_length];	// "sf_var4_length" strings with 16 chars each
#define sf_name_type CRAY_TYPE_CHAR
#define sf_name_length 16


//v1 VARS

uint8_t *sf_myspace;
#define sf_myspace_type CRAY_TYPE_UINT8
#define sf_myspace_length 1

double *sf_speedrun_times;
#define sf_speedrun_times_type CRAY_TYPE_DOUBLE
#define sf_speedrun_times_length 2


//For those unfamiliar with enum, a value with no assigned number is equal to the previous value plus 1
//Also you just use the variable name like a constant, not "savefile_version.sf_initial" or something
enum savefile_version{
	SFV_INITIAL = 0,
	SFV_SPEEDRUNNER,
	//Add new versions here
	SFV_LATEST_PLUS_ONE	//DON'T REMOVE
};

#define VAR_STILL_PRESENT SFV_LATEST_PLUS_ONE

static const crayon_savefile_version_t SFV_CURRENT = SFV_LATEST_PLUS_ONE - 1;

#endif
