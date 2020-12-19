#include <kos.h>
#include <png/png.h>	// For the png_to_texture function

#include <stdlib.h>     // srand, rand
#include <time.h>       // time

// Texture
pvr_ptr_t pic;				// To store the image from pic.png

uint16_t dim = 8;
uint8_t hardware_crop_mode = 0;	// 0 for no cropping, 2 for keep inside, 3 for keep outside

float *vals_x, *vals_y;
int num_sprites = 100;

// The boundries for TA clipping
	// 160, 128, 480 and 352 in the end
int camera_clip_x1;
int camera_clip_y1;
int camera_clip_x2;
int camera_clip_y2;


// -------------------------------------------------


// Hardware cropping structs/functions
typedef struct ta_userclip_cmd{
	int cmd;
	int padding[3];
	int minx;
	int miny;
	int maxx;
	int maxy;
} ta_userclip_cmd_t;

void set_userclip(unsigned int minx, unsigned int miny, unsigned int maxx, unsigned int maxy){
	ta_userclip_cmd_t clip;

	// Check if the dimension values are correct
	// Those "(blah & ((1 << 5) - 1)) != 0" are the equivalent of "blah % 32 != 0", but faster
	if((minx & ((1 << 5) - 1)) != 0 || (miny & ((1 << 5) - 1)) != 0 || (maxx & ((1 << 5) - 1)) != 0 ||
		(maxy & ((1 << 5) - 1)) != 0 || minx >= maxx || miny >= maxy || maxx > 1280 || maxy > 480){
		return;
	}
	
	clip.cmd = PVR_CMD_USERCLIP;
	clip.minx = minx >> 5;
	clip.miny = miny >> 5;
	clip.maxx = (maxx >> 5) - 1;
	clip.maxy = (maxy >> 5) - 1;	
	
	// Send it to the TA
	pvr_prim(&clip, sizeof(clip));
}


// -------------------------------------------------


// Init pic
void pic_init(){
	pic = pvr_mem_malloc(dim * dim * 2);
	png_to_texture("/rd/Insta.png", pic, PNG_NO_ALPHA);
}

void draw_pic(int x, int y, int z, float x_scale, float y_scale, uint8_t list_type){
	pvr_poly_cxt_t cxt;
	pvr_poly_hdr_t hdr;
	pvr_vertex_t vert;

	pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_RGB565, dim, dim, pic, PVR_FILTER_NONE);

	// Hardware cropping stuff
	// cxt.gen.shading = PVR_SHADE_FLAT;	// Its PVR_SHADE_GOURAUD by default
	cxt.gen.clip_mode = hardware_crop_mode;	// Seems to be off by default
	// cxt.blend.src = PVR_BLEND_SRCALPHA;
	// cxt.blend.dst = PVR_BLEND_INVSRCALPHA;

	pvr_poly_compile(&hdr, &cxt);
	pvr_prim(&hdr, sizeof(hdr));

	vert.argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
	vert.oargb = 0;
	vert.flags = PVR_CMD_VERTEX;

	// These define the verticies of the triangles "strips" (One triangle uses verticies of other triangle)
	vert.x = x;
	vert.y = y;
	vert.z = z;
	vert.u = 0.0;
	vert.v = 0.0;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x + (dim * x_scale);
	vert.y = y;
	vert.z = z;
	vert.u = 1;
	vert.v = 0.0;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x;
	vert.y = y + (dim * y_scale);
	vert.z = z;
	vert.u = 0.0;
	vert.v = 1;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x + (dim * x_scale);
	vert.y = y + (dim * y_scale);
	vert.z = z;
	vert.u = 1;
	vert.v = 1;
	vert.flags = PVR_CMD_VERTEX_EOL;
	pvr_prim(&vert, sizeof(vert));
}


void draw_untextured_poly(int x0, int y0, int x1, int y1, float r, float g, float b, uint8_t list_type){
	int z = 1;

	pvr_poly_cxt_t cxt;
	pvr_poly_hdr_t hdr;
	pvr_vertex_t vert;

	pvr_poly_cxt_col(&cxt, list_type);

	// Hardware cropping stuff
	// cxt.gen.shading = PVR_SHADE_FLAT;	// Its PVR_SHADE_GOURAUD by default
	cxt.gen.clip_mode = hardware_crop_mode;	// Seems to be off by default
	// cxt.blend.src = PVR_BLEND_SRCALPHA;	// If (list_type > PVR_LIST_OP_MOD) (AKA not an OP list of any kind) then
										// These are the default, otherwise the defaults are PVR_BLEND_ONE and PVR_BLEND_ZERO
	// cxt.blend.dst = PVR_BLEND_INVSRCALPHA;

	pvr_poly_compile(&hdr, &cxt);
	pvr_prim(&hdr, sizeof(hdr));

	vert.argb = PVR_PACK_COLOR(1.0f, r, g, b);
	vert.oargb = 0;
	vert.flags = PVR_CMD_VERTEX;

	// These define the verticies of the triangles "strips" (One triangle uses verticies of other triangle)
	vert.x = x0;
	vert.y = y0;
	vert.z = z;
	vert.u = 0.0;
	vert.v = 0.0;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x1;
	vert.y = y0;
	vert.z = z;
	vert.u = 1;
	vert.v = 0.0;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x0;
	vert.y = y1;
	vert.z = z;
	vert.u = 0.0;
	vert.v = 1;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x1;
	vert.y = y1;
	vert.z = z;
	vert.u = 1;
	vert.v = 1;
	vert.flags = PVR_CMD_VERTEX_EOL;
	pvr_prim(&vert, sizeof(vert));
}

// Draw one frame
void draw_frame(void){
	int i;

	pvr_wait_ready();	// Wait until the pvr system is ready to output again
	pvr_scene_begin();

	// NOTE. Doing any clips outside the lists just don't work. idk why
	// set_userclip(0, 0, 640, 480);

	pvr_list_begin(PVR_LIST_OP_POLY);

		// Set the user/hardware clip
		set_userclip(camera_clip_x1, camera_clip_y1, camera_clip_x2, camera_clip_y2);

		draw_untextured_poly(camera_clip_x1, camera_clip_y1, camera_clip_x2, camera_clip_y2,
				0.5, 0.5, 0.5, PVR_LIST_OP_POLY);

		for(i = 0; i < num_sprites; i++){
			draw_pic(vals_x[i], vals_y[i], i + 2, 6, 6, PVR_LIST_OP_POLY);
		}

	pvr_list_finish();

	// pvr_list_begin(PVR_LIST_TR_POLY);

	// pvr_list_finish();

	pvr_scene_finish();
}

void cleanup(){
	// Clean up the texture memory we allocated earlier
	pvr_mem_free(pic);
	
	// Shut down libraries we used
	pvr_shutdown();

	free(vals_x);
	free(vals_y);
}

// Romdisk
extern uint8 romdisk_boot[];
KOS_INIT_ROMDISK(romdisk_boot);

void sprite_positions(){
	int i;
	for(i = 0; i < num_sprites; i++){
		// Generate rand_x/y
		vals_x[i] = rand() % (640 - (6 * dim));
		vals_y[i] = rand() % (480 - (6 * dim));
	}

	return;
}

int main(void){
	int done = 0;

	srand(time(0));

	pvr_init_defaults();		// Init pvr

	pic_init();			// Init pic

	// 640 wide, 20 tiles wide
	// 480 high, 15 tiles high
	camera_clip_x1 = (5 * 32);
	camera_clip_y1 = (4 * 32);
	camera_clip_x2 = 640 - camera_clip_x1;
	camera_clip_y2 = 480 - camera_clip_y1;

	int i;
	uint32_t curr_buttons[4] = {0};
	uint32_t prev_buttons[4] = {0};

	vals_x = malloc(sizeof(float) * num_sprites);
	vals_y = malloc(sizeof(float) * num_sprites);

	sprite_positions();

	// Keep drawing frames until start is pressed
	while(!done){
		MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)

		prev_buttons[__dev->port] = curr_buttons[__dev->port];
		curr_buttons[__dev->port] = st->buttons;

		MAPLE_FOREACH_END()

		for(i = 0; i < 4; i++){
			// Quits if start is pressed. Screen goes black
			if(curr_buttons[i] & CONT_START){
				done = 1;
				break;
			}

			// Will toggle between no cropping, keeping inside the box and outside the box outside
			if((curr_buttons[i] & CONT_A) && !(prev_buttons[i] & CONT_A)){
				if(hardware_crop_mode == PVR_USERCLIP_DISABLE){
					hardware_crop_mode = PVR_USERCLIP_INSIDE;
				}
				else if(hardware_crop_mode == PVR_USERCLIP_INSIDE){
					hardware_crop_mode = PVR_USERCLIP_OUTSIDE;
				}
				else{
					hardware_crop_mode = PVR_USERCLIP_DISABLE;
				}
			}

			// Regenerates the sprite x/y positions
			if((curr_buttons[i] & CONT_X) && !(prev_buttons[i] & CONT_X)){
				sprite_positions();
			}
		}

		draw_frame();
	}

	cleanup();	// Free all usage of RAM and do the pvr_shutdown procedure

	return 0;
}
