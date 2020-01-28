#ifndef AUDIO_ASSIST_H
#define AUDIO_ASSIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdbool.h>

#include <AL/al.h>
#include <AL/alc.h>

typedef struct al_audio_data{
	uint8_t play_type;	//In RAM or streaming. We can only have one streaming at a time
	// FILE * fp;	//Usually NULL, but points to file if its streaming
	uint8_t is_cdda;
	char * path;	//NULL if CDDA

	ALvoid * data;
	ALsizei size, freq;
	ALenum format;
} al_audio_data_t;

typedef struct vec2_f{
	float x, y;
} vec2_f_t;

typedef struct al_audio_sound{
	al_audio_data_t * sound;
	vec2_f_t position;	//Since we are doing 2D we only need two
						//velocity is always zero

	uint8_t loop;
	uint8_t volume;
	float speed;

	ALuint source_data;	//The source it uses
	ALint source_state;
} al_audio_sound_t;

ALCcontext * al_context;	//We only need one for all audio
ALCdevice * al_device;

FILE * al_streamer_fp;	//If NULL then we don't have a streamer struct, else something is streaming
al_audio_data_t * al_streamer;	//Is null if none are streaming, otherwise points to the only streaming struct


#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

uint8_t al_init();	//Returns 1 if an error occured, 0 otherwise

void al_shutdown();

ALboolean al_test_error(ALCenum * error, char * msg);

void list_audio_devices(const ALCchar *devices);

inline ALenum to_al_format(short channels, short samples);	//Unused

bool is_big_endian();

int convert_to_int(char * buffer, int len);

// void WAVE_buffer(void *data);

#define AL_AS_TYPE_NON_STREAM 0
#define AL_AS_TYPE_STREAM 1
#define AL_AS_TYPE_CDDA 2
ALboolean LoadWAVFile(const char * filename, al_audio_data_t * audio_data, uint8_t mode);

#endif