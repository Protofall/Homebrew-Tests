/*
	Controls:
	Hold X - Complex clip pattern
	Hold Y - Random square demo
	Press A - Print GPU render time to console
*/

#include <kos.h>
#include <stdlib.h>
#include <math.h>
#include <png/png.h>

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
pvr_poly_mod_hdr_t hdrout, hdrin;
pvr_mod_hdr_t hdrmod;


void pvr_setup() {
	pvr_init_params_t pvr_params = {
	    { PVR_BINSIZE_8, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_16, PVR_BINSIZE_0 },
	    512 * 1024
	};
	
	//Init hardware
	pvr_init(&pvr_params);
	pvr_set_bg_color(0, 0, .3);
	
	//Make sure full modifiers are on/cheap shadows off
	//I use an older, heavily modified version of KOS and don't feel like
	//updating and losing my changes.
	PVR_SET(PVR_CHEAP_SHADOW, 0x080);
	//It looks like this the propper way to do this in modern KOS:
	//	pvr_set_shadow_scale(0, 0);
	
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
	pvr_poly_cxt_txr_mod(&cxt, PVR_LIST_TR_POLY,
		PVR_TXRFMT_ARGB4444, w, h, pvrptr, PVR_FILTER_BILINEAR,
		PVR_TXRFMT_ARGB4444, w, h, pvrptr, PVR_FILTER_BILINEAR);
	
	cxt.gen.shading = PVR_SHADE_FLAT;
	
	//Outside volume parameters draw normally (dst=src*alpha+dst*(1-alpha))
	//Inside volume parameters have blend mode set to no op (dst=Src*0+dst*1)
	cxt.blend.src = PVR_BLEND_SRCALPHA;
	cxt.blend.dst = PVR_BLEND_INVSRCALPHA;
	cxt.blend.src2 = PVR_BLEND_ZERO;
	cxt.blend.dst2 = PVR_BLEND_ONE;
	pvr_poly_mod_compile(&hdrout, &cxt);
	
	//Outside volume parameters have blend mode set to no op  (dst=Src*0+dst*1)
	//Inside volume parameters draw normally (dst=src*alpha+dst*(1-alpha))
	cxt.blend.src = PVR_BLEND_ZERO;
	cxt.blend.dst = PVR_BLEND_ONE;
	cxt.blend.src2 = PVR_BLEND_SRCALPHA;
	cxt.blend.dst2 = PVR_BLEND_INVSRCALPHA;
	pvr_poly_mod_compile(&hdrin, &cxt);
	
	//For 2D work, it's fine to have all modifier polygons set to "last poly"
	//No need for culling in 2D either
	pvr_mod_compile(&hdrmod, PVR_LIST_TR_MOD, PVR_MODIFIER_INCLUDE_LAST_POLY, PVR_CULLING_NONE);
}

inline void draw_sprite(float x, float y, float z, float h, float w, unsigned int argb)
{
	//Enough vertices to send an entire rect in one pvr_prim call
	pvr_vertex_tpcm_t v[4];
	
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
	//v[0].argb0 = v[1].argb0 = argb;
	//v[0].argb1 = v[1].argb1 = argb;
	
	//To make this function work for drawing both inside and outside the volume,
	//we set both the inside and outside color/UVs, even though only one will be used
	v[2].argb0 = v[3].argb0 = v[2].argb1 = v[3].argb1 = argb;
	
	v[0].u0 = v[0].u1 = 0;
	v[0].v0 = v[0].v1 = 1;
	v[1].u0 = v[1].u1 = 0;
	v[1].v0 = v[1].v1 = 0;
	v[2].u0 = v[2].u1 = 1;
	v[2].v0 = v[2].v1 = 1;
	v[3].u0 = v[3].u1 = 1;
	v[3].v0 = v[3].v1 = 0;
	
	pvr_prim(&v, sizeof(v));
}

//DEMO DRAWING
#define PARTICLE_SIZE	16

//Draw a rotating ring of particles
void draw_subscreen_ring(uint32_t color, int particlecount, float angle, float radius) {
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

//Draw particles at random places
void draw_subscreen_rand(float x, float y, float w, float h, uint32_t color, int particlecount) {
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
	//Rotation for ring demo
	static float rot = 0;
	rot += 0.01;
	if (rot > 2*M_PI)
		rot -= 2*M_PI;

	if (y_pressed) {
		//Set PVR to draw only outside the modifer volume
		pvr_prim(&hdrout, sizeof(hdrout));
		//Draw screen upper half
		draw_subscreen_rand(0, 0, 640, 240, 0x40ff0000, 1400);
		
		//Set PVR to draw only inside  the modifer volume if clipping enabled
		if (!b_pressed)
			pvr_prim(&hdrin, sizeof(hdrin));
		//Draw screen lower half
		draw_subscreen_rand(0, 240, 640, 240, 0x4000ff00, 1400);
	} else {
		//Set PVR to draw only outside the modifer volume
		pvr_prim(&hdrout, sizeof(hdrout));
		//Draw two rings upper half
		draw_subscreen_ring(0xffff0000, 32, rot, 200);
		draw_subscreen_ring(0xffffff00, 32, rot, 140);
		
		//Set PVR to draw only inside  the modifer volume if clipping enabled
		if (!b_pressed)
			pvr_prim(&hdrin, sizeof(hdrin));
		//Draw two rings lower half
		draw_subscreen_ring(0xff00ff00, 32, rot, 170);
		draw_subscreen_ring(0xffffffff, 32, rot, 110);
	}
}


void draw_modifier() {
	pvr_modifier_vol_t vol[2];
	
	pvr_prim(&hdrmod, sizeof(hdrmod));
	
	vol[0].flags = vol[1].flags = PVR_CMD_VERTEX_EOL;
	vol[0].az = vol[0].bz = vol[0].cz = vol[1].az = vol[1].bz = vol[1].cz = 1.0f;
	
	/*
		These send quads/a quad with volume triangles
		
		First triangle stored in vol[0]:
		a---c
		|  /
		v ^
		|/
		b
		
		Second triangle stored in vol[1]:
		    b
		   /|
		  ^ v
		 /  |
		a---c
	*/
	
	if (!x_pressed) {
		//Simple clip pattern (quad)
		vol[0].ax = 0.0f;
		vol[0].ay = 240.0f;
		vol[0].bx = 0.0f;
		vol[0].by = 480.0f;
		vol[0].cx = 640.0f;
		vol[0].cy = 240.0f;
		
		vol[1].ax = 0.0f;
		vol[1].ay = 480.0f;
		vol[1].bx = 640.0f;
		vol[1].by = 240.0f;
		vol[1].cx = 640.0f;
		vol[1].cy = 480.0f;
		
		pvr_prim(&vol, sizeof(vol));
	} else {
		//Complex clip pattern (waves)
		int i;
		static float anim = 0;
		float lasty = 0;
		anim += 0.1;
		
		int res = 5;	//Resolution of complex clip mode
		int steps = 640 / res + 1;
		for (i = -1; i < steps; i++) {
			float l = i*res;	//left edge of quad
			float r = (i+1)*res;	//right edge of quad
			
			float x = (float)i / steps * 2 * M_PI;
			float y = fsin(x*14 + -anim)*4 + fsin(x*3+anim*1.2)*7 + 240;
			
			vol[0].ax = l;
			vol[0].ay = lasty;
			vol[0].bx = l;
			vol[0].by = 480.0f;
			vol[0].cx = r;
			vol[0].cy = y;
			
			vol[1].ax = l;
			vol[1].ay = 480.0f;
			vol[1].bx = r;
			vol[1].by = y;
			vol[1].cx = r;
			vol[1].cy = 480.0f;
			
			lasty = y;
			
			pvr_prim(&vol, sizeof(vol));
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
			
			//Don't draw the modifier volumes if clipping is disabled
			if (!b_pressed) {
				pvr_list_begin(PVR_LIST_TR_MOD);
					draw_modifier();
				pvr_list_finish();
			}
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
