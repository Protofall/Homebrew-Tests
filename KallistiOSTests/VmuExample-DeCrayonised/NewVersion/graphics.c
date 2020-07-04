#include "graphics.h"

#ifdef _arch_dreamcast
void font_init(){
	#if CRAYON_BOOT_MODE == 1
		crayon_memory_mount_romdisk("/sd/sf_icon.img", "/Save");
	#else
		crayon_memory_mount_romdisk("/cd/sf_icon.img", "/Save");
	#endif

	int i, x, y, c;
	unsigned short * temp_tex;

	uint8_t error = 0;
	font_tex = pvr_mem_malloc(256 * 256 * 2);	//16bpp (ARGB4444 Mode), 256 by 256 texture.
												//We must allocate 2^n by 2^n space because of hardware limitations.

	temp_tex = (unsigned short *)malloc(256 * 128 * 2);	//We can do *any* size for temp stuff (Draws into top half of texture)

	//Load the file into memory (Well, we have a baked-in romdisk...but still fine anyways)
	uint16_t header_size = 265;
	uint16_t file_size = 4361;
	FILE * texture_file = fopen("Save/fixed-fiction.pbm", "rb");
	if(!texture_file){error |= (1 << 0);}
	fseek(texture_file, header_size, SEEK_SET);	//Move file pointer forwards 0x109 or 265 bytes
	char * wfont = (char *)malloc(file_size - header_size);
	if(!wfont){error |= (1 << 1);}
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

	fs_romdisk_unmount("/Save");
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
#endif

void draw_string(float x, float y, float z, uint8_t a, uint8_t r, uint8_t g, uint8_t b, char *str, float xs, float ys){
	#ifndef _arch_dreamcast
	printf("%s\n", str);
	#endif

	#ifdef _arch_dreamcast
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
	#endif
}