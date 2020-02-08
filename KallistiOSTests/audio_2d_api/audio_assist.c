#include "audio_assist.h"

uint8_t audio_init(){
	audio_streamer_command = AUDIO_COMMAND_NONE;
	audio_streamer_source = NULL;
	audio_streamer_fp = NULL;

	ALboolean enumeration;
	ALfloat listenerOri[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };	//Double check what these vars mean
	ALCenum error;

	enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
	if(enumeration == AL_FALSE){
		fprintf(stderr, "enumeration extension not available\n");
	}

	al_list_audio_devices(alcGetString(NULL, ALC_DEVICE_SPECIFIER));

	// const ALCchar *defaultDeviceName;
	// if(!defaultDeviceName){
	// 	defaultDeviceName = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
	// }
	// al_device = alcOpenDevice(defaultDeviceName);
	al_device = alcOpenDevice(NULL);	//Chooses the preferred/default device
	if(!al_device){
		fprintf(stderr, "unable to open default device\n");
		return 1;
	}

    // fprintf(stdout, "Device: %s\n", alcGetString(device, ALC_DEVICE_SPECIFIER));

	alGetError();	//This resets the error state

	al_context = alcCreateContext(al_device, NULL);
	if(!alcMakeContextCurrent(al_context)){
		fprintf(stderr, "failed to make default context\n");
		return 1;
	}
	if(audio_test_error(&error, "make default context") == AL_TRUE){return 1;}

	// set orientation
	alListener3f(AL_POSITION, 0, 0, 1.0f);
	if(audio_test_error(&error, "listener position") == AL_TRUE){return 1;}

    alListener3f(AL_VELOCITY, 0, 0, 0);
	if(audio_test_error(&error, "listener velocity") == AL_TRUE){return 1;}

	alListenerfv(AL_ORIENTATION, listenerOri);
	if(audio_test_error(&error, "listener orientation") == AL_TRUE){return 1;}

	//Init the mutex for the streamer thread
	if(pthread_mutex_init(&audio_streamer_lock, NULL) != 0){
		printf("Mutex init failed\n");
		return 1;
	}

	audio_streamer_command = AUDIO_COMMAND_NONE;
	audio_streamer_thd_active = 0;

	return 0;
}

void audio_shutdown(){
	// al_context = alcGetCurrentContext();	//With only one device/context, this line might not be required
	// al_device = alcGetContextsDevice(al_context);
	alcMakeContextCurrent(NULL);
	alcDestroyContext(al_context);
	alcCloseDevice(al_device);

	pthread_mutex_destroy(&audio_streamer_lock);

	return;
}

ALboolean audio_load_WAV_file_info(const char * filename, audio_info_t * info, uint8_t mode){
	if((mode & AUDIO_STREAMING) && (audio_streamer_source != NULL || audio_streamer_fp != NULL)){
		return AL_FALSE;
	}

	info->streaming = !!(mode & AUDIO_STREAMING);
	switch(info->streaming){
		case AUDIO_NOT_STREAMING:
			info->data_type = AUDIO_DATA_TYPE_WAV;
		break;
		case AUDIO_STREAMING:
			info->data_type = AUDIO_DATA_TYPE_WAV;
		break;
		default:
			fprintf(stderr, "Invalid wav load mode given\n");
			return AL_FALSE;
		break;
	}

	char buffer[4];

	FILE* in = fopen(filename, "rb");
	if(!in){
		fprintf(stderr, "Couldn't open file\n");
		return AL_FALSE;
	}

	fread(buffer, 4, sizeof(char), in);

	if(strncmp(buffer, "RIFF", 4) != 0){
		fprintf(stderr, "Not a valid wave file\n");
		return AL_FALSE;
	}

	fread(buffer, 4, sizeof(char), in);
	fread(buffer, 4, sizeof(char), in);		//WAVE
	fread(buffer, 4, sizeof(char), in);		//fmt
	fread(buffer, 4, sizeof(char), in);		//16
	fread(buffer, 2, sizeof(char), in);		//1
	fread(buffer, 2, sizeof(char), in);

	int chan = convert_to_int(buffer, 2);
	fread(buffer, 4, sizeof(char), in);
	info->freq = convert_to_int(buffer, 4);
	fread(buffer, 4, sizeof(char), in);
	fread(buffer, 2, sizeof(char), in);
	fread(buffer, 2, sizeof(char), in);
	int bps = convert_to_int(buffer, 2);
	fread(buffer, 4, sizeof(char), in);		//data
	fread(buffer, 4, sizeof(char), in);
	info->size = (ALsizei) convert_to_int(buffer, 4);	//This isn't the true size

	if(info->streaming != AUDIO_STREAMING){
		info->data = (ALvoid*) malloc(info->size * sizeof(char));
		fread(info->data, info->size, sizeof(char), in);
	}
	else{
		info->data = NULL;
	}

	if(chan == 1){
		info->format = (bps == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
	}
	else{
		info->format = (bps == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
	}

	audio_streamer_info = info;
	if(info->streaming != AUDIO_STREAMING){
		fclose(in);
	}
	else{
		audio_streamer_fp = in;
	}

	printf("Loaded WAV file!\n");

	return AL_TRUE;
}

//Currently unimplemented
ALboolean audio_load_CDDA_track_info(uint8_t track, audio_info_t * info, uint8_t mode){
	if(mode & AUDIO_STREAMING && (audio_streamer_source != NULL || audio_streamer_fp != NULL)){
		return AL_FALSE;
	}

	return AL_FALSE;
}

ALboolean audio_unload_info(audio_info_t * info){
	if(info == NULL){
		return AL_FALSE;
	}

	audio_free_info_data(info);	//Note: If there's no data this doesn nothing

	if(info->streaming == AUDIO_STREAMING){
		fclose(audio_streamer_fp);
		audio_streamer_fp = NULL;
		audio_streamer_source->info = NULL;
		audio_free_source(audio_streamer_source);
		audio_streamer_info = NULL;
	}

	return AL_TRUE;
}

void audio_free_info_data(audio_info_t * info){
	if(info != NULL && info->data){
		free(info->data);
		info->data = NULL;
	}
	return;
}

ALboolean audio_free_source(audio_source_t * source){
	if(source == NULL){
		return AL_FALSE;
	}

	uint8_t num_buffers = (source->info->streaming == AUDIO_STREAMING) ? AUDIO_STREAMING_NUM_BUFFERS : 1;

	alDeleteSources(1, &source->source_id);
	alDeleteBuffers(num_buffers, source->buffer_id);	//1st param is number of buffers

	//So we can later know there isn't a streamer presents
	if(source == audio_streamer_source){
		pthread_mutex_lock(&audio_streamer_lock);
		if(audio_streamer_thd_active == 1){
			audio_streamer_command = AUDIO_COMMAND_END;	//Doesn't matter what we said before, we want it to terminate now
		}
		pthread_mutex_unlock(&audio_streamer_lock);

		//NOTE: I could add code to wait for the thread to end, but I think its fine to assume it will end on its own

		audio_streamer_source = NULL;
	}

	return AL_TRUE;
}

ALboolean audio_create_source(audio_source_t * source, audio_info_t * info, vec2_f_t position, ALboolean looping,
	float volume, float speed, uint8_t delete_data){

	if(source == NULL || info == NULL || volume < 0 || speed < 0){
		return AL_FALSE;
	}

	if(info->streaming == AUDIO_STREAMING){
		if(audio_streamer_source != NULL){return AL_FALSE;}

		pthread_mutex_lock(&audio_streamer_lock);
		if(audio_streamer_thd_active == 1){	//There's already a streamer thread
			pthread_mutex_unlock(&audio_streamer_lock);
			return AL_FALSE;
		}
		pthread_mutex_unlock(&audio_streamer_lock);
		
		audio_streamer_source = source;
	}

	source->info = info;
	source->position = position;
	source->looping = looping;
	source->volume = volume;
	source->speed = speed;

	ALCenum error;

	alGenSources((ALuint)1, &source->source_id);	//Generate one source
	if(audio_test_error(&error, "source generation") == AL_TRUE){return AL_FALSE;}

	alSourcef(source->source_id, AL_PITCH, speed);
	if(audio_test_error(&error, "source pitch") == AL_TRUE){return AL_FALSE;}

	alSourcef(source->source_id, AL_GAIN, volume);
	if(audio_test_error(&error, "source gain") == AL_TRUE){return AL_FALSE;}

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

	alSource3f(source->source_id, AL_POSITION, position.x, position.y, 0);	//Since we're 2D, Z is always zero
	if(audio_test_error(&error, "source position") == AL_TRUE){return AL_FALSE;}

	alSource3f(source->source_id, AL_VELOCITY, 0, 0, 0);
	if(audio_test_error(&error, "source velocity") == AL_TRUE){return AL_FALSE;}

	alSourcei(source->source_id, AL_LOOPING, looping);
	if(audio_test_error(&error, "source looping") == AL_TRUE){return AL_FALSE;}

	/*
		Other alSourcei options:

		AL_SOURCE_RELATIVE
		AL_CONE_INNER_ANGLE
		AL_CONE_OUTER_ANGLE
		AL_BUFFER
		AL_SOURCE_STATE
	*/

	//1 buffer normally, but "AUDIO_STREAMING_NUM_BUFFERS" for streaming
	uint8_t num_buffers = (info->streaming == AUDIO_STREAMING) ? AUDIO_STREAMING_NUM_BUFFERS : 1;

	//Generate the buffers
	source->buffer_id = malloc(sizeof(ALuint) * num_buffers);
	alGenBuffers(num_buffers, source->buffer_id);	//Generating "num_buffers" buffer. 2nd param is a pointer to an array
													//of ALuint values which will store the names of the new buffers
													//Seems "buffer" is just an ID and doesn't actually contain the data?
	if(audio_test_error(&error, "buffer generation") == AL_TRUE){return AL_FALSE;}

	if(info->streaming == AUDIO_NOT_STREAMING){
		alBufferData(source->buffer_id[0], info->format, info->data, info->size, info->freq);	//Fill the buffer with PCM data
		if(audio_test_error(&error, "buffer copy") == AL_TRUE){return AL_FALSE;}

		alSourcei(source->source_id, AL_BUFFER, source->buffer_id[0]);
		if(audio_test_error(&error, "buffer binding") == AL_TRUE){return AL_FALSE;}

		if(delete_data){
			audio_free_info_data(info);
		}
	}
	else{	//We start the streamer thread
		if(pthread_create(&audio_streamer_thd_id, NULL, audio_stream_player, NULL)){
			printf("Failed to create streamer thread\n");
			return AL_FALSE;
		}
	}

	return AL_TRUE;
}

ALboolean audio_update_source_state(audio_source_t * source){
	if(source == NULL){
		return AL_FALSE;
	}

	ALCenum error;
	alGetSourcei(source->source_id, AL_SOURCE_STATE, &source->source_state);
	if(audio_test_error(&error, "source state get") == AL_TRUE){return AL_FALSE;}

	return AL_TRUE;
}

ALboolean audio_play_source(audio_source_t * source){
	if(source == NULL){return AL_FALSE;}

	ALCenum error;
	ALboolean ret_val;
	if(source == audio_streamer_source){
		pthread_mutex_lock(&audio_streamer_lock);
		if(audio_streamer_thd_active == 1){
			if(audio_streamer_command == AUDIO_COMMAND_NONE){
				audio_streamer_command = AUDIO_COMMAND_PLAY;
				ret_val = AL_TRUE;
			}
			else{ret_val = AL_FALSE;}
		}
		else{ret_val = AL_FALSE;}	//This should never occur
		pthread_mutex_unlock(&audio_streamer_lock);
		return ret_val;
	}
	else{
		alSourcePlay(source->source_id);	//If called on a source that is already playing, it will restart from the beginning	
		if(audio_test_error(&error, "source playing") == AL_TRUE){return AL_FALSE;}
	}
	return AL_TRUE;
}

ALboolean audio_pause_source(audio_source_t * source){
	if(source == NULL){return AL_FALSE;}

	ALCenum error;
	ALboolean ret_val;
	if(source == audio_streamer_source){
		pthread_mutex_lock(&audio_streamer_lock);
		if(audio_streamer_thd_active == 1 && audio_streamer_command == AUDIO_COMMAND_NONE){
			ret_val = AL_TRUE;
		}
		else{ret_val = AL_FALSE;}

		//Should checking the source state be out of the mutex lock?
		audio_update_source_state(source);
		if(source->source_state != AL_PLAYING){
			ret_val = AL_FALSE;
		}

		if(ret_val == AL_TRUE){
			audio_streamer_command = AUDIO_COMMAND_PAUSE;
		}

		pthread_mutex_unlock(&audio_streamer_lock);
		return ret_val;
	}
	alSourcePause(source->source_id);
	if(audio_test_error(&error, "source pausing") == AL_TRUE){return AL_FALSE;}
	return AL_TRUE;
}

ALboolean audio_unpause_source(audio_source_t * source){
	if(source == NULL){return AL_FALSE;}

	ALCenum error;
	ALboolean ret_val;
	if(source == audio_streamer_source){
		pthread_mutex_lock(&audio_streamer_lock);
		if(audio_streamer_thd_active == 1 && audio_streamer_command == AUDIO_COMMAND_NONE){
			ret_val = AL_TRUE;
		}
		else{ret_val = AL_FALSE;}

		//Should checking the source state be out of the mutex lock?
		audio_update_source_state(source);
		if(source->source_state != AL_PAUSED){
			ret_val = AL_FALSE;
		}

		if(ret_val == AL_TRUE){
			audio_streamer_command = AUDIO_COMMAND_UNPAUSE;
		}

		pthread_mutex_unlock(&audio_streamer_lock);
		return ret_val;
	}

	alSourcePlay(source->source_id);
	if(audio_test_error(&error, "source playing") == AL_TRUE){return AL_FALSE;}
	return AL_TRUE;
}

ALboolean audio_stop_source(audio_source_t * source){
	if(source == NULL){return AL_FALSE;}

	ALCenum error;
	ALboolean ret_val;
	if(source == audio_streamer_source){
		pthread_mutex_lock(&audio_streamer_lock);
		if(audio_streamer_thd_active == 1){
			if(audio_streamer_command == AUDIO_COMMAND_NONE){
				audio_streamer_command = AUDIO_COMMAND_STOP;
				ret_val = AL_TRUE;
			}
			else{ret_val = AL_FALSE;}
		}
		else{ret_val = AL_FALSE;}	//This should never occur
		pthread_mutex_unlock(&audio_streamer_lock);
		return ret_val;
	}
	else{
		alSourceStop(source->source_id);
		if(audio_test_error(&error, "source stopping") == AL_TRUE){return AL_FALSE;}
	}
	return AL_TRUE;
}

ALboolean audio_prep_stream_buffers(){
	ALvoid * data;

	uint8_t i;
	// Fill all the buffers with audio data from the wave file
	for (i = 0; i < AUDIO_STREAMING_NUM_BUFFERS; i++)
	{
		data = malloc(AUDIO_STREAMING_DATA_CHUNK_SIZE);	//This data is empty
		audio_WAVE_buffer_fill(data);	//Then its filled
		alBufferData(audio_streamer_source->buffer_id[i], audio_streamer_source->info->format, data, AUDIO_STREAMING_DATA_CHUNK_SIZE, audio_streamer_source->info->freq);
		free(data);
		alSourceQueueBuffers(audio_streamer_source->source_id, 1, &audio_streamer_source->buffer_id[i]);
	}

	ALCenum error;
	if(audio_test_error(&error, "loading wav file") == AL_TRUE){return AL_FALSE;}
	return AL_TRUE;
}

//Need to fix returns to unalloc memory if need be
//I won't be checking the return type so it doesn't matter
void * audio_stream_player(void * args){
	//Rework everything below
	while(1){
		;
	}

	if(audio_prep_stream_buffers() == AL_FALSE){return NULL;}

 	ALvoid *data;

	ALint iBuffersProcessed;
	ALint iTotalBuffersProcessed;
	ALuint uiBuffer;
	// Buffer queuing loop must operate in a new thread
	iBuffersProcessed = 0;

	if(audio_play_source(audio_streamer_source) == AL_FALSE){return NULL;}

	while (1)
	{

		//Different play states
		// AL_STOPPED
		// AL_PLAYING
		// AL_PAUSED
		// AL_INITIAL (Set by rewind)
		// AL_UNDETERMINED (When a source is initially stated)
		// AL_STREAMING (after successful alSourceQueueBuffers)

		// - The difference between STOP and PAUSE is that calling alSourcePlay after pausing
		// will resume from the position the source was when it was paused and
		// calling alSourcePlay after stopping will resume from the beginning of
		// the buffer(s).

		//Normally when the attached buffers are done playing, the source will progress to the stopped state

		if(audio_update_source_state(audio_streamer_source) == AL_FALSE){return NULL;}
		// while(audio_streamer_source->source_state != AL_STOPPED){	//This allows us to pause it later
		while(audio_streamer_source->source_state == AL_PLAYING){
			//Process any command it might have been given
			;

			alGetSourcei(audio_streamer_source->source_id, AL_BUFFERS_PROCESSED, &iBuffersProcessed);	//Is this call required?

			// Buffer queuing loop must operate in a new thread
			iBuffersProcessed = 0;
			alGetSourcei(audio_streamer_source->source_id, AL_BUFFERS_PROCESSED, &iBuffersProcessed);

			iTotalBuffersProcessed += iBuffersProcessed;	//Is that var required?

			// For each processed buffer, remove it from the source queue, read the next chunk of
			// audio data from the file, fill the buffer with new data, and add it to the source queue
			while (iBuffersProcessed)
			{
				// Remove the buffer from the queue (uiBuffer contains the buffer ID for the dequeued buffer)
				//The unqueue operation will only take place if all n (1) buffers can be removed from the queue.
				//Thats why we do it one at a time
				uiBuffer = 0;
				alSourceUnqueueBuffers(audio_streamer_source->source_id, 1, &uiBuffer);

				// Read more pData audio data (if there is any)
				data = malloc(AUDIO_STREAMING_DATA_CHUNK_SIZE);
				audio_WAVE_buffer_fill(data);
				// Copy audio data to buffer
				alBufferData(uiBuffer, audio_streamer_source->info->format, data, AUDIO_STREAMING_DATA_CHUNK_SIZE, audio_streamer_source->info->freq);
				free(data);
				// Insert the audio buffer to the source queue
				alSourceQueueBuffers(audio_streamer_source->source_id, 1, &uiBuffer);

				iBuffersProcessed--;
			}

			if(audio_update_source_state(audio_streamer_source) == AL_FALSE){return NULL;}
			
			//All of these will basically tell the thread manager that this thread is done and if any other threads are waiting then
			//we should process them
			#if defined(_arch_unix) || defined(_arch_dreamcast)
				sched_yield();
			#endif
			#ifdef _arch_windows
				sleep(0);	// https://stackoverflow.com/questions/3727420/significance-of-sleep0
							//Might want to replace this with something else since the CPU will be at 100% if this is the only active thread
			#endif
		}
	}

	// if(al_stop_source(audio_streamer_source) == AL_FALSE){return 4;}

	// fclose(audio_streamer_fp);

	//Should I then reset the buffers?

	return 0;
}

void audio_WAVE_buffer_fill(ALvoid * data){
	int spare = (audio_streamer_source->info->size + WAV_HDR_SIZE) - ftell(audio_streamer_fp);	//This is how much data in the entire file
	int read = fread(data, 1, MIN(AUDIO_STREAMING_DATA_CHUNK_SIZE, spare), audio_streamer_fp);
	if(read < AUDIO_STREAMING_DATA_CHUNK_SIZE){
		fseek(audio_streamer_fp, WAV_HDR_SIZE, SEEK_SET);	//Skips the header, beginning of body
		if(audio_streamer_source->looping){	//Fill from beginning
			fread(&((char*)data)[read], AUDIO_STREAMING_DATA_CHUNK_SIZE - read, 1, audio_streamer_fp);
		}
		else{	//Fill with zeroes
			memset(&((char *)data)[read], 0, AUDIO_STREAMING_DATA_CHUNK_SIZE - read);
			//Also send info somehow telling it to not loop
			//Note: Currently if looping doesn't actually do anything. That might make it easier for us to stop it from looping
		}
	}
}


//----------------------ADJUSTMENT---------------------//


uint8_t audio_adjust_master_volume(float vol){
	if(vol < 0){return 1;}

	ALCenum error;
	alListenerf(AL_GAIN, vol);
	if(audio_test_error(&error, "listener gain") == AL_TRUE){return 1;}
	return 0;
}

uint8_t audio_adjust_source_volume(audio_source_t * source, float vol){
	if(vol < 0 || source == NULL){return 1;}

	ALCenum error;
	alSourcef(source->source_id, AL_GAIN, vol);
	if(audio_test_error(&error, "source gain") == AL_TRUE){return 1;}
	return 0;
}

uint8_t audio_adjust_source_speed(audio_source_t * source, float speed){
	if(speed < 0 || source == NULL){return 1;}

	ALCenum error;
	alSourcef(source->source_id, AL_PITCH, speed);
	if(audio_test_error(&error, "source pitch") == AL_TRUE){return 1;}
	return 0;
}

uint8_t audio_set_source_looping(audio_source_t * source, ALboolean looping){
	if(source == NULL){return 1;}

	ALCenum error;
	alSourcei(source->source_id, AL_LOOPING, looping);
	if(audio_test_error(&error, "source looping") == AL_TRUE){return AL_FALSE;}
	return 0;
}


//----------------------MISC---------------------------//


ALboolean audio_test_error(ALCenum * error, char * msg){
	*error = alGetError();
	if(*error != AL_NO_ERROR){
        fprintf(stderr, "ERROR: ");
		fprintf(stderr, msg);
		fprintf(stderr, "\n");
		return AL_TRUE;
	}
	return AL_FALSE;
}

void al_list_audio_devices(const ALCchar *devices){
	const ALCchar *device = devices, *next = devices + 1;
	size_t len = 0;

	fprintf(stdout, "Devices list:\n");
	fprintf(stdout, "----------\n");
	while(device && *device != '\0' && next && *next != '\0'){
		fprintf(stdout, "%s\n", device);
		len = strlen(device);
		device += (len + 1);
		next += (len + 2);
	}
	fprintf(stdout, "----------\n");
}

inline ALenum to_al_format(short channels, short samples){
	bool stereo = (channels > 1);

	switch(samples){
	case 16:
		if(stereo){
			return AL_FORMAT_STEREO16;
		}
		else{
			return AL_FORMAT_MONO16;
		}
	case 8:
		if(stereo){
			return AL_FORMAT_STEREO8;
		}
		else{
			return AL_FORMAT_MONO8;
		}
	default:
		return -1;
	}
}

bool is_big_endian(){
	int a = 1;
	return !((char*)&a)[0];
}

int convert_to_int(char * buffer, int len){
	int i = 0;
	int a = 0;
	if(!is_big_endian()){
		for(; i<len; i++){
			((char*)&a)[i] = buffer[i];
		}
	}
	else{
		for(; i<len; i++){
			((char*)&a)[3 - i] = buffer[i];
		}
	}
	return a;
}
