/*
 * OpenAL example
 *
 * Copyright(C) Florian Fainelli <f.fainelli@gmail.com>
 * Copyright (c) 2019 HaydenKow (Streaming technique)
 * Copyright (c) 2019 Luke Benstead
 * Copyright (c) 2020 Protofall
 */

#include "audio_assist.h"

#if defined(_arch_dreamcast)

extern uint8 romdisk[];
KOS_INIT_ROMDISK(romdisk);

#include <dc/pvr.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

pvr_ptr_t font_tex;

//Init font
void font_init(){
	int i, x, y, c;
	unsigned short * temp_tex;

	font_tex = pvr_mem_malloc(256 * 256 * 2);	//16bpp (ARGB4444 Mode), 256 by 256 texture.
												//We must allocate 2^n by 2^n space because of hardware limitations.

	temp_tex = (unsigned short *)malloc(256 * 128 * 2);	//We can do *any* size for temp stuff (Draws into top half of texture)

	//Load the file into memory (Well, we have a baked-in romdisk...but still fine anyways)
	uint16_t header_size = 265;
	uint16_t file_size = 4361;
	FILE * texture_file = fopen("rd/fixed-fiction.pbm", "rb");
	fseek(texture_file, header_size, SEEK_SET);	//Move file pointer forwards 0x109 or 265 bytes
	char * wfont = (char *)malloc(file_size - header_size);
	fread(wfont, (file_size - header_size), 1, texture_file);
	fclose(texture_file);

	c = 0;
	for(y = 0; y < 128 ; y += 16){
		for(x = 0; x < 256 ; x += 8){
			for(i = 0; i < 16; i++){
				temp_tex[x + (y + i) * 256 + 0] = 0xffff * ((wfont[c + i] & 0x80) >> 7);	//0xffff is white pixel
				temp_tex[x + (y + i) * 256 + 1] = 0xffff * ((wfont[c + i] & 0x40) >> 6);	//0x0000 is black pixel in texture
				temp_tex[x + (y + i) * 256 + 2] = 0xffff * ((wfont[c + i] & 0x20) >> 5);	//1 bit in wfont.bin = 1 pixel
				temp_tex[x + (y + i) * 256 + 3] = 0xffff * ((wfont[c + i] & 0x10) >> 4);
				temp_tex[x + (y + i) * 256 + 4] = 0xffff * ((wfont[c + i] & 0x08) >> 3);
				temp_tex[x + (y + i) * 256 + 5] = 0xffff * ((wfont[c + i] & 0x04) >> 2);
				temp_tex[x + (y + i) * 256 + 6] = 0xffff * ((wfont[c + i] & 0x02) >> 1);
				temp_tex[x + (y + i) * 256 + 7] = 0xffff * (wfont[c + i] & 0x01);
			}

			c += 16;
		}
	}

	pvr_txr_load_ex(temp_tex, font_tex, 256, 256, PVR_TXRLOAD_16BPP);	//Texture load 16bpp

	free(wfont);
	free(temp_tex);
}

void draw_char(float x1, float y1, float z1, uint8_t a, uint8_t r, uint8_t g, uint8_t b, int c, float xs, float ys){
	pvr_vertex_t    vert;
	int             ix, iy;
	float           u1, v1, u2, v2;

	ix = (c % 32) * 8;	//C is the character to draw
	iy = (c / 32) * 16;
	u1 = (ix + 0.5f) * 1.0f / 256.0f;
	v1 = (iy + 0.5f) * 1.0f / 256.0f;
	u2 = (ix + 7.5f) * 1.0f / 256.0f;
	v2 = (iy + 15.5f) * 1.0f / 256.0f;

	vert.flags = PVR_CMD_VERTEX;
	vert.x = x1;
	vert.y = y1 + 16.0f * ys;
	vert.z = z1;
	vert.u = u1;
	vert.v = v2;
	vert.argb = (a << 24) + (r << 16) + (g << 8) + b;
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x1;
	vert.y = y1;
	vert.u = u1;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x1 + 8.0f * xs;
	vert.y = y1 + 16.0f * ys;
	vert.u = u2;
	vert.v = v2;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.x = x1 + 8.0f * xs;
	vert.y = y1;
	vert.u = u2;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));
}

//Draw a string
//x, y, "argb" params and str are self explanatory. xs and ys are kind like scaling/how large to make it
//Keep in mind this works best with xs/xy == 2, smaller spaces the lines too far apart and too much doesn't space them enough
void draw_string(float x, float y, float z, uint8_t a, uint8_t r, uint8_t g, uint8_t b, char *str, float xs, float ys){
	pvr_poly_cxt_t cxt;
	pvr_poly_hdr_t hdr;
	float orig_x = x;

	pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB4444, 256, 256, font_tex, PVR_FILTER_NONE);	//Draws characters in ARGB4444 mode
	pvr_poly_compile(&hdr, &cxt);
	pvr_prim(&hdr, sizeof(hdr));

	while(*str){
		if(*str == '\n'){
			x = orig_x;
			y += 40;
			str++;
			continue;
		}

		draw_char(x, y, z, a, r, g, b, *str++, xs, ys);
		x += 8 * xs;
	}
}

#endif








int main(int argc, char **argv){
	//Initialise OpenAL and the listener
	if(audio_init() != 0){return -1;}
	printf("HELLO2!\n");

	#ifdef _arch_dreamcast
	pvr_init_defaults();	//Init kos
	font_init();	//Initialise the font
	#endif

	ALCenum error;
	audio_info_t infoFX, infoMusic;
	audio_source_t sourceFX1, sourceFX2, sourceMusic;

	//Setup chopper sound effect
	#ifdef _arch_dreamcast
		if(audio_load_WAV_file_info("/rd/test.wav", &infoFX, AUDIO_NOT_STREAMING) == AL_FALSE){return -1;}
	#else
		if(audio_load_WAV_file_info("romdisk/test.wav", &infoFX, AUDIO_NOT_STREAMING) == AL_FALSE){return -1;}
	#endif
	if(audio_test_error(&error, "loading wav file") == AL_TRUE){return -1;}
	
	if(audio_create_source(&sourceFX1, &infoFX, (vec2_f_t){0,0}, AL_TRUE, 0.25, 1) == AL_FALSE){return -1;}
	if(audio_create_source(&sourceFX2, &infoFX, (vec2_f_t){0,0}, AL_TRUE, 0.25, 1) == AL_FALSE){return -1;}

	//Setup music
	#ifdef _arch_dreamcast
		if(audio_load_WAV_file_info("/rd/The-Haunted-House.wav", &infoMusic, AUDIO_STREAMING) == AL_FALSE){return -1;}
	#else
		if(audio_load_WAV_file_info("romdisk/The-Haunted-House.wav", &infoMusic, AUDIO_STREAMING) == AL_FALSE){return -1;}
	#endif
	if(audio_test_error(&error, "loading wav file") == AL_TRUE){return -1;}
	
	//Note last param is ignored for streaming
	if(audio_create_source(&sourceMusic, &infoMusic, (vec2_f_t){0,0}, AL_FALSE, 0.5, 1) == AL_FALSE){return -1;}

	//Play the sound effect and music
	// if(audio_play_source(&sourceFX1) == AL_FALSE){return -1;}
	// if(audio_play_source(&sourceMusic) == AL_FALSE){return -1;}

	//So the program continues forever
	char input[20];
	char text[200];
	#if defined(_arch_dreamcast)
	sprintf(text, "Commands.\nSourceMusic [UP]\nSourceFX1 [LEFT]\nSourceFX2 [RIGHT]\nPlay [A]\nStop [B]\nPause [X]\nUnpause [Y]\nExit [START]\n");
	#else
	sprintf(text, "Commands.\nSourceMusic [0]\nSourceFX1 [1]\nSourceFX2 [2]\nPlay [3]\nStop [4]\nPause [5]\nUnpause [6]\nExit [7]\n");
	#endif

	audio_source_t * target_source = &sourceMusic;
	int8_t target_cmd = -1;

	uint32_t previous_input[4] = {0};
	uint8_t play = 1;
	while(play){
		#if defined(_arch_dreamcast)
		MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)

			if((st->buttons & CONT_DPAD_UP) && !(previous_input[__dev->port] & CONT_DPAD_UP)){
				target_source = &sourceMusic;
			}
			else if((st->buttons & CONT_DPAD_LEFT) && !(previous_input[__dev->port] & CONT_DPAD_LEFT)){
				target_source = &sourceFX1;
			}
			else if((st->buttons & CONT_DPAD_RIGHT) && !(previous_input[__dev->port] & CONT_DPAD_RIGHT)){
				target_source = &sourceFX2;
			}
			
			if((st->buttons & CONT_A) && !(previous_input[__dev->port] & CONT_A)){
				audio_play_source(target_source);
			}

			else if((st->buttons & CONT_B) && !(previous_input[__dev->port] & CONT_B)){
				audio_stop_source(target_source);
			}

			else if((st->buttons & CONT_X) && !(previous_input[__dev->port] & CONT_X)){
				audio_pause_source(target_source);
			}

			else if((st->buttons & CONT_Y) && !(previous_input[__dev->port] & CONT_Y)){
				audio_unpause_source(target_source);
			}

			if((st->buttons & CONT_START) && !(previous_input[__dev->port] & CONT_START)){
				play = 0;
				break;
			}

		previous_input[__dev->port] = st->buttons;
		MAPLE_FOREACH_END()

		pvr_wait_ready();
		pvr_scene_begin();

		pvr_list_begin(PVR_LIST_TR_POLY);

			draw_string(30, 30, 1, 255, 255, 216, 0, text, 2, 2);	//Draws in yellow colour

		pvr_list_finish();


		pvr_scene_finish();
		#else
		
		printf("%s", text);
		scanf("%19s", input);
		uint8_t i = 0;
		while(input[i] != '\0'){
			switch(input[i]){
			case '0':
				target_source = &sourceMusic;
				break;
			case '1':
				target_source = &sourceFX1;
				break;
			case '2':
				target_source = &sourceFX2;
				break;
			case '3':
				target_cmd = 0;
				break;
			case '4':
				target_cmd = 1;
				break;
			case '5':
				target_cmd = 2;
				break;
			case '6':
				target_cmd = 3;
				break;
			case '7':
				play = 0;
				break;
			}
			i++;
		}

		if(target_cmd == 0){audio_play_source(target_source);}
		else if(target_cmd == 1){audio_stop_source(target_source);}
		else if(target_cmd == 2){audio_pause_source(target_source);}
		else if(target_cmd == 3){audio_unpause_source(target_source);}

		#endif

		target_cmd = -1;
	}

	//Stop the sources
	audio_stop_source(&sourceMusic);
	audio_stop_source(&sourceFX1);
	audio_stop_source(&sourceFX2);

	#ifdef _arch_dreamcast
	pvr_mem_free(font_tex);
	#endif

	//Now free their data
	audio_free_source(&sourceMusic);
	audio_free_info(&infoMusic);

	audio_free_source(&sourceFX1);
	audio_free_source(&sourceFX2);
	audio_free_info(&infoFX);

	audio_shutdown();

	return 0;
}

