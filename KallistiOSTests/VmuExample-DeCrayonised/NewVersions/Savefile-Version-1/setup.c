#include "setup.h"

uint8_t setup_savefile(crayon_savefile_details_t * details){
	uint8_t i;
	crayon_savefile_init_savefile_details(details, "Crayon's VMU demo\0", "Save Demo\0",
		"ProtoSaveDemo3\0", "SAVE_DEMO3.s\0", sf_current_version);
	
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
	uint8_t res = crayon_savefile_add_eyecatcher(details, "Save/eyecatch3.bin");	//Must be called AFTER init

	fs_romdisk_unmount("/Save");

	//Now lets construct our history
	crayon_savefile_add_variable(details, var1, var1_type, var1_length, &var1_default, sf_initial);
	crayon_savefile_add_variable(details, var2, var2_type, var2_length, &var2_default, sf_initial);
	for(i = 0; i < var3_length; i++){
		crayon_savefile_add_variable(details, lol[i], lol_type, lol_length, &lol_default, sf_initial);
		crayon_savefile_add_variable(details, hi[i], hi_type, hi_length, &hi_default, sf_initial);
		crayon_savefile_add_variable(details, name[i], name_type, name_length, &name_default, sf_initial);
	}

	//Set the savefile
	crayon_savefile_solidify(details);

	return res;
}

//We use a double pointer because we want to modify the pointer itself with malloc
int16_t setup_vmu_icon_load(uint8_t ** vmu_lcd_icon, char * icon_path){
	*vmu_lcd_icon = (uint8_t *) malloc(6 * 32);	//6 * 32 because we have 48/32 1bpp so we need that / 8 bytes
	FILE * file_lcd_icon = fopen(icon_path, "rb");
	if(!file_lcd_icon){return -1;}
	size_t res = fread(*vmu_lcd_icon, 192, 1, file_lcd_icon);	//If the icon is right, it *must* byt 192 bytes
	fclose(file_lcd_icon);

	return res;
}

void setup_vmu_icon_apply(uint8_t * vmu_lcd_icon, uint8_t valid_vmu_screens){
	crayon_vmu_display_icon(valid_vmu_screens, vmu_lcd_icon);
	free(vmu_lcd_icon);

	return;
}
