/*
	Controls:
	Hold X - 4 way splitscreen
	Hold Y - Random square demo
	Hold B - Disable clipping
	Press A - Print GPU render time to console
*/

#include <kos.h>
#include <stdlib.h>
#include <math.h>
#include <png/png.h>
#include "userclip.h"

extern uint8 romdisk[];
KOS_INIT_ROMDISK(romdisk);

//CONTROLS
int start_pressed, a_pressed, b_pressed, x_pressed, y_pressed;
int a_last, a_click;

void check_inputs() {
	maple_device_t *cont;
	cont_state_t *state;

	cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

	start_pressed = 0;
	a_pressed = 0;
	
	if (cont) {
		state = (cont_state_t *)maple_dev_status(cont);

		if(state) {
			start_pressed = state->buttons & CONT_START;
			a_pressed = state->buttons & CONT_A;
			b_pressed = state->buttons & CONT_B;
			x_pressed = state->buttons & CONT_X;
			y_pressed = state->buttons & CONT_Y;
			a_click = a_pressed && !a_last;
			a_last = a_pressed;
		}
	}
}

// RENDERING
pvr_poly_hdr_t hdr;

void pvr_setup() {
	pvr_init_params_t pvr_params = {
	    { PVR_BINSIZE_8, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0 },
	    512 * 1024
	};
	
	//Init hardware
	pvr_init(&pvr_params);
	pvr_set_bg_color(0, 0, .3);
	
	//Load texture
	void *pvrptr = 0;
	uint32_t w = 0, h = 0;
	int failure = png_load_texture("/rd/circle.png", &pvrptr, PNG_FULL_ALPHA, &w, &h);
	if (failure) {
		printf("Error loading texture %i %p\n", failure, pvrptr);
		fflush(stdout);
		exit(1);
	}
	
	//Prepare header
	pvr_poly_cxt_t cxt;
	//pvr_poly_cxt_col(&cxt, PVR_LIST_TR_POLY);
	pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY,
		PVR_TXRFMT_ARGB4444, w, h, pvrptr, PVR_FILTER_BILINEAR);
	
	cxt.gen.shading = PVR_SHADE_FLAT;
	cxt.gen.clip_mode = PVR_USERCLIP_INSIDE;
	cxt.blend.src = PVR_BLEND_SRCALPHA;
	cxt.blend.dst = PVR_BLEND_INVSRCALPHA;
	
	pvr_poly_compile(&hdr, &cxt);
}

void set_userclip(unsigned int minx, unsigned int miny, unsigned int maxx, unsigned int maxy) {
	struct ta_userclip_cmd clip;
	
	//Generate userclip command
	make_userclip(&clip, minx, miny, maxx, maxy);
	
	//Send it to the TA
	pvr_prim(&clip, sizeof(clip));
}

inline void draw_sprite(float x, float y, float z, float h, float w, unsigned int argb)
{
	pvr_vertex_t v[4];
	
	v[0].flags = v[1].flags = v[2].flags = PVR_CMD_VERTEX;
	v[3].flags = PVR_CMD_VERTEX_EOL;
	
	v[0].x = x;
	v[0].y = y + h;
	v[1].x = x;
	v[1].y = y;
	v[2].x = x + w;
	v[2].y = y + h;
	v[3].x = x + w;
	v[3].y = y;
	v[0].z = v[1].z = v[2].z = v[3].z = z;
	
	//Not needed for flat shaded polygons
	//v[0].argb = v[1].argb = argb;
	v[2].argb = v[3].argb = argb;
	
	v[0].u = 0;
	v[0].v = 1;
	v[1].u = 0;
	v[1].v = 0;
	v[2].u = 1;
	v[2].v = 1;
	v[3].u = 1;
	v[3].v = 0;
	
	pvr_prim(&v, sizeof(v));
}

#define PARTICLE_SIZE	16

void draw_subscreen_ring(float x, float y, float w, float h, uint32_t color, int particlecount, float angle, float radius) {
	if (!b_pressed)
		set_userclip(x, y, x+w, y+h);
	
	int i;
	float step = 2 * M_PI / particlecount;
	
	for(i = 0; i < particlecount; i++) {
		float s, c;
		sincosf(angle+i*step, &s, &c);
		s *= radius;
		c *= radius;
		draw_sprite(320 + s, 240 + c, 0.5, PARTICLE_SIZE, PARTICLE_SIZE, color);
	}
}

inline float frnd() {
	return (float)rand() / RAND_MAX;
}

void draw_subscreen_rand(float x, float y, float w, float h, uint32_t color, int particlecount) {
	if (!b_pressed)
		set_userclip(x, y, x+w, y+h);
	
	//Shift the window position&size so we generate particles off the left/top edge
	x -= PARTICLE_SIZE;
	y -= PARTICLE_SIZE;
	w += PARTICLE_SIZE;
	h += PARTICLE_SIZE;
	
	do {
		draw_sprite(frnd() * w + x, frnd() * h + y, 0.5, PARTICLE_SIZE, PARTICLE_SIZE, color);
	} while(--particlecount);
}

void draw_transparent() {
	//Set clip area to full screen so user can disable clipping
	set_userclip(0, 0, 640, 480);

	//Rotation for ring demo
	static float rot = 0;
	rot += 0.01;
	if (rot > 2*M_PI)
		rot -= 2*M_PI;

	pvr_prim(&hdr, sizeof(hdr));

	if (y_pressed) {
		if (x_pressed) {
			draw_subscreen_rand(0, 0, 320, 256, 0x40ff0000, 700);
			draw_subscreen_rand(320, 0, 320, 256, 0x4000ff00, 700);
			draw_subscreen_rand(0, 256, 320, 224, 0x40ffff00, 700);
			draw_subscreen_rand(320, 256, 320, 224, 0x40ffffff, 700);
		} else {
			draw_subscreen_rand(0, 0, 320, 480, 0x40ff0000, 1400);
			draw_subscreen_rand(320, 0, 320, 480, 0x4000ff00, 1400);
		}
	} else {
		if (x_pressed) {
			draw_subscreen_ring(0, 0, 320, 256, 0xffff0000, 32, rot, 200);
			draw_subscreen_ring(320, 0, 320, 256, 0xff00ff00, 32, rot, 170);
			draw_subscreen_ring(0, 256, 320, 224, 0xffffff00, 32, rot, 140);
			draw_subscreen_ring(320, 256, 320, 224, 0xffffffff, 32, rot, 110);
		} else {
			draw_subscreen_ring(0, 0, 320, 480, 0xffff0000, 32, rot, 200);
			draw_subscreen_ring(320, 0, 320, 480, 0xff00ff00, 32, rot, 170);
			draw_subscreen_ring(0, 0, 320, 480, 0xffffff00, 32, rot, 140);
			draw_subscreen_ring(320, 0, 320, 480, 0xffffffff, 32, rot, 110);
		}
	}
}


int main() {
	pvr_setup();
	
	pvr_stats_t stats;
	float average_render_time = 10.0f;
	do {
		check_inputs();
		
		vid_border_color(0, 0, 0);
		pvr_wait_ready();
		vid_border_color(255, 0, 0);
		
		pvr_scene_begin();
			pvr_list_begin(PVR_LIST_TR_POLY);
				draw_transparent();
			pvr_list_finish();
		pvr_scene_finish();
		vid_border_color(0, 255, 0);
		
		pvr_get_stats(&stats);
		average_render_time = average_render_time * 0.9 + stats.rnd_last_time * 0.1;
		
		if (a_click) {
			printf("Average render time: %f\n", average_render_time);
			fflush(stdout);
		}
		if (start_pressed)
			break;
		
	} while(1);
	
	return 0;
}
