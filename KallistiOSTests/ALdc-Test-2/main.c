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
	ALuint buffer, source;
	ALint source_state;
	ALCenum error = AL_FALSE;

	//Initialise OpenAL and the listener
	al_init();

	alGenSources((ALuint)1, &source);
	al_test_error(&error, "source generation");
	if(error == AL_TRUE){return -1;}

	alSourcef(source, AL_PITCH, 1);
	al_test_error(&error, "source pitch");
	if(error == AL_TRUE){return -1;}

	alSourcef(source, AL_GAIN, 1);
	al_test_error(&error, "source gain");
	if(error == AL_TRUE){return -1;}

	alSource3f(source, AL_POSITION, 0, 0, 0);
	al_test_error(&error, "source position");
	if(error == AL_TRUE){return -1;}

	alSource3f(source, AL_VELOCITY, 0, 0, 0);
	al_test_error(&error, "source velocity");
	if(error == AL_TRUE){return -1;}

	alSourcei(source, AL_LOOPING, AL_FALSE);
	al_test_error(&error, "source looping");
	if(error == AL_TRUE){return -1;}

	alGenBuffers(1, &buffer);
	al_test_error(&error, "buffer generation");
	if(error == AL_TRUE){return -1;}

	al_audio_data_t audio_data;

#ifdef _arch_dreamcast
    if(!LoadWAVFile("/rd/test.wav", &audio_data, AL_AS_TYPE_NON_STREAM)){
        return -1;
    }
#else
    if(!LoadWAVFile("test.wav", &audio_data, AL_AS_TYPE_NON_STREAM)){
        return -1;
    }
#endif

	al_test_error(&error, "loading wav file");
	if(error == AL_TRUE){return -1;}

	alBufferData(buffer, audio_data.format, audio_data.data, audio_data.size, audio_data.freq);
	al_test_error(&error, "buffer copy");
	if(error == AL_TRUE){return -1;}

	alSourcei(source, AL_BUFFER, buffer);
	al_test_error(&error, "buffer binding");
	if(error == AL_TRUE){return -1;}

    alSourcei(source, AL_LOOPING, 1);
	alSourcePlay(source);
	al_test_error(&error, "source playing");
	if(error == AL_TRUE){return -1;}

	alGetSourcei(source, AL_SOURCE_STATE, &source_state);
	al_test_error(&error, "source state get");
	if(error == AL_TRUE){return -1;}
	while (source_state == AL_PLAYING) {
		alGetSourcei(source, AL_SOURCE_STATE, &source_state);
		al_test_error(&error, "source state get");
		if(error == AL_TRUE){return -1;}
        thd_pass();
	}

	al_shutdown();
	if(audio_data->path != NULL){	//Only CDDA audio doesn't have a path
		free(audio_data->path);
	}

	return 0;
}

