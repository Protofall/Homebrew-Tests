//Crayon libraries
#include <crayon/memory.h>
#include <crayon/graphics.h>

//For region and htz stuff
#include <dc/flashrom.h>

//For the controller
#include <dc/maple.h>
#include <dc/maple/controller.h>

#include "startup_support.h"

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
crayon_sprite_array_t fade_draw;

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

void crayon_graphics_init_display(){
	STARTUP_Init_Video(FB_RGB565, USE_640x480);
	// STARTUP_Set_Video(FB_RGB565, USE_640x480);	//Normal 480P
	STARTUP_848x480_VGA_CVT_RBv2(FB_RGB565);	//The closest to native 16:9 60hz.
												//The actual (horizontal) FB is a little bit smaller than that

	vid_mode->width = STARTUP_video_params.fb_width;
	vid_mode->height = STARTUP_video_params.fb_height;
	vid_mode->pm = STARTUP_video_params.video_color_type;

	/*
		Do whatever you want.
		After this point you can set one of the extra video modes, but you'll need to do this:

		// Need to overwrite KOS's settings for these specific parameters
		vid_mode->width = STARTUP_video_params.fb_width;
		vid_mode->height = STARTUP_video_params.fb_height;
		vid_mode->pm = STARTUP_video_params.video_color_type;

		After any call to change the video mode via an extra mode or a call to STARTUP_Set_Video().
	*/

	// width = crayon_graphics_get_window_width();
	// height = crayon_graphics_get_window_height();

	width = STARTUP_video_params.fb_width;
	height = STARTUP_video_params.fb_height;

	pvr_init(&pvr_params);

	return;
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

	//load in assets here
	crayon_font_mono_t BIOS_font;
	crayon_palette_t BIOS_P;
	crayon_memory_mount_romdisk("/cd/stuff.img", "/Setup");
	crayon_memory_load_mono_font_sheet(&BIOS_font, &BIOS_P, 0, "/Setup/BIOS.dtex");
	fs_romdisk_unmount("/Setup");
	crayon_graphics_setup_palette(&BIOS_P);

	pvr_set_bg_color(0.3, 0.3, 0.3); // Its useful-ish for debugging

	#if CRAYON_BOOT_MODE == 1
		unmount_fat_sd();	//Unmounts the SD dir to prevent corruption since we won't need it anymore
	#endif

	while(1){
		pvr_wait_ready();
		MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)
			//Poll for input here
		MAPLE_FOREACH_END()


		pvr_scene_begin();


		pvr_list_begin(PVR_LIST_TR_POLY);
			// crayon_graphics_draw_sprites(&fade_draw, NULL, PVR_LIST_TR_POLY, CRAY_DRAW_SIMPLE);
		pvr_list_finish();


		pvr_list_begin(PVR_LIST_OP_POLY);
			crayon_graphics_draw_text_mono("MOOP VIDEO", &BIOS_font, PVR_LIST_OP_POLY, 32, 32, 50, 2, 2, BIOS_P.palette_id);
		pvr_list_finish();


		pvr_list_begin(PVR_LIST_PT_POLY);
			;
		pvr_list_finish();


		pvr_scene_finish();
	}

	crayon_memory_free_sprite_array(&fade_draw);

	STARTUP_Set_Video(FB_RGB0555, USE_640x480);	//Only needed for older dcload
	return 0;
}
