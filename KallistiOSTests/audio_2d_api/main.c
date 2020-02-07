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
	if(audio_init() != 0){return -1;}

	audio_info_t infoFX, infoMusic;
	audio_source_t sourceFX, sourceMusic;

	//Setup chopper sound effect
	#ifdef _arch_dreamcast
		if(al_load_WAV_file_info("/rd/test.wav", &infoFX, AUDIO_NOT_STREAMING) == AL_FALSE){return -1;}
	#else
		if(al_load_WAV_file_info("test.wav", &infoFX, AUDIO_NOT_STREAMING) == AL_FALSE){return -1;}
	#endif
	if(al_test_error(&error, "loading wav file") == AL_TRUE){return -1;}
	
	if(al_create_source(&sourceFX, &infoFX, (vec2_f_t){0,0}, AL_TRUE, 0.25, 1, AUDIO_FREE_DATA) == AL_FALSE){return -1;}

	//Setup music
	// #ifdef _arch_dreamcast
	// 	if(al_load_WAV_file_info("/rd/file_1.wav", &infoMusic, AUDIO_STREAMING) == AL_FALSE){return -1;}
	// #else
	// 	if(al_load_WAV_file_info("file_1.wav", &infoMusic, AUDIO_STREAMING) == AL_FALSE){return -1;}
	// #endif
	#ifdef _arch_dreamcast
		if(al_load_WAV_file_info("/cd/best.wav", &infoMusic, AUDIO_STREAMING) == AL_FALSE){return -1;}
	#else
		if(al_load_WAV_file_info("best.wav", &infoMusic, AUDIO_STREAMING) == AL_FALSE){return -1;}
	#endif
	if(al_test_error(&error, "loading wav file") == AL_TRUE){return -1;}
	
	//Note last param is ignored for streaming
	if(al_create_source(&sourceMusic, &infoMusic, (vec2_f_t){0,0}, AL_FALSE, 0.5, 1, AUDIO_FREE_DATA) == AL_FALSE){return -1;}

	//Play the sound effect
	if(audio_play_source(&sourceFX) == AL_FALSE){return -1;}

	// Play the music (Later make this a seperate thread)
	if(al_prep_stream_buffers() == AL_FALSE){return -1;}
	al_stream_player();

	audio_stop_source(&sourceFX);
	audio_stop_source(&sourceMusic);

	al_free_audio_source(&sourceFX);
	al_unload_audio_info(&infoFX);

	audio_shutdown();

	return 0;
}

