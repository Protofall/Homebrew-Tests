/*
 * OpenAL example
 *
 * Copyright(C) Florian Fainelli <f.fainelli@gmail.com>
 * Copyright (c) 2019 HaydenKow (Streaming technique)
 * Copyright (c) 2019 Luke Benstead
 * Copyright (c) 2020 Protofall
 */

#include "audio_assist.h"

#ifdef _arch_dreamcast
extern uint8 romdisk[];
KOS_INIT_ROMDISK(romdisk);
#endif

int main(int argc, char **argv){
	ALCenum error;

	//Initialise OpenAL and the listener
	if(al_init() != 0){return -1;}

	al_audio_info_t infoFX, infoMusic;
	al_audio_source_t sourceFX, sourceMusic;

	//Setup chopper sound effect
	#ifdef _arch_dreamcast
		if(al_load_WAV_file_info("/rd/test.wav", &infoFX, AL_AS_TYPE_NON_STREAM) == AL_FALSE){return -1;}
	#else
		if(al_load_WAV_file_info("test.wav", &infoFX, AL_AS_TYPE_NON_STREAM) == AL_FALSE){return -1;}
	#endif
	if(al_test_error(&error, "loading wav file") == AL_TRUE){return -1;}
	
	if(al_create_source(&sourceFX, &infoFX, (vec2_f_t){0,0}, AL_TRUE, 1, 1, 0) == AL_FALSE){return -1;}

	//Setup music
	#ifdef _arch_dreamcast
		if(al_load_WAV_file_info("/rd/file_1.wav", &infoMusic, AL_AS_TYPE_STREAM) == AL_FALSE){return -1;}
	#else
		if(al_load_WAV_file_info("file_1.wav", &infoMusic, AL_AS_TYPE_STREAM) == AL_FALSE){return -1;}
	#endif
	if(al_test_error(&error, "loading wav file") == AL_TRUE){return -1;}
	
	if(al_create_source(&sourceMusic, &infoMusic, (vec2_f_t){0,0}, AL_TRUE, 1, 1, 1) == AL_FALSE){return -1;}

	//Play the sound effect
	// if(al_play_source(&sourceFX) == AL_FALSE){return -1;}

	// if(al_prep_stream_buffers() == AL_FALSE){return -1;}

	// if(al_update_source_state(&sourceFX) == AL_FALSE){return -1;}

	// while(sourceFX.source_state == AL_PLAYING){
	// 	if(al_update_source_state(&sourceFX) == AL_FALSE){return -1;}
	// 	thd_pass();
	// }

	// Play the music (Later make this a seperate thread)
	if(al_prep_stream_buffers() == AL_FALSE){return -1;}
	uint8_t res = al_stream_player();
	if(res == 0){while(1){;}}

	if(al_play_source(&sourceFX) == AL_FALSE){return -1;}

	if(al_update_source_state(&sourceFX) == AL_FALSE){return -1;}

	while(sourceFX.source_state == AL_PLAYING){
		if(al_update_source_state(&sourceFX) == AL_FALSE){return -1;}
		thd_pass();
	}

	al_stop_source(&sourceFX);
	al_stop_source(&sourceMusic);

	al_free_audio_source(&sourceFX);
	al_unload_audio_info(&infoFX);

	al_shutdown();

	return 0;
}

