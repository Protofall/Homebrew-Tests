#include "setup.h"

//NOTE: This function contains the default values only for the current version
//Thats why we can reference the variables directly like this
void savefile_defaults(){
	// if(!sf_var1){printf("Is NULL\n"); exit(1);}	//Triggers
	sf_var1[0] = 300;
	sf_var2[0] = 5.5;
	sf_var3[0] = 27;

	uint16_t i, j;
	for(i = 0; i < sf_var4_length; i++){
		sf_lol[i][0] = 2;

		for(j = 0; j < sf_hi_length; j++){
			sf_hi[i][j] = -1;
		}

		//I use strncpy instead of strcpy so we know the value
		//of all characters in the buffer
		strncpy(sf_name[i], "PLACEHOLDER", sf_name_length);
	}

	sf_myspace[0] = 1;

	for(i = 0; i < sf_speedrun_times_length; i++){
		sf_speedrun_times[i] = -1;
	}

	return;
}

//In this function, we don't handle the variables directly like normal, since some of
//them don't exist anymore. So instead we refer to them by their history IDs like so

//THIS IS USED BY THE CRAYON SAVEFILE DESERIALISER WHEN LOADING A SAVE FROM AN OLDER VERSION
//THERE IS NO NEED TO CALL THIS MANUALLY
uint8_t update_savefile(crayon_savefile_old_variable_t *loaded_savedata,
	crayon_savefile_version_t loaded_version, crayon_savefile_version_t latest_version){
	
	//NOTE: We only need to handle vars that no longer exist
	//We assume that the user's variable are global so thats why they don't have the latest savedata struct present
	//We also assume the user's var IDs are globally accessable or they manually used them here as magic numbers
	;

	//Use "crayon_savefile_get_variable_ptr(crayon_savefile_old_variable_t *array, uint32_t index)"
	//to get the right pointer

	return 0;
}

uint8_t setup_savefile(crayon_savefile_details_t * details){
	uint8_t i, error;

	// sf_var1 = NULL;

	#ifdef _arch_pc

	crayon_savefile_set_path("saves/");

	#else
	
	crayon_savefile_set_path(NULL);	//Dreamcast ignores the parameter anyways
									//(Assumes "/vmu/") so its still fine to
									//do the method above for all platforms
	#endif

	error = crayon_savefile_init_savefile_details(details, "SAVE_DEMO3.s", SFV_CURRENT,
		savefile_defaults, update_savefile);
	if(error){printf("ERROR, savefile couldn't be created\n");}
	error += crayon_savefile_set_app_id(details, "ProtoSaveDemo3");
	error += crayon_savefile_set_short_desc(details, "Save Demo");
	error += crayon_savefile_set_long_desc(details, "Crayon's VMU demo");
	if(error){printf("ERROR, Savefile string too long\n");}

	#ifdef _arch_dreamcast

	//Load the VMU icon data
	#if CRAYON_BOOT_MODE == 1
		crayon_memory_mount_romdisk("/sd/sf_icon.img", "/Save");
	#else
		crayon_memory_mount_romdisk("/cd/sf_icon.img", "/Save");
	#endif

	uint8_t * vmu_lcd_icon = NULL;

	setup_vmu_icon_load(&vmu_lcd_icon, "/Save/LCD.bin");

	//Apply the VMU LCD icon (Apparently this is automatic if your savefile is an ICONDATA.VMS)
	setup_vmu_icon_apply(vmu_lcd_icon, details->valid_vmu_screens);
	// free(vmu_lcd_icon);	//Already free-d within the above function
	
	crayon_savefile_add_icon(details, "/Save/image.bin", "/Save/palette.bin", 3, 15);
	crayon_savefile_add_eyecatcher(details, "Save/eyecatch3.bin");	//Must be called AFTER init

	fs_romdisk_unmount("/Save");

	#endif

	//Now lets construct our history
	crayon_savefile_add_variable(details, &sf_var1, sf_var1_type, sf_var1_length, SFV_INITIAL, VAR_STILL_PRESENT);
	crayon_savefile_add_variable(details, &sf_var2, sf_var2_type, sf_var2_length, SFV_INITIAL, VAR_STILL_PRESENT);
	crayon_savefile_add_variable(details, &sf_var3, sf_var3_type, sf_var3_length, SFV_INITIAL, VAR_STILL_PRESENT);
	for(i = 0; i < sf_var4_length; i++){
		crayon_savefile_add_variable(details, &sf_lol[i], sf_lol_type, sf_lol_length, SFV_INITIAL, VAR_STILL_PRESENT);
		crayon_savefile_add_variable(details, &sf_hi[i], sf_hi_type, sf_hi_length, SFV_INITIAL, VAR_STILL_PRESENT);
		crayon_savefile_add_variable(details, &sf_name[i], sf_name_type, sf_name_length, SFV_INITIAL, VAR_STILL_PRESENT);
	}

	crayon_savefile_add_variable(details, &sf_myspace, sf_myspace_type, sf_myspace_length,
		SFV_SPEEDRUNNER, VAR_STILL_PRESENT);
	crayon_savefile_add_variable(details, &sf_speedrun_times, sf_speedrun_times_type, sf_speedrun_times_length,
		SFV_SPEEDRUNNER, VAR_STILL_PRESENT);

	//Set the savefile
	crayon_savefile_solidify(details);

	// for(i = 0; i < CRAY_NUM_TYPES; i++){
	// 	printf("Lengths %d\n", details->savedata.lengths[i]);
	// }

	// if(!sf_var1){printf("Still NULL\n");}

	// if(details->history->data_ptr.u16 != &sf_var1){
	// 	printf("Its not pointing to the right place\n");
	// }
	// else{
	// 	printf("It should be fine\n");
	// }

	return 0;
}

//We use a double pointer because we want to modify the pointer itself with malloc
int16_t setup_vmu_icon_load(uint8_t ** vmu_lcd_icon, char * icon_path){
	#ifdef _arch_dreamcast
	*vmu_lcd_icon = (uint8_t *) malloc(6 * 32);	//6 * 32 because we have 48/32 1bpp so we need that / 8 bytes
	FILE * file_lcd_icon = fopen(icon_path, "rb");
	if(!file_lcd_icon){return -1;}
	size_t res = fread(*vmu_lcd_icon, 192, 1, file_lcd_icon);	//If the icon is right, it *must* byt 192 bytes
	fclose(file_lcd_icon);

	return res;
	#else

	return 0;
	#endif
}

void setup_vmu_icon_apply(uint8_t * vmu_lcd_icon, uint8_t valid_vmu_screens){
	#ifdef _arch_dreamcast
	crayon_vmu_display_icon(valid_vmu_screens, vmu_lcd_icon);
	free(vmu_lcd_icon);
	#endif

	return;
}
