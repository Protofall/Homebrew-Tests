#include "audio_assist.h"


//----------------------MISC---------------------------//


ALboolean audio_test_error(ALCenum * error, char * msg){
	*error = alGetError();
	if(*error != AL_NO_ERROR){
		fprintf(stderr, "ERROR: %s\n", msg);
		return AL_TRUE;
	}
	return AL_FALSE;
}

static void al_list_audio_devices(const ALCchar *devices){
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

static bool is_big_endian(){
	int a = 1;
	return !((char*)&a)[0];
}

static int convert_to_int(char * buffer, int len){
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

static void sleep_ms(int milliseconds){
#ifdef _WIN32
	Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
	struct timespec ts;
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000000;
	nanosleep(&ts, NULL);
#else
	usleep(milliseconds * 1000);
#endif
	return;
}


//----------------------SETUP---------------------------//


uint8_t audio_init(){
	__audio_streamer_source = NULL;
	__audio_streamer_info = NULL;
	__audio_streamer_fp = NULL;
	__audio_streamer_stopping = 0;
	__audio_streamer_command = AUDIO_COMMAND_NONE;
	__audio_streamer_thd_active = 0;

	ALboolean enumeration;	//Should this be a ALCboolean instead?
	ALfloat listenerOri[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };	//Double check what these vars mean
	ALCenum error;

	enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
	if(enumeration == AL_FALSE){
		fprintf(stderr, "enumeration extension not available\n");
	}

	al_list_audio_devices(alcGetString(NULL, ALC_DEVICE_SPECIFIER));

	//Chooses the preferred/default device
	const ALCchar *defaultDeviceName;
	defaultDeviceName = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
	if(!defaultDeviceName){
		fprintf(stderr, "unable to get default device name\n");
		return 1;
	}
	__al_device = alcOpenDevice(defaultDeviceName);
	if(!__al_device){
		fprintf(stderr, "unable to open default device\n");
		return 1;
	}

	alGetError();	//This resets the error state

	__al_context = alcCreateContext(__al_device, NULL);
	if(!alcMakeContextCurrent(__al_context)){
		fprintf(stderr, "failed to make default context\n");
		goto error1;
	}
	if(audio_test_error(&error, "make default context") == AL_TRUE){goto error1;}

	// set orientation
	alListener3f(AL_POSITION, 0, 0, 1.0f);
	if(audio_test_error(&error, "listener position") == AL_TRUE){goto error2;}

	alListener3f(AL_VELOCITY, 0, 0, 0);
	if(audio_test_error(&error, "listener velocity") == AL_TRUE){goto error2;}

	alListenerfv(AL_ORIENTATION, listenerOri);
	if(audio_test_error(&error, "listener orientation") == AL_TRUE){goto error2;}

	// Init the mutex for the streamer thread
	if(pthread_mutex_init(&__audio_streamer_lock, NULL) != 0){
		printf("Mutex init failed\n");
		goto error2;
	}

	return 0;

	error2:
	alcMakeContextCurrent(NULL);
	alcDestroyContext(__al_context);

	error1:
	alcCloseDevice(__al_device);

	return 1;
}

void audio_shutdown(){
	//Just incase it hasn't finished shutting down yet
	uint8_t status = 0;
	pthread_mutex_lock(&__audio_streamer_lock);
	if(__audio_streamer_thd_active == 1){
		status = 1;
		__audio_streamer_command = AUDIO_COMMAND_END;
	}
	pthread_mutex_unlock(&__audio_streamer_lock);
	if(status){	//Note, if it has already terminated, then this does nothing
		pthread_join(__audio_streamer_thd_id, NULL);
	}
	pthread_mutex_destroy(&__audio_streamer_lock);

	// al_context = alcGetCurrentContext();	//With only one device/context, this line might not be required
	// __al_device = alcGetContextsDevice(al_context);
	alcMakeContextCurrent(NULL);
	alcDestroyContext(__al_context);
	alcCloseDevice(__al_device);

	return;
}

ALboolean audio_load_WAV_file_info(const char * filename, audio_info_t * info, uint8_t mode){
	printf("CRAYON AUDIO: Loading file %s\n", filename);
	if((mode & AUDIO_STREAMING) && (__audio_streamer_info != NULL || __audio_streamer_fp != NULL)){
		return AL_FALSE;
	}

	info->streaming = !!(mode & AUDIO_STREAMING);
	info->data_type = AUDIO_DATA_TYPE_WAV;

	char buffer[4];

	FILE* in = fopen(filename, "rb");
	if(!in){
		fprintf(stderr, "Couldn't open file\n");
		return AL_FALSE;
	}


	// ---------------READING-THE-HDR--------------------


	fread(buffer, 4, sizeof(char), in);
	if(strncmp(buffer, "RIFF", 4) != 0){
		fprintf(stderr, "Not a RIFF file\n");
		goto error1;
	}

	fseek(in, 4, SEEK_CUR);	// Skip this, the file size is useless

	fread(buffer, 4, sizeof(char), in);
	if(strncmp(buffer, "WAVE", 4) != 0){
		fprintf(stderr, "Not a WAVE file\n");
		goto error1;
	}

	fread(buffer, 4, sizeof(char), in);
	if(strncmp(buffer, "fmt ", 4) != 0){
		fprintf(stderr, "\"fmt\" hdr incorrect\n");
		goto error1;
	}

	// Size of the fmt field
	fread(buffer, 4, sizeof(char), in);
	int fmt_size = convert_to_int(buffer, 4);
	printf("CRAYON AUDIO: fmt_size = %d\n", fmt_size);

	// AudioFormat type
	fread(buffer, 2, sizeof(char), in);
	int type = convert_to_int(buffer, 2);
	if(type != 1){
		printf("WAV's data type is %d which isn't PCM. Can't continue\n", type);
		goto error1;
	}
	printf("CRAYON AUDIO: type = %d\n", type);

	// Number of channels
	fread(buffer, 2, sizeof(char), in);
	int chan = convert_to_int(buffer, 2);
	printf("CRAYON AUDIO: chan = %d\n", chan);

	// The sample rate
	fread(buffer, 4, sizeof(char), in);
	info->freq = convert_to_int(buffer, 4);
	printf("CRAYON AUDIO: freq = %"PRIu32"\n", info->freq);

	fseek(in, 6, SEEK_CUR);	// Skip two fields that...shouldn't really exist anyways.

	// Get the Bits Per Sample
	fread(buffer, 2, sizeof(char), in);
	uint16_t bps = convert_to_int(buffer, 2);
	printf("CRAYON AUDIO: bps = %"PRIu16"\n", bps);

	fseek(in, 20 + fmt_size, SEEK_SET);	// Skip to the data section. If the type wasn't PCM, then there's fields we'd skip

	fread(buffer, 4, sizeof(char), in);
	if(strncmp(buffer, "data", 4) != 0){
		fprintf(stderr, "\"data\" hdr incorrect\n");
		goto error1;
	}

	// The supposed data size
	fread(buffer, 4, sizeof(char), in);
	info->size = convert_to_int(buffer, 4);	// This is just the size of the data
	printf("CRAYON AUDIO: size = %"PRIu32"\n", info->size);

	// The offset from the beginning of the file. Should be 44
	int data_offset = ftell(in);	// We'll use this instead of WAV_HDR_SIZE. STORE THIS IN INFO STRUCT


	// ---------------END-READING-THE-HDR--------------------


	ALvoid *data = NULL;
	if(info->streaming == AUDIO_NOT_STREAMING){
		data = (ALvoid*) malloc(info->size * sizeof(char));
		if(data == NULL){
			// error_freeze("Couldn't allocate data memory");
			goto error1;
		}
		fread(data, info->size, sizeof(char), in);
		fclose(in);
	}
	else{
		__audio_streamer_fp = in;
	}

	// I think the DC only accepts bps = 16 for uncompressed PCM data
	// And bpp = 4 for the special Yamaha ADPCM format. So bps = 8 should never appear
	if(chan == 1){
		info->format = (bps == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
	}
	else{
		info->format = (bps == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
	}

	//Generate the buffers
	ALCenum error;
	info->srcs_attached = 0;
	info->buff_cnt = (info->streaming == AUDIO_STREAMING) ? AUDIO_STREAMING_NUM_BUFFERS : 1;
	info->buff_id = malloc(sizeof(ALuint) * info->buff_cnt);
	if(info->buff_id == NULL){
		// error_freeze("Couldn't allocate buff ids"); 
		goto error2;
	}
	alGenBuffers(info->buff_cnt, info->buff_id);	//Generating "info->buff_cnt" buffers. 2nd param is a pointer to an array
													//of ALuint values which will store the names of the new buffers
													//Seems "buff_id" doesn't actually contain the data?
	if(audio_test_error(&error, "buffer generation") == AL_TRUE){
		// error_freeze("Couldn't gen buffers");
		goto error3;
	}

	//Filling the buffer for non-streamers
	if(info->streaming == AUDIO_NOT_STREAMING){
		printf("CRAYON AUDIO: alloced %"PRIu32"\n", info->size);
		// if(info->size > AUDIO_MAX_BUFFER_SIZE){
		// 	printf("CRAYON AUDIO: WARNING! Using too much sound RAM\n");
		// }
		// uint32_t rem_space = snd_mem_available();
		// if(info->size > rem_space){
		// 	printf("CRAYON AUDIO: WARNING! Requesting more space than available (%d)\n", rem_space);
		// }

		alBufferData(info->buff_id[0], info->format, data, info->size, info->freq);	//Fill the buffer with PCM data
		if(audio_test_error(&error, "buffer copy") == AL_TRUE){
			// error_freeze("Couldn't fill buffers"); 
			goto error4;
		}

		free(data);
	}
	else{
		__audio_streamer_info = info;
	}

	printf("CRAYON AUDIO: Successfully loaded %s\n", filename);

	return AL_TRUE;


	//--------------------


	error4:
	alDeleteBuffers(info->buff_cnt, info->buff_id);

	error3:
	free(info->buff_id);

	error2:
	if(data){free(data);}
	if(info->streaming == AUDIO_STREAMING){
		fclose(in);
	}
	return AL_FALSE;

	error1:
	fclose(in);
	return AL_FALSE;
}

//Currently unimplemented
ALboolean audio_load_CDDA_track_info(uint8_t drive, uint8_t track, audio_info_t * info, uint8_t mode){
	if(mode & AUDIO_STREAMING && (__audio_streamer_source != NULL || __audio_streamer_fp != NULL)){
		return AL_FALSE;
	}

	//Delete these later
	(void)drive;
	(void)track;
	(void)info;

	//Fix later
	return AL_FALSE;
}

ALboolean audio_load_OGG_file_info(const char * path, audio_info_t * info, uint8_t mode){
	if(mode & AUDIO_STREAMING && (__audio_streamer_source != NULL || __audio_streamer_fp != NULL)){
		return AL_FALSE;
	}

	//Delete these later
	(void)path;
	(void)info;

	//Fix later
	return AL_FALSE;
}

ALboolean audio_free_info(audio_info_t * info){
	if(info == NULL){
		return AL_FALSE;
	}

	if(info->streaming == AUDIO_STREAMING){
		//We haven't tried to free the source yet
		if(__audio_streamer_thd_active && __audio_streamer_command != AUDIO_COMMAND_END){
			return AL_FALSE;
		}
		//The sleep time might need adjusting
		//Loop purpose is to make sure streamer has stopped working before we continue
		while(__audio_streamer_thd_active && __audio_streamer_command == AUDIO_COMMAND_END){
			sleep_ms(10);
		}

		fclose(__audio_streamer_fp);
		__audio_streamer_fp = NULL;
		__audio_streamer_info = NULL;
	}

	alDeleteBuffers(info->buff_cnt, info->buff_id);	//1st param is number of buffers
	free(info->buff_id);

	return AL_TRUE;
}

ALboolean audio_free_source(audio_source_t * source){
	if(source == NULL){
		return AL_FALSE;
	}

	//So we can later know there isn't a streamer presents
	if(source == __audio_streamer_source){
		pthread_mutex_lock(&__audio_streamer_lock);
		if(__audio_streamer_thd_active == 1){
			__audio_streamer_command = AUDIO_COMMAND_END;	//Doesn't matter what we said before, we want it to terminate now
		}
		pthread_mutex_unlock(&__audio_streamer_lock);
	}
	else{	//Streamer thread will handle this itself
		alDeleteSources(1, &source->src_id);
	}

	source->info->srcs_attached--;

	return AL_TRUE;
}

ALboolean audio_create_source(audio_source_t * source, audio_info_t * info, vec2_f_t position, ALboolean looping,
	float volume, float speed){

	if(source == NULL || info == NULL || volume < 0 || speed < 0){
		return AL_FALSE;
	}

	if(info->streaming == AUDIO_STREAMING){
		if(__audio_streamer_source != NULL){return AL_FALSE;}

		pthread_mutex_lock(&__audio_streamer_lock);
		if(__audio_streamer_thd_active == 1){	//There's already a streamer thread
			pthread_mutex_unlock(&__audio_streamer_lock);
			return AL_FALSE;
		}
		pthread_mutex_unlock(&__audio_streamer_lock);
		
		__audio_streamer_source = source;
	}

	source->info = info;
	source->position = position;
	source->looping = looping;
	source->current_volume = volume;
	source->target_volume = volume;
	source->speed = speed;

	ALCenum error;

	alGenSources((ALuint)1, &source->src_id);	//Generate one source
	if(audio_test_error(&error, "source generation") == AL_TRUE){return AL_FALSE;}

	alSourcef(source->src_id, AL_PITCH, speed);
	if(audio_test_error(&error, "source pitch") == AL_TRUE){return AL_FALSE;}

	alSourcef(source->src_id, AL_GAIN, volume);
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

	alSource3f(source->src_id, AL_POSITION, position.x, position.y, 0);	//Since we're 2D, Z is always zero
	if(audio_test_error(&error, "source position") == AL_TRUE){return AL_FALSE;}

	alSource3f(source->src_id, AL_VELOCITY, 0, 0, 0);
	if(audio_test_error(&error, "source velocity") == AL_TRUE){return AL_FALSE;}

	//When streaming we handle looping manually, so we tell OpenAL not to bother with looping
	alSourcei(source->src_id, AL_LOOPING, (info->streaming == AUDIO_STREAMING) ? AL_FALSE : looping);
	if(audio_test_error(&error, "source looping") == AL_TRUE){return AL_FALSE;}

	/*
		Other alSourcei options:

		AL_SOURCE_RELATIVE
		AL_CONE_INNER_ANGLE
		AL_CONE_OUTER_ANGLE
		AL_BUFFER
		AL_SOURCE_STATE
	*/
	if(info->streaming == AUDIO_NOT_STREAMING){
		alSourcei(source->src_id, AL_BUFFER, info->buff_id[0]);
		if(audio_test_error(&error, "buffer binding") == AL_TRUE){return AL_FALSE;}
	}
	else{	//We start the streamer thread
		if(pthread_create(&__audio_streamer_thd_id, NULL, audio_stream_player, NULL)){
			printf("Failed to create streamer thread\n");
			return AL_FALSE;
		}
		else{	//The threaded function already does this, but incase you try to change the state right after doing this and
				//somehow the threaded function hasn't run yet, this will allow it to do so
			pthread_mutex_lock(&__audio_streamer_lock);
			__audio_streamer_thd_active = 1;
			pthread_mutex_unlock(&__audio_streamer_lock);
		}
	}

	info->srcs_attached++;

	return AL_TRUE;
}

ALboolean audio_update_source_state(audio_source_t * source){
	if(source == NULL){
		return AL_FALSE;
	}

	ALCenum error;
	alGetSourcei(source->src_id, AL_SOURCE_STATE, &source->state);
	if(audio_test_error(&error, "source state get") == AL_TRUE){return AL_FALSE;}

	return AL_TRUE;
}

ALboolean audio_play_source(audio_source_t * source){
	if(source == NULL){return AL_FALSE;}

	ALCenum error;
	ALboolean ret_val;
	if(source == __audio_streamer_source){
		pthread_mutex_lock(&__audio_streamer_lock);
		if(__audio_streamer_thd_active == 1){
			if(__audio_streamer_command == AUDIO_COMMAND_NONE){
				__audio_streamer_command = AUDIO_COMMAND_PLAY;
				ret_val = AL_TRUE;
			}
			else{ret_val = AL_FALSE;}
		}
		else{ret_val = AL_FALSE;}	//This should never occur
		pthread_mutex_unlock(&__audio_streamer_lock);
		return ret_val;
	}
	alSourcePlay(source->src_id);	//If called on a source that is already playing, it will restart from the beginning
	if(audio_test_error(&error, "source playing") == AL_TRUE){return AL_FALSE;}
	return AL_TRUE;
}

ALboolean audio_pause_source(audio_source_t * source){
	if(source == NULL){return AL_FALSE;}

	ALCenum error;
	ALboolean ret_val;
	if(source == __audio_streamer_source){
		pthread_mutex_lock(&__audio_streamer_lock);
		if(__audio_streamer_thd_active == 1 && __audio_streamer_command == AUDIO_COMMAND_NONE){
			ret_val = AL_TRUE;
		}
		else{ret_val = AL_FALSE;}

		//Should checking the source state be out of the mutex lock?
		audio_update_source_state(source);
		if(source->state != AL_PLAYING){
			ret_val = AL_FALSE;
		}

		if(ret_val == AL_TRUE){
			__audio_streamer_command = AUDIO_COMMAND_PAUSE;
		}

		pthread_mutex_unlock(&__audio_streamer_lock);
		return ret_val;
	}
	alSourcePause(source->src_id);
	if(audio_test_error(&error, "source pausing") == AL_TRUE){return AL_FALSE;}
	return AL_TRUE;
}

ALboolean audio_unpause_source(audio_source_t * source){
	if(source == NULL){return AL_FALSE;}

	ALCenum error;
	ALboolean ret_val;
	if(source == __audio_streamer_source){
		pthread_mutex_lock(&__audio_streamer_lock);
		if(__audio_streamer_thd_active == 1 && __audio_streamer_command == AUDIO_COMMAND_NONE){
			ret_val = AL_TRUE;
		}
		else{ret_val = AL_FALSE;}

		//Should checking the source state be out of the mutex lock?
		audio_update_source_state(source);
		if(source->state != AL_PAUSED && source->state != AL_STOPPED){
			ret_val = AL_FALSE;
		}

		if(ret_val == AL_TRUE){
			__audio_streamer_command = AUDIO_COMMAND_UNPAUSE;
		}

		pthread_mutex_unlock(&__audio_streamer_lock);
		return ret_val;
	}
	alSourcePlay(source->src_id);
	if(audio_test_error(&error, "source playing") == AL_TRUE){return AL_FALSE;}
	return AL_TRUE;
}

ALboolean audio_stop_source(audio_source_t * source){
	if(source == NULL){return AL_FALSE;}

	ALCenum error;
	ALboolean ret_val;
	if(source == __audio_streamer_source){
		pthread_mutex_lock(&__audio_streamer_lock);
		if(__audio_streamer_thd_active == 1){
			if(__audio_streamer_command == AUDIO_COMMAND_NONE){
				__audio_streamer_command = AUDIO_COMMAND_STOP;
				ret_val = AL_TRUE;
			}
			else{ret_val = AL_FALSE;}
		}
		else{ret_val = AL_FALSE;}	//This should never occur
		pthread_mutex_unlock(&__audio_streamer_lock);
		return ret_val;
	}
	alSourceStop(source->src_id);
	if(audio_test_error(&error, "source stopping") == AL_TRUE){return AL_FALSE;}
	return AL_TRUE;
}

ALboolean audio_streamer_buffer_fill(ALuint id){
	ALvoid * data;
	ALCenum error;
	data = malloc(AUDIO_STREAMING_DATA_CHUNK_SIZE);

	//data array is filled with song info
	switch(__audio_streamer_info->data_type){
	case AUDIO_DATA_TYPE_WAV:
		audio_WAV_buffer_fill(data);
		break;
	// case AUDIO_DATA_TYPE_CDDA:
	// 	audio_CDDA_buffer_fill(data);
	// 	break;
	// case AUDIO_DATA_TYPE_OGG:
	// 	audio_OGG_buffer_fill(data);
	// 	break;
	default:
		free(data);
		return AL_FALSE;
	}

	alBufferData(id, __audio_streamer_info->format, data, AUDIO_STREAMING_DATA_CHUNK_SIZE, __audio_streamer_info->freq);
	free(data);

	if(audio_test_error(&error, "loading wav file") == AL_TRUE){return AL_FALSE;}
	alSourceQueueBuffers(__audio_streamer_source->src_id, 1, &id);	//last parameter is a pointer to an array
	if(audio_test_error(&error, "queue-ing buffer") == AL_TRUE){return AL_FALSE;}

	return AL_TRUE;
}

ALboolean audio_prep_stream_buffers(){
	// Fill all the buffers with audio data from the wave file
	uint8_t i;
	ALboolean res;
	for(i = 0; i < __audio_streamer_info->buff_cnt; i++){
		res = audio_streamer_buffer_fill(__audio_streamer_info->buff_id[i]);
		if(res == AL_FALSE){return AL_FALSE;}
	}

	return AL_TRUE;
}

//Need to fix returns to unalloc memory if need be
//I won't be checking the return type so it doesn't matter
void * audio_stream_player(void * args){
	(void)args;	//This only exists to make the compiler shut up

	pthread_mutex_lock(&__audio_streamer_lock);
	__audio_streamer_thd_active = 1;
	pthread_mutex_unlock(&__audio_streamer_lock);

	//Queue up all buffers for the source with the beginning of the song
	if(audio_prep_stream_buffers() == AL_FALSE){__audio_streamer_thd_active = 0; return NULL;}

	uint8_t command;

	ALint iBuffersProcessed = 0;
	ALuint uiBuffer;

	uint8_t starting = 0;
	uint8_t refresh_buffers = 0;

	// ALfloat speed, new_speed;
	// alGetSourcef(__audio_streamer_source->src_id, AL_PITCH, &speed);
	// new_speed = speed;

	//NOTE: This doesn't really account for playback speed very well
	// int sleep_time = (__audio_streamer_info->freq / AUDIO_STREAMING_DATA_CHUNK_SIZE) * 1000 / speed;

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

	while(1){
		pthread_mutex_lock(&__audio_streamer_lock);
		command = __audio_streamer_command;
		__audio_streamer_command = AUDIO_COMMAND_NONE;
		pthread_mutex_unlock(&__audio_streamer_lock);

		//If source update is false then that means the source has been removed before this thread has finished
		//Its not harmful though since if its false the statement is never entered and we exit when we check the command
			//Next two conditions basically detect when it naturally stops playing (Non-looping) and it will reset the buffers
		if(audio_update_source_state(__audio_streamer_source) == AL_TRUE &&
			__audio_streamer_source->state == AL_STOPPED && __audio_streamer_stopping == 1){

			fseek(__audio_streamer_fp, WAV_HDR_SIZE, SEEK_SET);
			refresh_buffers = 1;
			__audio_streamer_stopping = 0;
		}

		//If the user changed the playback speed, we'll update our sleep time
		// alGetSourcef(__audio_streamer_source->src_id, AL_PITCH, &new_speed);
		// if(new_speed != speed){
		// 	sleep_time = (__audio_streamer_info->freq / AUDIO_STREAMING_DATA_CHUNK_SIZE) * 1000 / speed;
		// 	speed = new_speed;
		// }

		if(command > AUDIO_COMMAND_END){command = AUDIO_COMMAND_NONE;}
		if(command == AUDIO_COMMAND_PLAY || command == AUDIO_COMMAND_UNPAUSE){
			if(__audio_streamer_source->state == AL_PLAYING){	//If we play during playing then we reset
				alSourceStop(__audio_streamer_source->src_id);
				fseek(__audio_streamer_fp, WAV_HDR_SIZE, SEEK_SET);
				refresh_buffers = 1;
				starting = 1;
			}
			else{
				alSourcePlay(__audio_streamer_source->src_id);
				//Don't need to refresh buffers since they should already be prep-d
			}
			
			// printf("STARTING\n");
		}
		else if(command == AUDIO_COMMAND_PAUSE){
			alSourcePause(__audio_streamer_source->src_id);
		}
		else if(command == AUDIO_COMMAND_STOP){
			alSourceStop(__audio_streamer_source->src_id);	//All buffers should now be unqueued unless your Nvidia driver sucks
			fseek(__audio_streamer_fp, WAV_HDR_SIZE, SEEK_SET);	//Reset to beginning
			refresh_buffers = 1;
			// printf("STOPPING\n");
		}
		else if(command == AUDIO_COMMAND_END){break;}

		// if(__audio_streamer_source->state == AL_PLAYING){printf("STATE: PLAYING\n");}
		// else if(__audio_streamer_source->state == AL_STOPPED){printf("STATE: STOPPED\n");}
		// else if(__audio_streamer_source->state == AL_PAUSED){printf("STATE: PAUSED\n");}
		// else if(__audio_streamer_source->state == AL_STREAMING){printf("STATE: STREAMING\n");}	//Never seem to see these
		// else if(__audio_streamer_source->state == AL_INITIAL){printf("STATE: INITIAL\n");}
		// else if(__audio_streamer_source->state == AL_UNDETERMINED){printf("STATE: UNDETERMINED\n");}	//Never seem to see these
		// else{printf("STATE: Unknown\n");}	//Never seem to see these

		//So we don't waste time doing stuff when stopped
			//I've never seen the second condition before
		if(__audio_streamer_source->state == AL_PLAYING || __audio_streamer_source->state == AL_STREAMING ||
			refresh_buffers){
			// printf("I AM CHECKING THE PROCESSED BUFFERS\n");

			// Buffer queuing loop must operate in a new thread
			iBuffersProcessed = 0;
			alGetSourcei(__audio_streamer_source->src_id, AL_BUFFERS_PROCESSED, &iBuffersProcessed);

			// if(iBuffersProcessed > 0){
			// 	printf("I AM ACTUALLY PROCESSING %d BUFFERS\n", iBuffersProcessed);
			// }

			// For each processed buffer, remove it from the source queue, read the next chunk of
			// audio data from the file, fill the buffer with new data, and add it to the source queue
			// But we don't read if we're currently playing, but the audio is ending
			while(iBuffersProcessed && !__audio_streamer_stopping){
				// Remove the buffer from the queue (uiBuffer contains the buffer ID for the dequeued buffer)
				//The unqueue operation will only take place if all n (1) buffers can be removed from the queue.
				uiBuffer = 0;
				alSourceUnqueueBuffers(__audio_streamer_source->src_id, 1, &uiBuffer);

				audio_streamer_buffer_fill(uiBuffer);

				iBuffersProcessed--;
			}
		}
		refresh_buffers = 0;

		if(starting){
			alSourcePlay(__audio_streamer_source->src_id);
			starting = 0;
		}

		//All of these will basically tell the thread manager that this thread is done and if any other threads are waiting then
		//we should process them
		#if defined(__APPLE__) || defined(__linux__) || defined(_arch_dreamcast)
			sched_yield();
		#endif
		#ifdef _WIN32
			Sleep(0);	// https://stackoverflow.com/questions/3727420/significance-of-sleep0
						//Might want to replace this with something else since the CPU will be at 100% if this is the only active thread
		#endif

		//Might be an issue for higher frequency audio, but right now this works
		// sleep_ms(sleep_time);
	}

	//Shutdown the system
	audio_stop_source(__audio_streamer_source);	//This will de-queue all of the queue-d buffers

	//Free the source
	alDeleteSources(1, &__audio_streamer_source->src_id);

	//Tell the world we're done
	pthread_mutex_lock(&__audio_streamer_lock);
	__audio_streamer_thd_active = 0;
	__audio_streamer_source = NULL;
	__audio_streamer_command = AUDIO_COMMAND_NONE;
	pthread_mutex_unlock(&__audio_streamer_lock);

	return 0;
}

void audio_WAV_buffer_fill(ALvoid * data){
	// WIP. INCOMPLETE
	int spare = (__audio_streamer_source->info->size + WAV_HDR_SIZE) - ftell(__audio_streamer_fp);	//This is how much data in the entire file
	int read = fread(data, 1, (AUDIO_STREAMING_DATA_CHUNK_SIZE < spare) ? AUDIO_STREAMING_DATA_CHUNK_SIZE : spare, __audio_streamer_fp);
	if(read < AUDIO_STREAMING_DATA_CHUNK_SIZE){
		fseek(__audio_streamer_fp, WAV_HDR_SIZE, SEEK_SET);	//Skips the header, beginning of body
		if(__audio_streamer_source->looping){	//Fill from beginning
			fread(&((char*)data)[read], AUDIO_STREAMING_DATA_CHUNK_SIZE - read, 1, __audio_streamer_fp);
		}
		else{	//Fill with zeroes/silence
			memset(&((char *)data)[read], 0, AUDIO_STREAMING_DATA_CHUNK_SIZE - read);
			__audio_streamer_stopping = 1;	//It will take a second before the source state goes to stopped
			//Can't reset the file pointer yet since its kinda used above
		}
	}
}

void audio_CDDA_buffer_fill(ALvoid * data){
	(ALvoid)data;	//This only exists to make the compiler shut up
	;
}

void audio_OGG_buffer_fill(ALvoid * data){
	(ALvoid)data;	//This only exists to make the compiler shut up
	;
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
	alSourcef(source->src_id, AL_GAIN, vol);
	if(audio_test_error(&error, "source gain") == AL_TRUE){return 1;}
	return 0;
}

uint8_t audio_adjust_source_speed(audio_source_t * source, float speed){
	if(speed < 0 || source == NULL){return 1;}

	ALCenum error;
	alSourcef(source->src_id, AL_PITCH, speed);
	if(audio_test_error(&error, "source pitch") == AL_TRUE){return 1;}
	return 0;
}

uint8_t audio_set_source_looping(audio_source_t * source, ALboolean looping){
	if(source == NULL){return 1;}

	ALCenum error;
	alSourcei(source->src_id, AL_LOOPING, looping);
	if(audio_test_error(&error, "source looping") == AL_TRUE){return AL_FALSE;}
	return 0;
}
