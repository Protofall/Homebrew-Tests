#include <kos.h>
#include <png/png.h>	//For the png_to_texture function

// Texture
pvr_ptr_t pic, nerstr, controls;		//To store the image from pic.png, nerstr.png and controls.png

uint8_t colour_state;
float alpha = 0.5f;

// Init pic
void pic_init(){
	pic = pvr_mem_malloc(256 * 256 * 2);
	png_to_texture("/rd/pic.png", pic, PNG_NO_ALPHA);

	nerstr = pvr_mem_malloc(128 * 128 * 2);
	png_to_texture("/rd/nerstr.png", nerstr, PNG_FULL_ALPHA);

	controls = pvr_mem_malloc(256 * 256 * 2);
	png_to_texture("/rd/controls.png", controls, PNG_NO_ALPHA);

	return;
}

void draw_texture(pvr_ptr_t name, uint16_t x, uint16_t y, int dim, uint8_t list, int textureformat, int filtering){
	int z = 1;
	pvr_poly_cxt_t cxt;
	pvr_poly_hdr_t hdr;
	pvr_vertex_t vert[4];

	pvr_poly_cxt_txr(&cxt, list, textureformat, dim, dim, name, filtering);
	pvr_poly_compile(&hdr, &cxt);
	hdr.cmd |= 4;	//Enable oargb
	pvr_prim(&hdr, sizeof(hdr));

	//Only nerstr is in the OP list so this will work fine
	if(list == PVR_LIST_TR_POLY){
		if(colour_state == 0){
			float invalpha = (1.0f - alpha);
			vert[0].argb = PVR_PACK_COLOR(1.0f, invalpha, invalpha, invalpha);
			vert[0].oargb = PVR_PACK_COLOR(1, (0.6f * alpha), (0.6f * alpha), (1.0f * alpha));	//Fade to blue
			// vert[0].oargb = PVR_PACK_COLOR(1, 0.0f, 0.0f, 0.0f);	//Fade to black
		}
		else if(colour_state == 1){
			vert[0].argb = 0xffffffff;
			vert[0].oargb = PVR_PACK_COLOR(1, (0.6f * alpha), (0.6f * alpha), (1.0f * alpha));	//Add blue
		}
		else if(colour_state == 2){
			vert[0].argb = 0xffffffff;
			vert[0].oargb = PVR_PACK_COLOR(1, alpha, alpha, alpha);	//Fade to white (By adding white)
		}
		
	}
	else{
		vert[0].argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
		vert[0].oargb = 0;
	}

	vert[0].flags = PVR_CMD_VERTEX;    //I think this is used to define the start of a new polygon

	uint8_t i;
	for(i = 1; i < 4; i++){
		vert[i].argb = vert[0].argb;
		vert[i].oargb = vert[0].oargb;
		vert[i].flags = PVR_CMD_VERTEX;

	}
	vert[3].flags = PVR_CMD_VERTEX_EOL;

	vert[0].x = x;
	vert[0].y = y;
	vert[0].z = z;
	vert[0].u = 0.0;
	vert[0].v = 0.0;

	vert[1].x = x + dim;
	vert[1].y = y;
	vert[1].z = z;
	vert[1].u = 1;
	vert[1].v = 0.0;

	vert[2].x = x;
	vert[2].y = y + dim;
	vert[2].z = z;
	vert[2].u = 0.0;
	vert[2].v = 1;

	vert[3].x = x + dim;
	vert[3].y = y + dim;
	vert[3].z = z;
	vert[3].u = 1;
	vert[3].v = 1;
	pvr_prim(&vert, sizeof(pvr_vertex_t) * 4);
}

// Draw one frame
void draw_frame(void){
	pvr_wait_ready();	//Wait until the pvr system is ready to output again
	pvr_scene_begin();

	pvr_list_begin(PVR_LIST_OP_POLY);
	draw_texture(pic, 0, 0, 256, PVR_LIST_OP_POLY, PVR_TXRFMT_RGB565, PVR_FILTER_NONE);
	draw_texture(controls, 260, 30, 256, PVR_LIST_OP_POLY, PVR_TXRFMT_RGB565, PVR_FILTER_NONE);
	pvr_list_finish();

	pvr_list_begin(PVR_LIST_TR_POLY);
	draw_texture(nerstr, 50, 50, 128, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB4444, PVR_FILTER_NONE);
	pvr_list_finish();

	pvr_scene_finish();
}

void cleanup(){
	// Clean up the texture memory we allocated earlier
	pvr_mem_free(pic);
	pvr_mem_free(nerstr);
	pvr_mem_free(controls);
	
	// Shut down libraries we used
	pvr_shutdown();
}

// Romdisk
extern uint8 romdisk_boot[];
KOS_INIT_ROMDISK(romdisk_boot);

int main(void){
	pvr_init_defaults();	//Init kos
	pic_init();				//Init pic

	uint32_t prev_buttons[4] = {0};
	colour_state = 0;

	//Keep drawing frames until start is pressed
	while(1){
		MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)

		if(st->buttons & CONT_START){	//Quits if start is pressed. Screen goes black
				// MAPLE_FOREACH_BEGIN is a hidden loop, a "break" here is not enough to exit the while(1),
				// it will just exit the FOREACH and skip reading some of the controllers. So jump out of
				// the entire loop with a goto.
				
				// But for this simple demo, clean up really isn't required, a return would be fine as well.
			goto exitmainloop;
		}

		if((st->buttons & CONT_A) && !(prev_buttons[__dev->port] & CONT_A)){
			colour_state = 0;
		}
		else if((st->buttons & CONT_B) && !(prev_buttons[__dev->port] & CONT_B)){
			colour_state = 1;
		}
		else if((st->buttons & CONT_X) && !(prev_buttons[__dev->port] & CONT_X)){
			colour_state = 2;
		}
		
		if(st->buttons & CONT_DPAD_UP){
			alpha += 0.02;
		}
		if(st->buttons & CONT_DPAD_DOWN){
			alpha -= 0.02;
		}

		prev_buttons[__dev->port] = st->buttons;

		MAPLE_FOREACH_END()
		
		if(alpha < 0.0f){
			alpha = 0.0f;
		}
		else if(alpha > 1.0f){
			alpha = 1.0f;
		}

		draw_frame();
	}
exitmainloop:

	cleanup();	//Free all usage of RAM and do the pvr_shutdown procedure

	return 0;
}
