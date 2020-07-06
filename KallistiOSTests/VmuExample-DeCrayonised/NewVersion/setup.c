#include "setup.h"

uint8_t setup_savefile(crayon_savefile_details_t * details){
	uint8_t i, error;

	#ifdef _arch_pc

	crayon_savefile_set_path("saves/");

	#else
	
	crayon_savefile_set_path(NULL);	//Dreamcast ignore the parameter anyways
											//(Assumes "/vmu/") so its still fine to
											//do the method above for all platforms
	#endif

	error = crayon_savefile_init_savefile_details(details, "SAVE_DEMO3.s", sf_current_version);
	if(error){printf("ERROR, savefile couldn't be created\n");}
	error += crayon_savefile_set_app_id(details, "ProtoSaveDemo3");
	error += crayon_savefile_set_short_desc(details, "Save Demo");
	error += crayon_savefile_set_long_desc(details, "Crayon's VMU demo");
	if(error){printf("ERROR, Savefile string too long\n");}

	if(error){while(1);}

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
	crayon_savefile_add_variable(details, &sf_var1, sf_var1_type, sf_var1_length, &sf_var1_default, sf_initial);
	crayon_savefile_add_variable(details, &sf_var2, sf_var2_type, sf_var2_length, &sf_var2_default, sf_initial);
	crayon_savefile_add_variable(details, &sf_var3, sf_var3_type, sf_var3_length, &sf_var3_default, sf_initial);
	for(i = 0; i < sf_var4_length; i++){
		crayon_savefile_add_variable(details, &sf_lol[i], sf_lol_type, sf_lol_length, &sf_lol_default, sf_initial);
		crayon_savefile_add_variable(details, &sf_hi[i], sf_hi_type, sf_hi_length, &sf_hi_default, sf_initial);
		crayon_savefile_add_variable(details, &sf_name[i], sf_name_type, sf_name_length, &sf_name_default, sf_initial);
	}

	//Set the savefile
	crayon_savefile_solidify(details);

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
