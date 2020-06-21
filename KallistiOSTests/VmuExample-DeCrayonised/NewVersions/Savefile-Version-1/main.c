//For the controller and mouse
#include <dc/maple.h>
#include <dc/maple/controller.h>

#include "setup.h"
#include "graphics.h"

#define CRAYON_DEBUG 0

int main(){
	#if CRAYON_BOOT_MODE == 1
		int sdRes = mount_ext2_sd();	//This function should be able to mount an ext2 formatted sd card to the /sd dir	
		if(sdRes == 0){
			MS_options.sd_present = 1;
		}
		else{
			return 0;
		}
	#endif

	crayon_savefile_details_t savefile_details;

	uint8_t res = setup_savefile(&savefile_details);

	pvr_init_defaults();	//Init kos
	font_init();

	//Find the first savefile (if it exists)
	int iter;
	int jiter;
	for(iter = 0; iter <= 3; iter++){
		for(jiter = 1; jiter <= 2; jiter++){
			if(crayon_savefile_get_vmu_bit(savefile_details.valid_saves, iter, jiter)){	//Use the left most VMU
				savefile_details.save_port = iter;
				savefile_details.save_slot = jiter;
				goto Exit_loop_1;
			}
		}
	}
	Exit_loop_1:

	//Try and load savefile
	crayon_savefile_load(&savefile_details);

	//No savefile yet
	if(savefile_details.valid_memcards && savefile_details.save_port == -1 &&
		savefile_details.save_slot == -1){
		//If we don't already have a savefile, choose a VMU
		if(savefile_details.valid_memcards){
			for(iter = 0; iter <= 3; iter++){
				for(jiter = 1; jiter <= 2; jiter++){
					if(crayon_savefile_get_vmu_bit(savefile_details.valid_memcards, iter, jiter)){	//Use the left most VMU
						savefile_details.save_port = iter;
						savefile_details.save_slot = jiter;
						goto Exit_loop_2;
					}
				}
			}
		}
		Exit_loop_2:
		;
	}

	uint16_t save_res = 0;
	if(savefile_details.valid_memcards){
		save_res = crayon_savefile_save(&savefile_details);
		crayon_savefile_update_valid_saves(&savefile_details, CRAY_SAVEFILE_UPDATE_MODE_SAVE_PRESENT);	//Updating the save
	}

	#if CRAYON_BOOT_MODE == 1
		unmount_ext2_sd();	//Unmounts the SD dir to prevent corruption since we won't need it anymore
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

	crayon_savefile_free(&savefile_details);
	pvr_mem_free(font_tex);

	return 0;
}
