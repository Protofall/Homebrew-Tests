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
	ALCenum error;

	//Initialise OpenAL and the listener
	if(al_init() != 0){return -1;}

	//Setup the sources and buffers
	alGenSources((ALuint)1, &source);
	if(al_test_error(&error, "source generation") == AL_TRUE){return -1;}

	alSourcef(source, AL_PITCH, 1);	//Note: Last param is float
	if(al_test_error(&error, "source pitch") == AL_TRUE){return -1;}

	alSourcef(source, AL_GAIN, 1);
	if(al_test_error(&error, "source gain") == AL_TRUE){return -1;}

	/*
		Other alSourcef options:

		AL_MIN_GAIN
		AL_MAX_GAIN
		AL_MAX_DISTANCE
		AL_ROLLOFF_FACTOR
		AL_CONE_OUTER_GAIN
		AL_CONE_INNER_ANGLE
		AL_CONE_OUTER_ANGLE
		AL_REFERENCE_DISTANCE 
	*/

	alSource3f(source, AL_POSITION, 0, 0, 0);
	if(al_test_error(&error, "source position") == AL_TRUE){return -1;}

	alSource3f(source, AL_VELOCITY, 0, 0, 0);
	if(al_test_error(&error, "source velocity") == AL_TRUE){return -1;}

	alSourcei(source, AL_LOOPING, AL_FALSE);
	if(al_test_error(&error, "source looping") == AL_TRUE){return -1;}

	/*
		Other alSourcei options:

		AL_SOURCE_RELATIVE
		AL_CONE_INNER_ANGLE
		AL_CONE_OUTER_ANGLE
		AL_BUFFER
		AL_SOURCE_STATE
	*/

	alGenBuffers(1, &buffer);	//Generating 1 buffer. 2nd param is a pointer to an array
								//of ALuint values which will store the names of the new buffers
								//Seems "buffer" is just an ID and doesn't actually contain the data?
	if(al_test_error(&error, "buffer generation") == AL_TRUE){return -1;}

	al_audio_info_t audio_data;

#ifdef _arch_dreamcast
    if(!LoadWAVFile("/rd/test.wav", &audio_data, AL_AS_TYPE_NON_STREAM)){
        return -1;
    }
#else
    if(!LoadWAVFile("test.wav", &audio_data, AL_AS_TYPE_NON_STREAM)){
        return -1;
    }
#endif

	if(al_test_error(&error, "loading wav file") == AL_TRUE){return -1;}

	alBufferData(buffer, audio_data.format, audio_data.data, audio_data.size, audio_data.freq);	//Fill the buffer with PCM data
	if(al_test_error(&error, "buffer copy") == AL_TRUE){return -1;}
	free(audio_data.data);	//Its no longer required

	alSourcei(source, AL_BUFFER, buffer);
	if(al_test_error(&error, "buffer binding") == AL_TRUE){return -1;}

    alSourcei(source, AL_LOOPING, 1);
	alSourcePlay(source);	//If called on a source that is already playing, it will restart from the beginning	
	if(al_test_error(&error, "source playing") == AL_TRUE){return -1;}

	alGetSourcei(source, AL_SOURCE_STATE, &source_state);
	if(al_test_error(&error, "source state get") == AL_TRUE){return -1;}

	while (source_state == AL_PLAYING){
		alGetSourcei(source, AL_SOURCE_STATE, &source_state);
		if(al_test_error(&error, "source state get") == AL_TRUE){return -1;}
        thd_pass();
	}

	alDeleteSources(1, &source);	//1st param is number of buffers
	alDeleteBuffers(1, &buffer);

	// early_shutdown:	//Need to modify those failures to free up memory if they fail

	al_shutdown();
	if(audio_data.path != NULL){	//Only CDDA audio doesn't have a path
		free(audio_data.path);
	}

	return 0;
}

