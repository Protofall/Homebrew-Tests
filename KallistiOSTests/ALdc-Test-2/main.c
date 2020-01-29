/*
 * OpenAL example
 *
 * Copyright(C) Florian Fainelli <f.fainelli@gmail.com>
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

	al_audio_info_t info;
	al_audio_source_t source;

	#ifdef _arch_dreamcast
		if(al_load_WAV_file_info("/rd/test.wav", &info, AL_AS_TYPE_NON_STREAM) == AL_FALSE){return -1;}
	#else
		if(al_load_WAV_file_info("test.wav", &info, AL_AS_TYPE_NON_STREAM) == AL_FALSE){return -1;}
	#endif
	if(al_test_error(&error, "loading wav file") == AL_TRUE){return -1;}
	
	if(al_create_source(&source, &info, (vec2_f_t){0,0}, AL_FALSE, 1, 1, 1) == AL_FALSE){return -1;}

	alSourcei(source.source_id, AL_LOOPING, 1);
	alSourcePlay(source.source_id);	//If called on a source that is already playing, it will restart from the beginning	
	if(al_test_error(&error, "source playing") == AL_TRUE){return -1;}

	if(al_update_source_state(&source) == AL_FALSE){return -1;}

	while(source.source_state == AL_PLAYING){
		if(al_update_source_state(&source) == AL_FALSE){return -1;}
		thd_pass();
	}

	alDeleteSources(1, &source.source_id);	//1st param is number of buffers
	alDeleteBuffers(source.num_buffers, source.buffer_id);

	al_unload_audio_info(&info);

	al_shutdown();

	return 0;
}

