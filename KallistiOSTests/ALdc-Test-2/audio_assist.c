#include "audio_assist.h"

uint8_t al_init(){
	ALboolean enumeration;
	ALfloat listenerOri[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };	//Double check what these vars mean
	ALCenum error;

	enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
	if(enumeration == AL_FALSE){
		fprintf(stderr, "enumeration extension not available\n");
	}

	list_audio_devices(alcGetString(NULL, ALC_DEVICE_SPECIFIER));

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
	info->play_type = mode & AL_AS_TYPE_STREAM;
	uint16_t length = strlen(filename);
	switch(mode){
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
	info->size = (ALsizei) convert_to_int(buffer, 4);
	info->data = (ALvoid*) malloc(info->size * sizeof(char));
	fread(info->data, info->size, sizeof(char), in);

	if(chan == 1){
		info->format = (bps == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
	}
	else{
		info->format = (bps == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
	}

	fclose(in);

	printf("Loaded WAV file!\n");

	return AL_TRUE;
}

//Currently unimplemented
ALboolean al_load_CDDA_track_info(uint8_t track, al_audio_info_t * info, uint8_t mode){
	return AL_FALSE;
}

void al_unload_audio_info(al_audio_info_t * info){
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
		al_streamer_info = NULL;
	}
}

ALboolean al_create_source(al_audio_source_t * source, al_audio_info_t * info, vec2_f_t position, ALboolean looping,
	float volume, float speed, uint8_t num_buffers){
	if(source == NULL || info == NULL || volume < 0 || speed < 0 || num_buffers == 0){
		return AL_FALSE;
	}

	source->info = info;
	source->position = position;
	source->looping = looping;	//0 or 1
	source->volume = volume;
	source->speed = speed;
	source->num_buffers = num_buffers;

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

	// source->buffer_id	//This one's a pointer
	// source->source_state

	//Generate the buffers
	source->buffer_id = malloc(sizeof(ALuint) * num_buffers);
	alGenBuffers(num_buffers, source->buffer_id);	//Generating "num_buffers" buffer. 2nd param is a pointer to an array
												//of ALuint values which will store the names of the new buffers
												//Seems "buffer" is just an ID and doesn't actually contain the data?
	if(al_test_error(&error, "buffer generation") == AL_TRUE){return AL_FALSE;}

	alBufferData(source->buffer_id[0], info->format, info->data, info->size, info->freq);	//Fill the buffer with PCM data
	if(al_test_error(&error, "buffer copy") == AL_TRUE){return AL_FALSE;}

	if(1){
		free(info->data);	//Its no longer required
		info->data = NULL;
	}

	alSourcei(source->source_id, AL_BUFFER, source->buffer_id[0]);
	if(al_test_error(&error, "buffer binding") == AL_TRUE){return AL_FALSE;}

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
	alSourcefi(source->source_id, AL_GAIN, vol);
	if(al_test_error(&error, "source gain") == AL_TRUE){return 1;}
	return 0;
}

uint8_t al_adjust_source_speed(al_audio_source_t * source, float speed){
	if(speed < 0 || source == NULL){return 1;}

	ALCenum error;
	alSourcefi(source->source_id, AL_PITCH, speed);
	if(al_test_error(&error, "source pitch") == AL_TRUE){return 1;}
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

void list_audio_devices(const ALCchar *devices){
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

// void WAVE_buffer(void *data){
	// int spare = WAV_size - ftell(in);
	// int read = fread(data, 1, MIN(DATA_CHUNK_SIZE, spare), in);
	// printf("Reading WAV: read: %d spare:%d\n", read, spare);
	// if(read < DATA_CHUNK_SIZE){
	// 	printf("Writing silence\n");
	// 	fseek(in, WAV_data_start, SEEK_SET);
	// 	memset(&((char *)data)[read], 0, DATA_CHUNK_SIZE - read);
	// 	//fread(&((char*)data)[read], DATA_CHUNK_SIZE-read, 1, in);
	// }
// }
