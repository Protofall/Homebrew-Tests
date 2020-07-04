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
	for(int i = 0; i < savefile_details->save_data.lengths[CRAY_TYPE_UINT8]; i++){
		printf("%d, ", savefile_details->save_data.u8[i]);
	}
	printf("\n");

	printf("u16\n");
	for(int i = 0; i < savefile_details->save_data.lengths[CRAY_TYPE_UINT16]; i++){
		printf("%d, ", savefile_details->save_data.u16[i]);
	}
	printf("\n");

	printf("u32\n");
	for(int i = 0; i < savefile_details->save_data.lengths[CRAY_TYPE_UINT32]; i++){
		printf("%d, ", savefile_details->save_data.u32[i]);
	}
	printf("\n");

	printf("s8\n");
	for(int i = 0; i < savefile_details->save_data.lengths[CRAY_TYPE_SINT8]; i++){
		printf("%d, ", savefile_details->save_data.s8[i]);
	}
	printf("\n");

	printf("s16\n");
	for(int i = 0; i < savefile_details->save_data.lengths[CRAY_TYPE_SINT16]; i++){
		printf("%d, ", savefile_details->save_data.s16[i]);
	}
	printf("\n");

	printf("s32\n");
	for(int i = 0; i < savefile_details->save_data.lengths[CRAY_TYPE_SINT32]; i++){
		printf("%d, ", savefile_details->save_data.s32[i]);
	}
	printf("\n");

	printf("Float\n");
	for(int i = 0; i < savefile_details->save_data.lengths[CRAY_TYPE_FLOAT]; i++){
		printf("%f, ", savefile_details->save_data.floats[i]);
	}
	printf("\n");

	printf("Double\n");
	for(int i = 0; i < savefile_details->save_data.lengths[CRAY_TYPE_DOUBLE]; i++){
		printf("%lf, ", savefile_details->save_data.doubles[i]);
	}
	printf("\n");

	printf("Chars\n");
	for(int i = 0; i < savefile_details->save_data.lengths[CRAY_TYPE_CHAR]; i++){
		printf("%c", savefile_details->save_data.chars[i]);
	}
	printf("(END)\n");

	return;
}

#endif

int main(){
	#ifdef _arch_dreamcast
	#if CRAYON_BOOT_MODE == 1
	int sdRes = mount_ext2_sd();	//This function should be able to mount an ext2 formatted sd card to the /sd dir
	if(sdRes == 0){
		MS_options.sd_present = 1;
	}
	else{
		return 0;
	}
	#endif
	#endif

	crayon_savefile_details_t savefile_details;

	uint8_t res = setup_savefile(&savefile_details);

	#ifdef _arch_dreamcast
	pvr_init_defaults();	//Init kos
	font_init();
	#endif

	//Find the first savefile (if it exists)
	uint8_t iter;
	for(iter = 0; iter < CRAY_SF_NUM_SAVE_DEVICES; iter++){
		if(crayon_savefile_get_memcard_bit(savefile_details.valid_saves, iter)){	//Use the left most VMU
			savefile_details.save_device_id = iter;
			break;
		}
	}

	//Try and load savefile
	crayon_savefile_load_savedata(&savefile_details);

	//No savefile yet
	if(savefile_details.valid_memcards && savefile_details.save_device_id == -1){
		//If we don't already have a savefile, choose a VMU
		if(savefile_details.valid_memcards){
			for(iter = 0; iter < CRAY_SF_NUM_SAVE_DEVICES; iter++){
				if(crayon_savefile_get_memcard_bit(savefile_details.valid_memcards, iter)){	//Use the left most VMU
					savefile_details.save_device_id = iter;
					goto Exit_loop_2;
				}
			}
		}
		Exit_loop_2:
		;
	}

	uint16_t save_res = 0;
	if(savefile_details.valid_memcards){
		save_res = crayon_savefile_save_savedata(&savefile_details);
		crayon_savefile_update_valid_saves(&savefile_details, CRAY_SAVEFILE_UPDATE_MODE_SAVE_PRESENT);	//Updating the save
	}

	#ifdef _arch_dreamcast
	#if CRAYON_BOOT_MODE == 1
		unmount_ext2_sd();	//Unmounts the SD dir to prevent corruption since we won't need it anymore
	#endif
	#endif

	char buffer[70];
	if(!res){
		sprintf(buffer, "Save created\nUses %d blocks and has %d frames of\nanimation",
		crayon_savefile_get_save_block_count(&savefile_details),
		savefile_details.icon_anim_count);
	}
	else{
		sprintf(buffer, "It failed with code %d", res);
	}

	#ifdef _arch_dreamcast
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
			switch(save_res){
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
	char buffer2[32];
	sprintf(buffer2, "save_res: %d\n", save_res);
	draw_string(0, 0, 0, 0, 0, 0, 0, buffer, 0, 0);
	draw_string(0, 0, 0, 0, 0, 0, 0, buffer2, 0, 0);

	// print_all_vars(&savefile_details);

	// var1[0] = 2997;
	// name[2][3] = '1';
	// //Each name is 16 chars and we have 20 names. So this is the 4th char of the 3rd name
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