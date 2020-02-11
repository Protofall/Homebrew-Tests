//Crayon libraries
#include <crayon/memory.h>
#include <crayon/graphics.h>

//For region and htz stuff
#include <dc/flashrom.h>

//For the controller
#include <dc/maple.h>
#include <dc/maple/controller.h>

#if CRAYON_BOOT_MODE == 1
	//For mounting the sd dir
	#include <dc/sd.h>
	#include <kos/blockdev.h>
	#include <fat/fs_fat.h>
#endif


#if CRAYON_BOOT_MODE == 1
	#define MNT_MODE FS_FAT_MOUNT_READONLY

	static void unmount_fat_sd(){
		fs_fat_unmount("/sd");
		fs_fat_shutdown();
		sd_shutdown();
	}

	static int mount_fat_sd(){
		kos_blockdev_t sd_dev;
		uint8 partition_type;

		// Initialize the sd card if its present
		if(sd_init()){
			return 1;
		}

		// Grab the block device for the first partition on the SD card. Note that
		// you must have the SD card formatted with an MBR partitioning scheme
		if(sd_blockdev_for_partition(0, &sd_dev, &partition_type)){
			return 2;
		}

		// Check to see if the MBR says that we have a valid partition
		// if(partition_type != 0x83){
			//I don't know what value I should be comparing against, hence this check is disabled for now
			// This: https://en.wikipedia.org/wiki/Partition_type
				//Suggests there's multiple types for FAT...not sure how to handle this
			// return 3;
		// }

		// Initialize fs_fat and attempt to mount the device
		if(fs_fat_init()){
			return 4;
		}

		//Mount the SD card to the sd dir in the VFS
		if(fs_fat_mount("/sd", &sd_dev, MNT_MODE)){
			return 5;
		}
		return 0;
	}

#endif

//Change this to only give room for PT list (Since other ones aren't used)
pvr_init_params_t pvr_params = {
		// Enable opaque, translucent and punch through polygons with size 16
			//To better explain, the opb_sizes or Object Pointer Buffer sizes
			//Work like this: Set to zero to disable. Otherwise the higher the
			//number the more space used (And more efficient under higher loads)
			//The lower the number, the less space used and less efficient under
			//high loads. You can choose 0, 8, 16 or 32
		{ PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16 },

		// Vertex buffer size 512K
		512 * 1024,

		// No DMA
		0,

		// No FSAA
		0,

		// Translucent Autosort enabled
		0
};

float width, height;
crayon_font_mono_t BIOS_font;
crayon_sprite_array_t fade_draw;

crayon_font_mono_t Default_Font;
crayon_palette_t Default_Font_P;

float g_deadspace;

uint8_t g_htz, g_htz_adjustment;
uint8_t vga_enabled;

void init_fade_struct(){
	crayon_memory_init_sprite_array(&fade_draw, NULL, 0, NULL, 1, 0, 0, PVR_FILTER_NONE, 0);
	fade_draw.coord[0].x = 0;
	fade_draw.coord[0].y = 0;
	fade_draw.scale[0].x = width;
	fade_draw.scale[0].y = height;
	fade_draw.fade[0] = 254;
	fade_draw.colour[0] = 0xFFFF0000;	//Full Black (Currently red for debugging)
	fade_draw.rotation[0] = 0;
	fade_draw.visible[0] = 1;
	fade_draw.layer[0] = 255;
}

void htz_select(){
	//If we have a VGA cable, then skip this screen
	if(vga_enabled){return;}

	#if CRAYON_BOOT_MODE == 0
		crayon_memory_mount_romdisk("/cd/stuff.img", "/Setup");
	#elif CRAYON_BOOT_MODE == 1
		crayon_memory_mount_romdisk("/sd/stuff.img", "/Setup");
	#elif CRAYON_BOOT_MODE == 2
		crayon_memory_mount_romdisk("/pc/stuff.img", "/Setup");
	#else
		#error "Invalid CRAYON_BOOT_MODE"
	#endif

	crayon_palette_t BIOS_P;		//Entry 0
	crayon_palette_t BIOS_Red_P;	//Entry 1

	crayon_memory_load_mono_font_sheet(&BIOS_font, &BIOS_P, 0, "/Setup/BIOS.dtex");

	fs_romdisk_unmount("/Setup");

	//Make the red font
	crayon_memory_clone_palette(&BIOS_P, &BIOS_Red_P, 1);
	crayon_memory_swap_colour(&BIOS_Red_P, 0xFFAAAAAA, 0xFFFF0000, 0);

	//Set the palettes in PVR memory
	crayon_graphics_setup_palette(&BIOS_P);	//0
	crayon_graphics_setup_palette(&BIOS_Red_P);	//1

	uint16_t htz_head_x = (width - (2 * crayon_graphics_string_get_length_mono(&BIOS_font, "Select Refresh Rate"))) / 2;
	uint16_t htz_head_y = 133;
	uint16_t htz_option_y = htz_head_y + (BIOS_font.char_height * 2) + 80;

	//Set the palettes for each option
		//Red is highlighted, white is normal
	int8_t * palette_50htz,* palette_60htz;

	if(g_htz == 50){	//For highlights
		palette_50htz = &BIOS_Red_P.palette_id;
		palette_60htz = &BIOS_P.palette_id;
	}
	else{
		palette_50htz = &BIOS_P.palette_id;
		palette_60htz = &BIOS_Red_P.palette_id;
	}
	
	int16_t counter = g_htz  * 11;	//Set this to 11 secs (So current fps * 11)
									//We choose 11 seconds because the fade-in takes 1 second

	uint8_t fade_frame_count = 0;
	float fade_cap = g_htz * 1.0;	//1 second, 60 or 50 frames

	//While counting || fade is not full alpha
	while(counter > 0 || crayon_assist_extract_bits(fade_draw.colour[0], 8, 24) != 0xFF){

		//Update the fade-in/out effects
		counter--;
		if(counter < 0){	//Fading out
			if(crayon_assist_extract_bits(fade_draw.colour[0], 8, 24) == 0){
				//Have another sound, not a blip, but some acceptance thing
			}
			// fade = 0xFF * (fade_frame_count / fade_cap);
			fade_draw.colour[0] = (uint32_t)(0xFF * (fade_frame_count / fade_cap)) << 24;
			fade_frame_count++;
		}
		else if(crayon_assist_extract_bits(fade_draw.colour[0], 8, 24) > 0){	//fading in
			// fade = 0xFF - (0xFF * (fade_frame_count / fade_cap));
			fade_draw.colour[0] = (uint32_t)(0xFF - (0xFF * (fade_frame_count / fade_cap))) << 24;
			fade_frame_count++;
		}
		else{	//10 seconds of choice
			fade_frame_count = 0;
		}

		pvr_wait_ready();	//Need vblank for inputs

		//Don't really want players doing stuff when it fades out
		if(counter >= 0){
			MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)
			if(st->buttons & CONT_DPAD_LEFT){
				palette_50htz = &BIOS_Red_P.palette_id;	//0 is 50 Htz
				palette_60htz = &BIOS_P.palette_id;
				//Make "blip" sound
			}
			else if(st->buttons & CONT_DPAD_RIGHT){
				palette_50htz = &BIOS_P.palette_id;	//1 is 60 Htz
				palette_60htz = &BIOS_Red_P.palette_id;
				//Make "blip" sound
			}

			//Press A or Start to skip the countdown
			if((st->buttons & CONT_START) || (st->buttons & CONT_A)){
				counter = 0;
			}

			MAPLE_FOREACH_END()
		}

		pvr_scene_begin();

		pvr_list_begin(PVR_LIST_TR_POLY);

			//The fading in/out effect
			if(crayon_assist_extract_bits(fade_draw.colour[0], 8, 24)){
				crayon_graphics_draw_sprites(&fade_draw, NULL, PVR_LIST_TR_POLY, CRAY_DRAW_SIMPLE);
			}

		pvr_list_finish();

		pvr_list_begin(PVR_LIST_OP_POLY);

			crayon_graphics_draw_text_mono("Select Refresh Rate", &BIOS_font, PVR_LIST_OP_POLY, htz_head_x, htz_head_y, 50, 2, 2,
				BIOS_P.palette_id);
			crayon_graphics_draw_text_mono("50Hz", &BIOS_font, PVR_LIST_OP_POLY, 40, htz_option_y, 50, 2, 2, *palette_50htz);
			crayon_graphics_draw_text_mono("60Hz", &BIOS_font, PVR_LIST_OP_POLY, 480, htz_option_y, 50, 2, 2, *palette_60htz);

		pvr_list_finish();

		pvr_scene_finish();

	}

	if(g_htz == 50 && *palette_60htz == BIOS_Red_P.palette_id){
		g_htz = 60;
		g_htz_adjustment = 1;
		vid_set_mode(DM_640x480_NTSC_IL, PM_RGB565);
	}
	else if(g_htz == 60 && *palette_50htz == BIOS_Red_P.palette_id){
		g_htz = 50;
		g_htz_adjustment = 1.2;
		vid_set_mode(DM_640x480_PAL_IL, PM_RGB565);
	}

	crayon_memory_free_palette(&BIOS_P);
	crayon_memory_free_palette(&BIOS_Red_P);

	crayon_memory_free_mono_font_sheet(&BIOS_font);

	return;
}

void init_default_font(){
	#if CRAYON_BOOT_MODE == 0
		crayon_memory_mount_romdisk("/cd/stuff.img", "/Font");
	#elif CRAYON_BOOT_MODE == 1
		crayon_memory_mount_romdisk("/sd/stuff.img", "/Font");
	#elif CRAYON_BOOT_MODE == 2
		crayon_memory_mount_romdisk("/pc/stuff.img", "/Font");
	#else
		#error "Invalid CRAYON_BOOT_MODE"
	#endif

	crayon_memory_load_mono_font_sheet(&Default_Font, &Default_Font_P, 63, "/Font/BIOS.dtex");	//Uses the last palette

	fs_romdisk_unmount("/Font");

	//Set the palettes in PVR memory
	crayon_graphics_setup_palette(&Default_Font_P);	//63
}

void crayon_graphics_init_display(){
	pvr_init(&pvr_params);

	width = crayon_graphics_get_window_width();
	height = crayon_graphics_get_window_height();

	vga_enabled = (vid_check_cable() == CT_VGA);
	if(vga_enabled){
		vid_set_mode(DM_640x480_VGA, PM_RGB565);	//60Hz
		g_htz = 60;
		g_htz_adjustment = 1;
	}
	else{
		if(flashrom_get_region() == FLASHROM_REGION_EUROPE){
			vid_set_mode(DM_640x480_PAL_IL, PM_RGB565);		//50Hz
			g_htz = 50;
			g_htz_adjustment = 1.2;
		}
		else{
			vid_set_mode(DM_640x480_NTSC_IL, PM_RGB565);	//60Hz
			g_htz = 60;
			g_htz_adjustment = 1;
		}
	}

	return;
}

float thumbstick_int_to_float(int joy){
	float ret_val;	//Range from -1 to 1

	if(joy > 0){	//Converting from -128, 127 to -1, 1
		ret_val = joy / 127.0;
	}
	else{
		ret_val = joy / 128.0;
	}

	return ret_val;
}

int main(){
	#if CRAYON_BOOT_MODE == 1
		int sdRes = mount_fat_sd();	//This function should be able to mount a FAT32 formatted sd card to the /sd dir	
		if(sdRes != 0){
			error_freeze("SD card couldn't be mounted: %d", sdRes);
		}
	#endif

	g_deadspace = 0.4;

	crayon_graphics_init_display();
	init_fade_struct();
	init_default_font();

	// htz_select();

	pvr_set_bg_color(0.3, 0.3, 0.3); // Its useful-ish for debugging

	#if CRAYON_BOOT_MODE == 1
		unmount_fat_sd();	//Unmounts the SD dir to prevent corruption since we won't need it anymore
	#endif

	char product_name[30];

	char text[20][32];	//16 buttons and the 2 thumbsticks and 2 triggers (20 total) with 32 chars each
	uint16_t i;
	vec2_f_t thumb;

	//Do stuff for previous thumbsticks/triggers
	while(1){
		pvr_wait_ready();

		//We only read the input for the first controller in line
		MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)
			strcpy(product_name, __dev->info.product_name);
			// if(!strcmp(__dev->info.product_name, "Dreamcast Controller         ")){while(1);}	//Triggers
			// if(!strcmp(__dev->info.product_name, "Twin Stick                   ")){while(1);}
			
			if(st->buttons & CONT_C){sprintf(text[0], "C pressed");}
			else{sprintf(text[0], "-");}

			if(st->buttons & CONT_B){sprintf(text[1], "B pressed");}
			else{sprintf(text[1], "-");}

			if(st->buttons & CONT_A){sprintf(text[2], "A pressed");}
			else{sprintf(text[2], "-");}

			if(st->buttons & CONT_START){sprintf(text[3], "START pressed");}
			else{sprintf(text[3], "-");}

			if(st->buttons & CONT_DPAD_UP){sprintf(text[4], "DPAD1_UP pressed");}
			else{sprintf(text[4], "-");}

			if(st->buttons & CONT_DPAD_DOWN){sprintf(text[5], "DPAD1_DOWN pressed");}
			else{sprintf(text[5], "-");}

			if(st->buttons & CONT_DPAD_LEFT){sprintf(text[6], "DPAD1_LEFT pressed");}
			else{sprintf(text[6], "-");}

			if(st->buttons & CONT_DPAD_RIGHT){sprintf(text[7], "DPAD1_RIGHT pressed");}
			else{sprintf(text[7], "-");}

			if(st->buttons & CONT_Z){sprintf(text[8], "Z pressed");}
			else{sprintf(text[8], "-");}

			if(st->buttons & CONT_Y){sprintf(text[9], "Y pressed");}
			else{sprintf(text[9], "-");}

			if(st->buttons & CONT_X){sprintf(text[10], "X pressed");}
			else{sprintf(text[10], "-");}

			if(st->buttons & CONT_D){sprintf(text[11], "D pressed");}
			else{sprintf(text[11], "-");}

			if(st->buttons & CONT_DPAD2_UP){sprintf(text[12], "DPAD2_UP pressed");}
			else{sprintf(text[12], "-");}

			if(st->buttons & CONT_DPAD2_DOWN){sprintf(text[13], "DPAD2_DOWN pressed");}
			else{sprintf(text[13], "-");}

			if(st->buttons & CONT_DPAD2_LEFT){sprintf(text[14], "DPAD2_LEFT pressed");}
			else{sprintf(text[14], "-");}

			if(st->buttons & CONT_DPAD2_RIGHT){sprintf(text[15], "DPAD2_RIGHT pressed");}
			else{sprintf(text[15], "-");}

			thumb.x = thumbstick_int_to_float(st->joyx);
			thumb.y = thumbstick_int_to_float(st->joyy);
			if((thumb.x * thumb.x) + (thumb.y * thumb.y) > g_deadspace * g_deadspace){
				sprintf(text[16], "THUMBSTICK1 active");
			}
			else{sprintf(text[16], "-");}

			thumb.x = thumbstick_int_to_float(st->joy2x);
			thumb.y = thumbstick_int_to_float(st->joy2y);
			if((thumb.x * thumb.x) + (thumb.y * thumb.y) > g_deadspace * g_deadspace){
				sprintf(text[17], "THUMBSTICK2 active");
			}
			else{sprintf(text[17], "-");}

			//Triggers start at 0 and go to 255 I think
			if(st->ltrig >= 255 * 0.2){sprintf(text[18], "LEFT_TRIGGER active");}
			else{sprintf(text[18], "-");}

			if(st->rtrig >= 255 * 0.2){sprintf(text[19], "RIGHT_TRIGGER active");}
			else{sprintf(text[19], "-");}



			goto done_reading;

		MAPLE_FOREACH_END()

		done_reading:
		;


		pvr_scene_begin();


		pvr_list_begin(PVR_LIST_OP_POLY);
			crayon_graphics_draw_text_mono("Buttons Pressed: ", &Default_Font, PVR_LIST_OP_POLY, 32, 24, 50, 2, 2, Default_Font_P.palette_id);
			for(i = 0; i < 20; i++){
				crayon_graphics_draw_text_mono(text[i], &Default_Font, PVR_LIST_OP_POLY, 32, 24 + ((i+3) * Default_Font.char_height),
					50, 1, 1, Default_Font_P.palette_id);
			}

			crayon_graphics_draw_text_mono(product_name, &Default_Font, PVR_LIST_OP_POLY, 150, 24 + (3 * Default_Font.char_height),
					50, 1, 1, Default_Font_P.palette_id);


		pvr_list_finish();


		pvr_scene_finish();
	}

	crayon_memory_free_sprite_array(&fade_draw);

	crayon_memory_free_palette(&Default_Font_P);
	crayon_memory_free_mono_font_sheet(&Default_Font);

	pvr_shutdown();

	return 0;
}
