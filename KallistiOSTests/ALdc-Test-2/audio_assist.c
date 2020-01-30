#include "audio_assist.h"

uint8_t al_init(){
	al_streamer_source = NULL;
	al_streamer_fp = NULL;

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
	if(al_test_error(&error, "make default context") == AL_TRUE){return 1;}

	// set orientation
	alListener3f(AL_POSITION, 0, 0, 1.0f);
	if(al_test_error(&error, "listener position") == AL_TRUE){return 1;}

    alListener3f(AL_VELOCITY, 0, 0, 0);
	if(al_test_error(&error, "listener velocity") == AL_TRUE){return 1;}

	alListenerfv(AL_ORIENTATION, listenerOri);
	if(al_test_error(&error, "listener orientation") == AL_TRUE){return 1;}

	return 0;
}

void al_shutdown(){
	// al_context = alcGetCurrentContext();	//With only one device/context, this line might not be required
	// al_device = alcGetContextsDevice(al_context);
	alcMakeContextCurrent(NULL);
	alcDestroyContext(al_context);
	alcCloseDevice(al_device);
	return;
}

ALboolean al_load_WAV_file_info(const char * filename, al_audio_info_t * info, uint8_t mode){
	if((mode & AL_AS_TYPE_STREAM) && (al_streamer_source != NULL || al_streamer_fp != NULL)){
		return AL_FALSE;
	}

	info->play_type = mode & AL_AS_TYPE_STREAM;
	uint16_t length = strlen(filename);
	switch(info->play_type){
		case AL_AS_TYPE_NON_STREAM:
			info->is_cdda = 0;
			info->path = malloc(sizeof(char) * length);
			strcpy(info->path, filename);
		break;
		case AL_AS_TYPE_STREAM:
			info->is_cdda = 0;
			info->path = malloc(sizeof(char) * length);
			strcpy(info->path, filename);
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

	// fseek(in, 0, SEEK_END);
	// info->size = (ALsizei) (ftell(in) - WAV_HDR_SIZE);	//This is the true size of the file
	// fseek(in, 0, SEEK_SET);

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

	if(info->play_type != AL_AS_TYPE_STREAM){
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

	if(info->play_type != AL_AS_TYPE_STREAM){
		fclose(in);
	}
	else{
		al_streamer_fp = in;		
	}

	printf("Loaded WAV file!\n");

	return AL_TRUE;
}

//Currently unimplemented
ALboolean al_load_CDDA_track_info(uint8_t track, al_audio_info_t * info, uint8_t mode){
	if(mode & AL_AS_TYPE_STREAM && (al_streamer_source != NULL || al_streamer_fp != NULL)){
		return AL_FALSE;
	}

	return AL_FALSE;
}

ALboolean al_unload_audio_info(al_audio_info_t * info){
	if(info == NULL){
		return AL_FALSE;
	}

	if(info->data){
		free(info->data);
		info->data = NULL;
	}

	if(info->path){
		free(info->path);
		info->path = NULL;
	}

	if(info->play_type == AL_AS_TYPE_STREAM){
		fclose(al_streamer_fp);
		al_streamer_fp = NULL;
		al_streamer_source->info = NULL;
		al_free_audio_source(al_streamer_source);
	}

	return AL_TRUE;
}

ALboolean al_free_audio_source(al_audio_source_t * source){
	if(source == NULL){
		return AL_FALSE;
	}

	uint8_t num_buffers = (source->info->play_type == AL_AS_TYPE_STREAM) ? STREAMING_NUM_BUFFERS : 1;

	//1st param is number of buffers
	alDeleteSources(1, &source->source_id);
	alDeleteBuffers(num_buffers, source->buffer_id);

	return AL_TRUE;
}

ALboolean al_create_source(al_audio_source_t * source, al_audio_info_t * info, vec2_f_t position, ALboolean looping,
	float volume, float speed, uint8_t flag){

	if(source == NULL || info == NULL || volume < 0 || speed < 0){
		return AL_FALSE;
	}

	if(info->play_type == AL_AS_TYPE_STREAM){
		if(al_streamer_source != NULL){return AL_FALSE;}
		al_streamer_source = source;
	}

	source->info = info;
	source->position = position;
	source->looping = looping;	//0 or 1
	source->volume = volume;
	source->speed = speed;

	ALCenum error;

	alGenSources((ALuint)1, &source->source_id);	//Generate one source
	if(al_test_error(&error, "source generation") == AL_TRUE){return AL_FALSE;}

	alSourcef(source->source_id, AL_PITCH, speed);
	if(al_test_error(&error, "source pitch") == AL_TRUE){return AL_FALSE;}

	alSourcef(source->source_id, AL_GAIN, volume);
	if(al_test_error(&error, "source gain") == AL_TRUE){return AL_FALSE;}

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
	if(al_test_error(&error, "source position") == AL_TRUE){return AL_FALSE;}

	alSource3f(source->source_id, AL_VELOCITY, 0, 0, 0);
	if(al_test_error(&error, "source velocity") == AL_TRUE){return AL_FALSE;}

	alSourcei(source->source_id, AL_LOOPING, looping);
	if(al_test_error(&error, "source looping") == AL_TRUE){return AL_FALSE;}

	/*
		Other alSourcei options:

		AL_SOURCE_RELATIVE
		AL_CONE_INNER_ANGLE
		AL_CONE_OUTER_ANGLE
		AL_BUFFER
		AL_SOURCE_STATE
	*/

	//1 buffer normally, but "STREAMING_NUM_BUFFERS" for streaming
	uint8_t num_buffers = (info->play_type == AL_AS_TYPE_STREAM) ? STREAMING_NUM_BUFFERS : 1;

	//Generate the buffers
	source->buffer_id = malloc(sizeof(ALuint) * num_buffers);
	alGenBuffers(num_buffers, source->buffer_id);	//Generating "num_buffers" buffer. 2nd param is a pointer to an array
													//of ALuint values which will store the names of the new buffers
													//Seems "buffer" is just an ID and doesn't actually contain the data?
	if(al_test_error(&error, "buffer generation") == AL_TRUE){return AL_FALSE;}

	if(info->play_type == AL_AS_TYPE_NON_STREAM){
		alBufferData(source->buffer_id[0], info->format, info->data, info->size, info->freq);	//Fill the buffer with PCM data
		if(al_test_error(&error, "buffer copy") == AL_TRUE){return AL_FALSE;}

		alSourcei(source->source_id, AL_BUFFER, source->buffer_id[0]);
		if(al_test_error(&error, "buffer binding") == AL_TRUE){return AL_FALSE;}
	}

	//Fix this condition later
	if(info->data != NULL){
		free(info->data);	//Its no longer required
		info->data = NULL;
	}

	return AL_TRUE;
}

ALboolean al_update_source_state(al_audio_source_t * source){
	if(source == NULL){
		return AL_FALSE;
	}

	ALCenum error;
	alGetSourcei(source->source_id, AL_SOURCE_STATE, &source->source_state);
	if(al_test_error(&error, "source state get") == AL_TRUE){return AL_FALSE;}

	return AL_TRUE;
}

ALboolean al_play_source(al_audio_source_t * source){
	if(source == NULL){
		return AL_FALSE;
	}

	ALCenum error;
	alSourcePlay(source->source_id);	//If called on a source that is already playing, it will restart from the beginning	
	if(al_test_error(&error, "source playing") == AL_TRUE){return AL_FALSE;}

	return AL_TRUE;
}

ALboolean al_pause_source(al_audio_source_t * source){
	if(source == NULL){
		return AL_FALSE;
	}

	ALCenum error;
	alSourcePause(source->source_id);	//If called on a source that is already playing, it will restart from the beginning	
	if(al_test_error(&error, "source pausing") == AL_TRUE){return AL_FALSE;}

	return AL_TRUE;
}

ALboolean al_stop_source(al_audio_source_t * source){
	if(source == NULL){
		return AL_FALSE;
	}

	ALCenum error;
	alSourceStop(source->source_id);	//If called on a source that is already playing, it will restart from the beginning	
	if(al_test_error(&error, "source stopping") == AL_TRUE){return AL_FALSE;}

	return AL_TRUE;
}

ALboolean al_prep_stream_buffers(){
	ALvoid * data;

	uint8_t i;
	// Fill all the buffers with audio data from the wave file
	for (i = 0; i < STREAMING_NUM_BUFFERS; i++)
	{
		data = malloc(AL_AS_STREAMING_DATA_CHUNK_SIZE);	//This data is empty
		al_WAVE_buffer_fill(data);	//Then its filled
		alBufferData(al_streamer_source->buffer_id[i], al_streamer_source->info->format, data, AL_AS_STREAMING_DATA_CHUNK_SIZE, al_streamer_source->info->freq);
		free(data);
		alSourceQueueBuffers(al_streamer_source->source_id, 1, &al_streamer_source->buffer_id[i]);
	}

	ALCenum error;
	if(al_test_error(&error, "loading wav file") == AL_TRUE){return AL_FALSE;}
	return AL_TRUE;
}

// int8_t al_prep_stream_buffers(ALuint source, ALuint * buffer, uint8_t num_buffers, ALsizei freq, ALenum format){
// 	ALvoid *data;

// 	int i;
// 	// Fill all the buffers with audio data from the wave file
// 	for (i = 0; i < num_buffers; i++)
// 	{
// 		data = malloc(DATA_CHUNK_SIZE);	//This data is empty
// 		WAVE_buffer(data);	//Then its filled
// 		alBufferData(buffer[i], format, data, DATA_CHUNK_SIZE, freq);
// 		free(data);
// 		alSourceQueueBuffers(source, 1, &buffer[i]);
// 	}
// 	TEST_ERROR("loading wav file");

// 	return 0;
// }

uint8_t al_stream_player(){
	ALint source_state;
 	ALvoid *data;

	ALint iBuffersProcessed;
	ALint iTotalBuffersProcessed;
	ALuint uiBuffer;
	// Buffer queuing loop must operate in a new thread
	iBuffersProcessed = 0;

	if(al_play_source(al_streamer_source) == AL_FALSE){return 1;}

	// while (1)
	// {
		if(al_update_source_state(al_streamer_source) == AL_FALSE){return 2;}
		while (source_state == AL_PLAYING)	//How is the state changing?
		// while (1)	//This works
		{
			alGetSourcei(al_streamer_source->source_id, AL_BUFFERS_PROCESSED, &iBuffersProcessed);	//Is this call required?

			// Buffer queuing loop must operate in a new thread
			iBuffersProcessed = 0;
			alGetSourcei(al_streamer_source->source_id, AL_BUFFERS_PROCESSED, &iBuffersProcessed);

			iTotalBuffersProcessed += iBuffersProcessed;	//Is that var required?

			// For each processed buffer, remove it from the source queue, read the next chunk of
			// audio data from the file, fill the buffer with new data, and add it to the source queue
			while (iBuffersProcessed)
			{
				// Remove the buffer from the queue (uiBuffer contains the buffer ID for the dequeued buffer)
				uiBuffer = 0;
				alSourceUnqueueBuffers(al_streamer_source->source_id, 1, &uiBuffer);

				// Read more pData audio data (if there is any)
				data = malloc(AL_AS_STREAMING_DATA_CHUNK_SIZE);
				al_WAVE_buffer_fill(data);
				// Copy audio data to buffer
				alBufferData(uiBuffer, al_streamer_source->info->format, data, AL_AS_STREAMING_DATA_CHUNK_SIZE, al_streamer_source->info->freq);
				free(data);
				// Insert the audio buffer to the source queue
				alSourceQueueBuffers(al_streamer_source->source_id, 1, &uiBuffer);

				iBuffersProcessed--;
			}

			if(al_update_source_state(al_streamer_source) == AL_FALSE){return 3;}
			thd_pass();	//Not entirely sure what this does
		}
	// }

	// fclose(in);

	//Should I then reset the buffers?

	return 0;	//How is it getting here?
}

void al_WAVE_buffer_fill(ALvoid * data){
	int spare = al_streamer_source->info->size - ftell(al_streamer_fp);
	int read = fread(data, 1, MIN(AL_AS_STREAMING_DATA_CHUNK_SIZE, spare), al_streamer_fp);
	// printf("Reading WAV: read: %d spare:%d\n", read, spare);
	if (read < AL_AS_STREAMING_DATA_CHUNK_SIZE)
	{
		fseek(al_streamer_fp, WAV_HDR_SIZE, SEEK_SET);
		// printf("Writing silence\n");
		// memset(&((char *)data)[read], 0, AL_AS_STREAMING_DATA_CHUNK_SIZE - read);	//This sets the remainder of the buffer to zero. Use if not looping
		fread(&((char*)data)[read], AL_AS_STREAMING_DATA_CHUNK_SIZE - read, 1, al_streamer_fp);	//Fills with beginning. Use if looping
	}
}


//----------------------ADJUSTMENT---------------------//


uint8_t al_adjust_master_volume(float vol){
	if(vol < 0){return 1;}

	ALCenum error;
	alListenerf(AL_GAIN, vol);
	if(al_test_error(&error, "listener gain") == AL_TRUE){return 1;}
	return 0;
}

uint8_t al_adjust_source_volume(al_audio_source_t * source, float vol){
	if(vol < 0 || source == NULL){return 1;}

	ALCenum error;
	alSourcef(source->source_id, AL_GAIN, vol);
	if(al_test_error(&error, "source gain") == AL_TRUE){return 1;}
	return 0;
}

uint8_t al_adjust_source_speed(al_audio_source_t * source, float speed){
	if(speed < 0 || source == NULL){return 1;}

	ALCenum error;
	alSourcef(source->source_id, AL_PITCH, speed);
	if(al_test_error(&error, "source pitch") == AL_TRUE){return 1;}
	return 0;
}

uint8_t al_set_source_looping(al_audio_source_t * source, ALboolean looping){
	if(source == NULL){return 1;}

	ALCenum error;
	alSourcei(source->source_id, AL_LOOPING, looping);
	if(al_test_error(&error, "source looping") == AL_TRUE){return AL_FALSE;}
	return 0;
}


//----------------------MISC---------------------------//


ALboolean al_test_error(ALCenum * error, char * msg){
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
