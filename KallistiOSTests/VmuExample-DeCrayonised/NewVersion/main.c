//For the controller and mouse
#ifdef _arch_dreamcast
#include <dc/maple.h>
#include <dc/maple/controller.h>
#endif

#include "setup.h"
#include "graphics.h"

#define CRAYON_DEBUG 0

#ifdef _arch_pc

//NOTE: You should never need to access these variables directly. I'm only doing so for debugging purposes
void print_all_vars(crayon_savefile_details_t *savefile_details){
	printf("u8\n");
	for(int i = 0; i < savefile_details->savedata.lengths[CRAY_TYPE_UINT8]; i++){
		printf("%d, ", savefile_details->savedata.u8[i]);
	}
	printf("\n");

	printf("u16\n");
	for(int i = 0; i < savefile_details->savedata.lengths[CRAY_TYPE_UINT16]; i++){
		printf("%d, ", savefile_details->savedata.u16[i]);
	}
	printf("\n");

	printf("u32\n");
	for(int i = 0; i < savefile_details->savedata.lengths[CRAY_TYPE_UINT32]; i++){
		printf("%d, ", savefile_details->savedata.u32[i]);
	}
	printf("\n");

	printf("s8\n");
	for(int i = 0; i < savefile_details->savedata.lengths[CRAY_TYPE_SINT8]; i++){
		printf("%d, ", savefile_details->savedata.s8[i]);
	}
	printf("\n");

	printf("s16\n");
	for(int i = 0; i < savefile_details->savedata.lengths[CRAY_TYPE_SINT16]; i++){
		printf("%d, ", savefile_details->savedata.s16[i]);
	}
	printf("\n");

	printf("s32\n");
	for(int i = 0; i < savefile_details->savedata.lengths[CRAY_TYPE_SINT32]; i++){
		printf("%d, ", savefile_details->savedata.s32[i]);
	}
	printf("\n");

	printf("Float\n");
	for(int i = 0; i < savefile_details->savedata.lengths[CRAY_TYPE_FLOAT]; i++){
		printf("%f, ", savefile_details->savedata.floats[i]);
	}
	printf("\n");

	printf("Double\n");
	for(int i = 0; i < savefile_details->savedata.lengths[CRAY_TYPE_DOUBLE]; i++){
		printf("%lf, ", savefile_details->savedata.doubles[i]);
	}
	printf("\n");

	printf("Chars\n");
	for(int i = 0; i < savefile_details->savedata.lengths[CRAY_TYPE_CHAR]; i++){
		printf("%c", savefile_details->savedata.chars[i]);
	}
	printf("(END)\n");

	return;
}

#endif

int main(){
	#if defined(_arch_dreamcast) && CRAYON_BOOT_MODE == 1

	int sdRes = mount_ext2_sd();	//This function should be able to mount an ext2 formatted sd card to the /sd dir
	if(sdRes != 0){
		return 0;
	}

	#endif

	crayon_savefile_details_t savefile_details;

	uint8_t setup_res = setup_savefile(&savefile_details);

	#if defined(_arch_dreamcast)
	
	pvr_init_defaults();	//Init kos
	font_init();
	
	#endif

	//Try and load savefile
	uint8_t load_error = crayon_savefile_load_savedata(&savefile_details);	//If a savefile DNE this fails

	//Change vars here
	// sf_var1[0] = 2997;
	// sf_var2[0] += 5.25;
	// sf_name[2][3] = '1';

	uint8_t save_error = 1;
	if(savefile_details.valid_devices){
		save_error = crayon_savefile_save_savedata(&savefile_details);
		crayon_savefile_update_valid_saves(&savefile_details, CRAY_SAVEFILE_UPDATE_MODE_SAVE_PRESENT);	//Updating the save
	}

	#if defined(_arch_dreamcast) && CRAYON_BOOT_MODE == 1
	
	unmount_ext2_sd();	//Unmounts the SD dir to prevent corruption since we won't need it anymore
	
	#endif

	#ifdef _arch_dreamcast
	
	char buffer[70];
	if(!setup_res){
		sprintf(buffer, "Save created\nUses %d blocks and has %d frames of\nanimation",
		crayon_savefile_get_save_block_count(&savefile_details),
		savefile_details.icon_anim_count);
	}
	else{
		sprintf(buffer, "It failed with code %d", setup_res);
	}

	uint8_t end = 0;
	while(!end){
		pvr_wait_ready();
		MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)
			if(st->buttons & CONT_START){
				end = 1;
			}
		MAPLE_FOREACH_END()

		pvr_scene_begin();

		pvr_list_begin(PVR_LIST_TR_POLY);
			switch(save_error){
				case 0:
					draw_string(30, 30, 1, 255, 255, 216, 0, buffer, 2, 2); break;
				case 1:
					draw_string(30, 30, 1, 255, 255, 216, 0, "Selected device isn't a VMU", 2, 2); break;
				case 2:
					draw_string(30, 30, 1, 255, 255, 216, 0, "Selected VMU doesn't have enough space", 2, 2); break;
				case 3:
					draw_string(30, 30, 1, 255, 255, 216, 0, "Ran out of memory when making savefile", 2, 2); break;
				case 4:
					draw_string(30, 30, 1, 255, 255, 216, 0, "Not enough space on VMU for savefile", 2, 2); break;
				case 5:
					draw_string(30, 30, 1, 255, 255, 216, 0, "Couldn't access savefile on VMU", 2, 2); break;
				case 6:
					draw_string(30, 30, 1, 255, 255, 216, 0, "Couldn't write to VMU", 2, 2); break;
			}
		pvr_list_finish();


		pvr_scene_finish();
	}

	#else

	char buffer[70];
	if(!setup_res){
		sprintf(buffer, "Save initialised\nUses %ld bytes", CRAY_SF_HDR_SIZE + savefile_details.savedata_size);
	}
	else{
		sprintf(buffer, "It failed with code %d", setup_res);
	}

	char buffer2[32];
	char buffer3[32];
	sprintf(buffer2, "save_error: %d. load_error %d\n", save_error, load_error);
	sprintf(buffer3, "bitmaps: %d. %d\n", savefile_details.valid_devices, savefile_details.valid_saves);
	draw_string(0, 0, 0, 0, 0, 0, 0, buffer, 0, 0);
	draw_string(0, 0, 0, 0, 0, 0, 0, buffer2, 0, 0);
	draw_string(0, 0, 0, 0, 0, 0, 0, buffer3, 0, 0);

	print_all_vars(&savefile_details);

	// sf_var1[0] = 2997;
	// sf_name[2][3] = '1';
	// //Each sf_name is 16 chars and we have 20 names. So this is the 4th char of the 3rd name
	// //(2 * 16) + 3 = 35-th index of the dest array

	// print_all_vars(&savefile_details);

	#endif

	crayon_savefile_free(&savefile_details);
	crayon_savefile_free_base_path();
	#ifdef _arch_dreamcast
	pvr_mem_free(font_tex);
	#endif

	return 0;
}
