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
		if(audio_load_WAV_file_info("/rd/test.wav", &infoFX, AUDIO_NOT_STREAMING) == AL_FALSE){return -1;}
	#else
		if(audio_load_WAV_file_info("romdisk/test.wav", &infoFX, AUDIO_NOT_STREAMING) == AL_FALSE){return -1;}
	#endif
	if(audio_test_error(&error, "loading wav file") == AL_TRUE){return -1;}
	
	if(audio_create_source(&sourceFX, &infoFX, (vec2_f_t){0,0}, AL_TRUE, 0.25, 1, AUDIO_FREE_DATA) == AL_FALSE){return -1;}

	//Setup music
	#ifdef _arch_dreamcast
		if(audio_load_WAV_file_info("/rd/The-Haunted-House.wav", &infoMusic, AUDIO_STREAMING) == AL_FALSE){return -1;}
	#else
		if(audio_load_WAV_file_info("romdisk/The-Haunted-House.wav", &infoMusic, AUDIO_STREAMING) == AL_FALSE){return -1;}
	#endif
	if(audio_test_error(&error, "loading wav file") == AL_TRUE){return -1;}
	
	//Note last param is ignored for streaming
	if(audio_create_source(&sourceMusic, &infoMusic, (vec2_f_t){0,0}, AL_FALSE, 0.5, 1, AUDIO_FREE_DATA) == AL_FALSE){return -1;}

	//Play the sound effect and music
	if(audio_play_source(&sourceFX) == AL_FALSE){return -1;}
	if(audio_play_source(&sourceMusic) == AL_FALSE){return -1;}

	//So the program continues forever
	while(1){
		;
	}

	audio_stop_source(&sourceFX);
	audio_stop_source(&sourceMusic);

	audio_free_source(&sourceMusic);
	audio_unload_info(&infoMusic);

	audio_free_source(&sourceFX);
	audio_unload_info(&infoFX);

	audio_shutdown();

	return 0;
}

