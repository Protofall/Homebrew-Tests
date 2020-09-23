#include <kos.h>

uint16_t lcd_offset = 0;
uint8_t *lcd_icon = NULL;
uint8_t screen_bitmap = 0;
uint8_t frames = 60;

//We use a double pointer because we want to modify the pointer itself with malloc
int16_t setup_vmu_icon_load(uint8_t **vmu_lcd_icon, char *icon_path){
	*vmu_lcd_icon = malloc(6 * 32);	//6 * 32 because we have 48/32 1bpp so we need that / 8 bytes
	FILE * file_lcd_icon = fopen(icon_path, "rb");
	if(!file_lcd_icon){return -1;}

	// If the icon is right, it *must* be a multiple of 192 bytes
	size_t res = fread(*vmu_lcd_icon, 192, frames * 3, file_lcd_icon);
	fclose(file_lcd_icon);

	screen_bitmap = (1 << 8) - 1;

	if(res != frames){
		return 1;
	}

	return 0;
}

uint8_t crayon_savefile_get_vmu_bit(uint8_t vmu_bitmap, int8_t savefile_port, int8_t savefile_slot){
	return !!(vmu_bitmap & (1 << ((2 - savefile_slot) + 6 - (2 * savefile_port))));
}

void crayon_vmu_display_icon(uint8_t vmu_bitmap, void *icon, uint16_t offset){
	maple_device_t *vmu;
	uint32_t true_offset = (192 * offset);
	uint8_t i, j;
	for(i = 0; i <= 3; i++){
		for(j = 1; j <= 2; j++){
			if(crayon_savefile_get_vmu_bit(vmu_bitmap, i, j)){	//We want to display on this VMU
				if(!(vmu = maple_enum_dev(i, j))){	//Device not present
					continue;
				}
				vmu_draw_lcd(vmu, icon + true_offset);
			}
		}
	}

	return;
}

// Draw one frame
void draw_frame(void){
	pvr_wait_ready();	// Wait until the pvr system is ready to output again

	// Empty loop to so we can get a 60FPS rate for the VMU screen
	pvr_scene_begin();
	pvr_scene_finish();

	crayon_vmu_display_icon(screen_bitmap, lcd_icon, lcd_offset);
}

void cleanup(){
	free(lcd_icon);
	// Shut down libraries we used
	pvr_shutdown();
}

// Romdisk
extern uint8 romdisk_boot[];
KOS_INIT_ROMDISK(romdisk_boot);

int main(void){
	pvr_init_defaults();	// Init kos

	setup_vmu_icon_load(&lcd_icon, "/rd/mario_lcd.bin");

	uint32_t prev_buttons[4] = {0};
	uint8_t counter2 = 0;

	//Keep drawing frames until start is pressed
	while(1){
		MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)

		if(st->buttons & CONT_START){	//Quits if start is pressed. Screen goes black
			goto exitmainloop;
		}

		// // Might use this later
		// if((st->buttons & CONT_A) && !(prev_buttons[__dev->port] & CONT_A)){
		// 	colour_state = 0;
		// }

		prev_buttons[__dev->port] = st->buttons;

		MAPLE_FOREACH_END()

		draw_frame();

		lcd_offset++;
		if(lcd_offset >= frames){
			lcd_offset = 0;
		}
	}

	exitmainloop:

	cleanup();	//Free all usage of RAM and do the pvr_shutdown procedure

	return 0;
}
