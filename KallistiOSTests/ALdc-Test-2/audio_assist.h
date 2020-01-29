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

#define AL_AS_TYPE_NON_STREAM (0 << 0)
#define AL_AS_TYPE_STREAM (1 << 0)

typedef struct al_audio_info{
	uint8_t play_type;	//In RAM or streaming. We can only have one streaming at a time
	uint8_t is_cdda;
	char * path;	//NULL if CDDA

	ALvoid * data;
	ALsizei size, freq;
	ALenum format;
} al_audio_info_t;

typedef struct vec2_f{
	float x, y;
} vec2_f_t;

typedef struct al_audio_source{
	al_audio_info_t * info;
	vec2_f_t position;	//Since we are doing 2D we only need two
						//velocity is always zero

	ALboolean looping;
	float volume;	//Gain
	float speed;	//Pitch

	uint8_t num_buffers;
	ALuint * buffer_id;	//Each source can use 1 or more buffers
	ALuint source_id;	//The source it uses
	ALint source_state;
} al_audio_source_t;

ALCcontext * al_context;	//We only need one for all audio
ALCdevice * al_device;

//Since it only makes sense to stream one audio source (The music). I'be hard coded it to only use one
FILE * al_streamer_fp;	//If a pointer to the file/data on disc
al_audio_info_t * al_streamer_info;	//Is null if none are streaming, otherwise points to the streaming struct
// al_audio_source_t * al_streamer_sound;

// void WAVE_buffer(void *data);

#define AL_AS_STORE_NO_DATA (0 << 1)
#define AL_AS_STORE_DATA (1 << 1)
#define AL_AS_FREE_DATA (1 << 1)

uint8_t al_init();	//Returns 1 if an error occured, 0 otherwise
void al_shutdown();

//NOTE: I want an option to load a CDDA song into RAM instead of streaming if thats what the user wants

//These load functions will instanly fail if you want to stream and there's another stream one present
	//Mode = ---- --DS
	//D = storing data, S = streaming
ALboolean al_load_WAV_file_info(const char * path, al_audio_info_t * info, uint8_t mode);	//Mode is stream/local and storing data
ALboolean al_load_CDDA_track_info(uint8_t track, al_audio_info_t * info, uint8_t mode);	//Data is never stored if in stream mode

void al_unload_audio_info(al_audio_info_t * info);	//This will free path and data if needed

ALboolean al_create_source(al_audio_source_t * source, al_audio_info_t * info, vec2_f_t position, ALboolean looping,
	float volume, float speed, uint8_t num_buffers);

ALboolean al_update_source_state(al_audio_source_t * source);

//----------------------ADJUSTMENT---------------------//

uint8_t al_adjust_master_volume(float vol);	//adjust's listener's gain
uint8_t al_adjust_source_volume(al_audio_source_t * source, float vol);	//adjust's source's gain
uint8_t al_adjust_source_speed(al_audio_source_t * source, float speed);


//----------------------MISC---------------------------//

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

ALboolean al_test_error(ALCenum * error, char * msg);

void list_audio_devices(const ALCchar *devices);

inline ALenum to_al_format(short channels, short samples);	//Unused

bool is_big_endian();

int convert_to_int(char * buffer, int len);


#endif