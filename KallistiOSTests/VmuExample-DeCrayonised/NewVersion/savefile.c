#include "savefile.h"

//Its a path to the folder where it will try to save to
char *__savefile_path = NULL;
uint16_t __savefile_path_length;

//Why is valid screens a part of Savefile details, but this function isn't?
void crayon_vmu_display_icon(uint8_t vmu_bitmap, void *icon){
	#ifdef _arch_dreamcast
	maple_device_t *vmu;
	vec2_s8_t port_and_slot;
	uint8_t i;
	for(i = 0; i < CRAY_SF_NUM_SAVE_DEVICES; i++){
		if(crayon_savefile_get_device_bit(vmu_bitmap, i)){	//We want to display on this VMU
			port_and_slot = crayon_savefile_dreamcast_get_port_and_slot(i);
			if(!(vmu = maple_enum_dev(port_and_slot.x, port_and_slot.y))){	//Device not present
				continue;
			}
			vmu_draw_lcd(vmu, icon);
		}
	}
	#endif

	return;
}

#if defined(_arch_dreamcast)

//Returns true if device has certain function/s
uint8_t crayon_savefile_check_device_for_function(uint32_t function, int8_t save_device_id){
	maple_device_t *vmu;

	vec2_s8_t port_and_slot = crayon_savefile_dreamcast_get_port_and_slot(save_device_id);

	//Invalid controller/port
	if(port_and_slot.x < 0){
		return 1;
	}

	//Make sure there's a device in the port/slot
	if(!(vmu = maple_enum_dev(port_and_slot.x, port_and_slot.y))){
		return 1;
	}

	//Check the device is valid and it has a certain function
	if(!vmu->valid || !(vmu->info.functions & function)){
		return 1;
	}

	return 0;
}

#endif

uint8_t crayon_savefile_dreamcast_get_screen_bitmap(){
	#if defined(_arch_dreamcast)

	uint8_t screens = 0;	//a1a2b1b2c1c2d1d2

	int i;
	for(i = 0; i < CRAY_SF_NUM_SAVE_DEVICES; i++){
		//Check if device contains this function bitmap (Returns 0 on success)
		if(!crayon_savefile_check_device_for_function(MAPLE_FUNC_LCD, i)){
			crayon_savefile_set_device_bit(&screens, i);
		}
		
	}
	return valid_function;

	#else

	return 0;

	#endif
}

//Dreamcast: Rounds the number down to nearest multiple of 512 , then adds 1 if there's a remainder
//PC: Returns 0 (Function isn't needed on PC anyways)
uint16_t crayon_savefile_bytes_to_blocks(uint32_t bytes){
	return (bytes >> 9) + !!(bytes & ((1 << 9) - 1));
}

//I used the "vmu_pkg_build" function's source as a guide for this. Doesn't work with compressed saves
uint32_t crayon_savefile_get_savefile_size(crayon_savefile_details_t *details){
	#ifdef _arch_dreamcast

	// uint16_t eyecatcher_size = vmu_eyecatch_size();
	// if(eyecatcher_size < 0){return 0;}

	uint16_t eyecatcher_size = 0;
	switch(details->eyecatcher_type){
		case VMUPKG_EC_NONE:
			eyecatcher_size = 0; break;
		case VMUPKG_EC_16BIT:
			eyecatcher_size = 8064; break;
		case VMUPKG_EC_256COL:
			eyecatcher_size = 512 + 4032; break;
		case VMUPKG_EC_16COL:
			eyecatcher_size = 32 + 2016; break;
		default:
			return 0;
	}

	//Get the total number of bytes. Keep in mind we need to think about the icon/s and EC
	return CRAY_SF_HDR_SIZE + (512 * details->icon_anim_count) + eyecatcher_size + details->savedata_size;

	#else

	return CRAY_SF_HDR_SIZE + details->savedata_size;

	#endif
}

uint16_t crayon_savefile_detail_string_length(uint8_t string_id){
	switch(string_id){
		case CRAY_SF_STRING_FILENAME:
			#if defined(_arch_pc)

			return 256;
		
			#elif defined(_arch_dreamcast)
		
			return 21 - 8;	//The 8 is the "/vmu/XX/" and the name itself can only be 13 chars (Last it null terminator)
		
			#endif
		case CRAY_SF_STRING_APP_ID:
			return 16;

		#if defined(_arch_dreamcast)
		
		case CRAY_SF_STRING_SHORT_DESC:
			return 16;
		case CRAY_SF_STRING_LONG_DESC:
			return 32;
		
		#endif

		default:
			return 0;
	}
}

void crayon_savefile_serialise(crayon_savefile_details_t *details, uint8_t *buffer){
	crayon_savefile_data_t data = details->savedata;

	// uint8_t *buffer_old = buffer;

	//Encode the version number
	crayon_misc_encode_to_buffer(buffer, (uint8_t*)&details->latest_version, sizeof(crayon_savefile_version_t));
	buffer += sizeof(crayon_savefile_version_t);

	//Next encode all doubles, then floats, then unsigned 32-bit ints, then signed 32-bit ints
	//Then 16-bit ints, 8-bit ints and finally the characters. No need to encode the lengths
	//since the history can tell us that
	crayon_misc_encode_to_buffer(buffer, (uint8_t*)data.doubles, sizeof(double) * data.lengths[CRAY_TYPE_DOUBLE]);
	buffer += (sizeof(double) * data.lengths[CRAY_TYPE_DOUBLE]);

	crayon_misc_encode_to_buffer(buffer, (uint8_t*)data.floats, sizeof(float) * data.lengths[CRAY_TYPE_FLOAT]);
	buffer += (sizeof(float) * data.lengths[CRAY_TYPE_FLOAT]);

	crayon_misc_encode_to_buffer(buffer, (uint8_t*)data.u32, sizeof(uint32_t) * data.lengths[CRAY_TYPE_UINT32]);
	buffer += (sizeof(uint32_t) * data.lengths[CRAY_TYPE_UINT32]);

	crayon_misc_encode_to_buffer(buffer, (uint8_t*)data.s32, sizeof(int32_t) * data.lengths[CRAY_TYPE_SINT32]);
	buffer += (sizeof(int32_t) * data.lengths[CRAY_TYPE_SINT32]);

	crayon_misc_encode_to_buffer(buffer, (uint8_t*)data.u16, sizeof(uint16_t) * data.lengths[CRAY_TYPE_UINT16]);
	buffer += (sizeof(uint16_t) * data.lengths[CRAY_TYPE_UINT16]);

	crayon_misc_encode_to_buffer(buffer, (uint8_t*)data.s16, sizeof(int16_t) * data.lengths[CRAY_TYPE_SINT16]);
	buffer += (sizeof(int16_t) * data.lengths[CRAY_TYPE_SINT16]);

	crayon_misc_encode_to_buffer(buffer, data.u8, sizeof(uint8_t) * data.lengths[CRAY_TYPE_UINT8]);
	buffer += (sizeof(uint8_t) * data.lengths[CRAY_TYPE_UINT8]);

	crayon_misc_encode_to_buffer(buffer, (uint8_t*)data.s8, sizeof(int8_t) * data.lengths[CRAY_TYPE_SINT8]);
	buffer += (sizeof(int8_t) * data.lengths[CRAY_TYPE_SINT8]);

	crayon_misc_encode_to_buffer(buffer, (uint8_t*)data.chars, sizeof(char) * data.lengths[CRAY_TYPE_CHAR]);
	buffer += (sizeof(char) * data.lengths[CRAY_TYPE_CHAR]);

	// printf("Size %d. Size we should have %d\n", (int)(buffer - buffer_old), details->savedata_size);

	return;
}

//Assume the buffer has the correct endian-ness going into this
	//WARNING. Data_length currently unchecked.
uint8_t crayon_savefile_deserialise(crayon_savefile_details_t *details, uint8_t *data, uint32_t data_length){
	//Same as serialiser, but instead we extract the variables and version from the buffer
	crayon_savefile_data_t *new_savedata = &details->savedata;

	//Decode the version number
	crayon_savefile_version_t loaded_version;
	crayon_misc_encode_to_buffer((uint8_t*)&loaded_version, data, sizeof(crayon_savefile_version_t));
	data += sizeof(crayon_savefile_version_t);
	data_length -= sizeof(crayon_savefile_version_t);

	//We'll also need to check if the version number is valid
	if(loaded_version > details->latest_version){
		printf("TEST\n");	
		return 1;
	}

	// printf("loaded_version %d\n", loaded_version);

	//If its an older savefile, put it in the crayon_savefile_data_t format, make that union pointer array
	//and call the user's upgrade function
	if(loaded_version < details->latest_version){
		// printf("Backwards compatibility currently unsupported\n");
		// return 1;

		crayon_savefile_data_t old_savedata;

		//Sets all the lengths to zero and the pointers to null
		memset(&old_savedata, 0, sizeof(crayon_savefile_data_t));

		//Need to use history to find the lengths of all 9 arrays
		crayon_savefile_history_t *curr = details->history;
		while(curr != NULL){
			//If the variable currently exists, add it to out length
			if(loaded_version >= curr->version_added && loaded_version < curr->version_removed){
				old_savedata.lengths[curr->data_type] += curr->data_length;
			}

			curr = curr->next;
		}

		uint32_t expected_size = (old_savedata.lengths[CRAY_TYPE_DOUBLE] * sizeof(double)) +
			(old_savedata.lengths[CRAY_TYPE_FLOAT] * sizeof(float)) +
			(old_savedata.lengths[CRAY_TYPE_UINT32] * sizeof(uint32_t)) +
			(old_savedata.lengths[CRAY_TYPE_SINT32] * sizeof(int32_t)) +
			(old_savedata.lengths[CRAY_TYPE_UINT16] * sizeof(uint16_t)) +
			(old_savedata.lengths[CRAY_TYPE_SINT16] * sizeof(int16_t)) +
			(old_savedata.lengths[CRAY_TYPE_UINT8] * sizeof(uint8_t)) +
			(old_savedata.lengths[CRAY_TYPE_SINT8] * sizeof(int8_t)) +
			(old_savedata.lengths[CRAY_TYPE_CHAR] * sizeof(char));

		//The size is off, the savefile must have been tampered with
		if(expected_size != data_length){
			printf("%d %d\n", expected_size, data_length);
			return 1;
		}

		//Allocate space for our arrays
		old_savedata.doubles = malloc(sizeof(double) * old_savedata.lengths[CRAY_TYPE_DOUBLE]);
		old_savedata.floats = malloc(sizeof(float) * old_savedata.lengths[CRAY_TYPE_FLOAT]);
		old_savedata.u32 = malloc(sizeof(uint32_t) * old_savedata.lengths[CRAY_TYPE_UINT32]);
		old_savedata.s32 = malloc(sizeof(int32_t) * old_savedata.lengths[CRAY_TYPE_SINT32]);
		old_savedata.u16 = malloc(sizeof(uint16_t) * old_savedata.lengths[CRAY_TYPE_UINT16]);
		old_savedata.s16 = malloc(sizeof(int16_t) * old_savedata.lengths[CRAY_TYPE_SINT16]);
		old_savedata.u8 = malloc(sizeof(uint8_t) * old_savedata.lengths[CRAY_TYPE_UINT8]);
		old_savedata.s8 = malloc(sizeof(int8_t) * old_savedata.lengths[CRAY_TYPE_SINT8]);
		old_savedata.chars = malloc(sizeof(char) * old_savedata.lengths[CRAY_TYPE_CHAR]);

		crayon_savefile_old_variable_t *array = malloc(sizeof(crayon_savefile_old_variable_t) *
			details->num_vars);

		//Check if any of those mallocs failed, if so terminate
		if(!old_savedata.doubles || !old_savedata.floats || !old_savedata.u32 || !old_savedata.s32 ||
			!old_savedata.u16 || !old_savedata.s16 || !old_savedata.u8 || !old_savedata.s8 ||
			!old_savedata.chars || !array){
			crayon_savefile_free_savedata(&old_savedata);

			if(array){free(array);}

			return 1;
		}

		//Convert the buffer into our savedata struct
		crayon_savefile_buffer_to_savedata(&old_savedata, data);

		//Go through the history again, but now set the pointers for each variable. Note variables that DNE in
		//This version should be set to NULL
		//Also copy over the common vars from the old savedata to the new one
		curr = details->history;
		uint32_t i, index = 0;
		uint32_t pointers[CRAY_NUM_TYPES] = {0};
		while(curr != NULL){
			//If the variable currently exists, assign its pointer
			if(loaded_version >= curr->version_added && loaded_version < curr->version_removed){
				switch(curr->data_type){
					case CRAY_TYPE_DOUBLE:
						array[index].t_double = old_savedata.doubles + pointers[curr->data_type];
						for(i = 0; i < curr->data_length; i++){
							(*curr->data_ptr.t_double)[i] = array[index].t_double[i];
						}
						break;
					case CRAY_TYPE_FLOAT:
						array[index].t_float = old_savedata.floats + pointers[curr->data_type];
						for(i = 0; i < curr->data_length; i++){
							(*curr->data_ptr.t_float)[i] = array[index].t_float[i];
						}
						break;
					case CRAY_TYPE_UINT32:
						array[index].t_u32 = old_savedata.u32 + pointers[curr->data_type];
						for(i = 0; i < curr->data_length; i++){
							(*curr->data_ptr.t_u32)[i] = array[index].t_u32[i];
						}
						break;
					case CRAY_TYPE_SINT32:
						array[index].t_s32 = old_savedata.s32 + pointers[curr->data_type];
						for(i = 0; i < curr->data_length; i++){
							(*curr->data_ptr.t_s32)[i] = array[index].t_s32[i];
						}
						break;
					case CRAY_TYPE_UINT16:
						array[index].t_u16 = old_savedata.u16 + pointers[curr->data_type];
						for(i = 0; i < curr->data_length; i++){
							(*curr->data_ptr.t_u16)[i] = array[index].t_u16[i];
						}
						break;
					case CRAY_TYPE_SINT16:
						array[index].t_s16 = old_savedata.s16 + pointers[curr->data_type];
						for(i = 0; i < curr->data_length; i++){
							(*curr->data_ptr.t_s16)[i] = array[index].t_s16[i];
						}
						break;
					case CRAY_TYPE_UINT8:
						array[index].t_u8 = old_savedata.u8 + pointers[curr->data_type];
						for(i = 0; i < curr->data_length; i++){
							(*curr->data_ptr.t_u8)[i] = array[index].t_u8[i];
						}
						break;
					case CRAY_TYPE_SINT8:
						array[index].t_s8 = old_savedata.s8 + pointers[curr->data_type];
						for(i = 0; i < curr->data_length; i++){
							(*curr->data_ptr.t_s8)[i] = array[index].t_s8[i];
						}
						break;
					case CRAY_TYPE_CHAR:
						array[index].t_char = old_savedata.chars + pointers[curr->data_type];
						for(i = 0; i < curr->data_length; i++){
							(*curr->data_ptr.t_char)[i] = array[index].t_char[i];
						}
						break;
				}
				pointers[curr->data_type] += curr->data_length;
			}
			else{	//Set the pointer to NULL if it doesn't exist in the old savefile
				memset(&array[index], 0, sizeof(array[index]));
			}

			//Set the type
			array[index].data_type = curr->data_type;

			index++;
			curr = curr->next;
		}

		//Call the user function to handle old to new savedata transfers
		(*details->update_savefile_func)(array, loaded_version, details->latest_version);

		printf("OLD v%d\n", loaded_version);
		__crayon_savefile_print_savedata(&old_savedata);
		printf("\n");

		//Free the union array
		free(array);

		//Free the old savedata arrays too
		crayon_savefile_free_savedata(&old_savedata);
	}
	else{	//If same version
		//The size is wrong, savefile has been tampered with
		if(details->savedata_size - sizeof(crayon_savefile_version_t) != data_length){
			return 1;
		}

		crayon_savefile_buffer_to_savedata(new_savedata, data);
	}

	printf("NEW v%d\n", details->latest_version);
	__crayon_savefile_print_savedata(new_savedata);
	printf("\n");

	return 0;
}

uint32_t crayon_savefile_get_device_free_space(int8_t device_id){
	#if defined(_arch_dreamcast)
	
	vec2_s8_t port_and_slot = crayon_savefile_dreamcast_get_port_and_slot(device_id);

	maple_device_t *vmu;
	vmu = maple_enum_dev(port_and_slot.x, port_and_slot.y);
	return vmufs_free_blocks(vmu) * 512;
	
	#else

	//Yeah, idk how to get system's remaining space size in C
	return INT32_MAX;
	
	#endif
}

//It will construct the full string for you
char *crayon_savefile_get_full_path(crayon_savefile_details_t *details, int8_t save_device_id){
	if(save_device_id < 0 || save_device_id >= CRAY_SF_NUM_SAVE_DEVICES){
		printf("Test\n");
		return NULL;
	}

	#if defined(_arch_dreamcast)

	//The 3 comes from the "a0/" part
	uint32_t length = __savefile_path_length + 3 + strlen(details->strings[CRAY_SF_STRING_FILENAME]) + 1;
	
	#elif defined(_arch_pc)

	uint32_t length = __savefile_path_length + strlen(details->strings[CRAY_SF_STRING_FILENAME]) + 1;

	#endif

	char *path;
	if(!(path = malloc(length * sizeof(char)))){
		return NULL;
	}

	strcpy(path, __savefile_path);

	#if defined(_arch_dreamcast)

	vec2_s8_t port_and_slot = crayon_savefile_dreamcast_get_port_and_slot(save_device_id);
	if(port_and_slot.x == -1){	//Due to first return NULL, this should never trigger
		free(path);
		return NULL;
	}

	uint32_t curr_length = strlen(path);
	path[curr_length    ] = port_and_slot.x + 'a';
	path[curr_length + 1] = port_and_slot.y + '0';
	path[curr_length + 2] = '/';
	path[curr_length + 3] = '\0';

	#endif

	strcat(path, details->strings[CRAY_SF_STRING_FILENAME]);

	return path;
}

//NOTE: You should never need to access these variables directly. I'm only doing so for debugging purposes
void __crayon_savefile_print_savedata(crayon_savefile_data_t *savedata){
	#if defined(_arch_pc)

	uint32_t i;
	printf("(START)\n\n");

	printf("LENGTHS\n");
	for(i = 0; i < CRAY_NUM_TYPES; i++){
		printf("%d, ", savedata->lengths[i]);
	}
	printf("\n");

	printf("Double\n");
	for(i = 0; i < savedata->lengths[CRAY_TYPE_DOUBLE]; i++){
		printf("%lf, ", savedata->doubles[i]);
	}
	printf("\n");

	printf("Float\n");
	for(i = 0; i < savedata->lengths[CRAY_TYPE_FLOAT]; i++){
		printf("%f, ", savedata->floats[i]);
	}
	printf("\n");

	printf("u32\n");
	for(i = 0; i < savedata->lengths[CRAY_TYPE_UINT32]; i++){
		printf("%d, ", savedata->u32[i]);
	}
	printf("\n");

	printf("s32\n");
	for(i = 0; i < savedata->lengths[CRAY_TYPE_SINT32]; i++){
		printf("%d, ", savedata->s32[i]);
	}
	printf("\n");

	printf("u16\n");
	for(i = 0; i < savedata->lengths[CRAY_TYPE_UINT16]; i++){
		printf("%d, ", savedata->u16[i]);
	}
	printf("\n");

	printf("s16\n");
	for(i = 0; i < savedata->lengths[CRAY_TYPE_SINT16]; i++){
		printf("%d, ", savedata->s16[i]);
	}
	printf("\n");

	printf("u8\n");
	for(i = 0; i < savedata->lengths[CRAY_TYPE_UINT8]; i++){
		printf("%d, ", savedata->u8[i]);
	}
	printf("\n");

	printf("s8\n");
	for(i = 0; i < savedata->lengths[CRAY_TYPE_SINT8]; i++){
		printf("%d, ", savedata->s8[i]);
	}
	printf("\n");

	printf("Chars\n");
	for(i = 0; i < savedata->lengths[CRAY_TYPE_CHAR]; i++){
		printf("%c", savedata->chars[i]);
	}
	printf("\n(END)\n");

	#endif

	return;
}

void crayon_savefile_buffer_to_savedata(crayon_savefile_data_t *data, uint8_t *buffer){
	crayon_misc_encode_to_buffer((uint8_t*)data->doubles, buffer, sizeof(double) * data->lengths[CRAY_TYPE_DOUBLE]);
	buffer += sizeof(double) * data->lengths[CRAY_TYPE_DOUBLE];

	crayon_misc_encode_to_buffer((uint8_t*)data->floats, buffer, sizeof(float) * data->lengths[CRAY_TYPE_FLOAT]);
	buffer += sizeof(float) * data->lengths[CRAY_TYPE_FLOAT];

	crayon_misc_encode_to_buffer((uint8_t*)data->u32, buffer, sizeof(uint32_t) * data->lengths[CRAY_TYPE_UINT32]);
	buffer += sizeof(uint32_t) * data->lengths[CRAY_TYPE_UINT32];

	crayon_misc_encode_to_buffer((uint8_t*)data->s32, buffer, sizeof(int32_t) * data->lengths[CRAY_TYPE_SINT32]);
	buffer += sizeof(int32_t) * data->lengths[CRAY_TYPE_SINT32];

	crayon_misc_encode_to_buffer((uint8_t*)data->u16, buffer, sizeof(uint16_t) * data->lengths[CRAY_TYPE_UINT16]);
	buffer += sizeof(uint16_t) * data->lengths[CRAY_TYPE_UINT16];

	crayon_misc_encode_to_buffer((uint8_t*)data->s16, buffer, sizeof(int16_t) * data->lengths[CRAY_TYPE_SINT16]);
	buffer += sizeof(int16_t) * data->lengths[CRAY_TYPE_SINT16];

	crayon_misc_encode_to_buffer(data->u8, buffer, sizeof(uint8_t) * data->lengths[CRAY_TYPE_UINT8]);
	buffer += sizeof(uint8_t) * data->lengths[CRAY_TYPE_UINT8];

	crayon_misc_encode_to_buffer((uint8_t*)data->s8, buffer, sizeof(int8_t) * data->lengths[CRAY_TYPE_SINT8]);
	buffer += sizeof(int8_t) * data->lengths[CRAY_TYPE_SINT8];

	crayon_misc_encode_to_buffer((uint8_t*)data->chars, buffer, sizeof(char) * data->lengths[CRAY_TYPE_CHAR]);
	// buffer += sizeof(char) * data->lengths[CRAY_TYPE_CHAR];

	return;
}

uint8_t crayon_savefile_set_string(crayon_savefile_details_t *details, const char *string, uint8_t string_id){
	uint16_t max_length = crayon_savefile_detail_string_length(string_id);
	uint16_t string_length = strlen(string);
	if(max_length == 0 || string_length >= max_length){return 1;}

	strncpy(details->strings[string_id], string, string_length + 1);
	details->strings[string_id][max_length - 1] = '\0';

	return 0;
}

uint8_t crayon_savefile_get_device_bit(uint8_t device_bitmap, uint8_t save_device_id){
	return (device_bitmap >> save_device_id) & 1;
}

void crayon_savefile_set_device_bit(uint8_t *device_bitmap, uint8_t save_device_id){
	*device_bitmap |= (1 << save_device_id);
	return;
}

//THIS CAN BE OPTIMISED
uint8_t crayon_savefile_check_savedata(crayon_savefile_details_t *details, int8_t save_device_id){
	char *savename = crayon_savefile_get_full_path(details, save_device_id);
	if(!savename){
		printf("FAILED AT 1\n");
		return 1;
	}

	// printf("PATH: %s\n", savename);

	FILE *fp = fopen(savename, "rb");
	free(savename);
	//File DNE
	if(!fp){
		printf("FAILED AT 2\n");
		return 2;
	}

	#if defined( _arch_dreamcast)

	fseek(fp, 0, SEEK_END);
	int pkg_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	//Surely instead of doing the below I can just read the header and hence the app id/version?
	;

	uint8_t *pkg_out = malloc(pkg_size);
	fread(pkg_out, pkg_size, 1, fp);
	fclose(fp);

	vmu_pkg_t pkg;
	if(vmu_pkg_parse(pkg_out, &pkg)){	//CRC is incorrect
		printf("FAILED AT 3\n");
		free(pkg_out);
		return 3;
	}

	free(pkg_out);

	//If the version IDs don't match, then can can't comprehend that savefile
	if(strcmp(pkg.app_id, details->strings[CRAY_SF_STRING_APP_ID])){
		printf("FAILED AT 4\n");
		return 4;
	}

	//Check to confirm the savefile version is not newer than it should be
	crayon_savefile_version_t sf_version;
	crayon_misc_encode_to_buffer((uint8_t*)&sf_version, (uint8_t*)pkg.data, sizeof(crayon_savefile_version_t));

	#else

	crayon_savefile_hdr_t hdr;

	fread(&hdr, 1, sizeof(hdr), fp);

	crayon_savefile_version_t sf_version;
	fread(&sf_version, 4, 1, fp);
	fclose(fp);

	//Pass that sf_version (And maybe "hdr.app_id") through our endian-ness function
	;

	//Either it has the wrong size or the app ids somehow don't match
	//(The later should never trigger if you use this library right)
	if(strcmp(hdr.app_id, details->strings[CRAY_SF_STRING_APP_ID])){
		printf("FAILED AT 4\n");
		return 4;
	}

	// printf("read version %d. our version %d\n", sf_version, details->version);

	#endif

	details->savefile_versions[save_device_id] = sf_version;

	//Check to confirm the savefile version is not newer than it should be
	// if(sf_version > details->latest_version){
	// 	printf("FAILED AT 5\n");
	// 	return 5;
	// }


	return 0;
}

uint8_t crayon_savefile_set_path(char *path){
	if(__savefile_path){
		free(__savefile_path);
	}

	#ifdef _arch_dreamcast

	(void)path;

	if(!(__savefile_path = malloc(6))){	// "/vmu/"
		return 1;
	}

	strcpy(__savefile_path, "/vmu/");

	#else

	uint16_t length = strlen(path);

	if(!(__savefile_path = malloc(length + 1))){
		return 1;
	}

	strcpy(__savefile_path, path);

	#endif

	__savefile_path_length = strlen(__savefile_path);
	return 0;
}

//Make sure to call this first (And call the save icon and eyecatcher functions after since this overides them)
uint8_t crayon_savefile_init_savefile_details(crayon_savefile_details_t *details, const char *save_name,
	crayon_savefile_version_t latest_version, void (*default_values_func)(),
	uint8_t (*update_savefile_func)(crayon_savefile_old_variable_t*, crayon_savefile_version_t,
	crayon_savefile_version_t)){

	uint16_t save_name_length = strlen(save_name);

	details->latest_version = latest_version;

	//Could probably replace this with a memset zero
	details->savedata.doubles = NULL;
	details->savedata.floats = NULL;
	details->savedata.u32 = NULL;
	details->savedata.s32 = NULL;
	details->savedata.u16 = NULL;
	details->savedata.s16 = NULL;
	details->savedata.u8 = NULL;
	details->savedata.s8 = NULL;
	details->savedata.chars = NULL;
	uint8_t i;
	for(i = 0; i < CRAY_NUM_TYPES; i++){
		details->savedata.lengths[i] = 0;
	}

	for(i = 0; i < CRAY_SF_NUM_SAVE_DEVICES; i++){
		details->savefile_versions[i] = 0;
	}

	details->savedata_size = 0;	//For now

	#ifdef _arch_dreamcast

	details->icon_data = NULL;
	details->icon_palette = NULL;
	details->icon_anim_count = 0;

	details->eyecatcher_data = NULL;
	details->eyecatcher_type = VMUPKG_EC_NONE;	//The default
	
	#endif

	uint16_t str_lengths[CRAY_SF_NUM_DETAIL_STRINGS];
	for(i = 0; i < CRAY_SF_NUM_DETAIL_STRINGS; i++){
		details->strings[i] = NULL;
		str_lengths[i] = crayon_savefile_detail_string_length(i);
		details->strings[i] = malloc(sizeof(char) * str_lengths[i]);
	}

	for(i = 0; i < CRAY_SF_NUM_DETAIL_STRINGS; i++){
		if(!details->strings[i]){
			//Need to properly un-allocate stuff
			uint8_t j;
			for(j = 0; j < CRAY_SF_NUM_DETAIL_STRINGS; j++){
				if(details->strings[j]){free(details->strings[j]);}
			}

			return 1;
		}
	}

	//Given string is too big (Plus 1 for null terminator)
	if(save_name_length + 1 > str_lengths[CRAY_SF_STRING_FILENAME]){
		for(i = 0; i < CRAY_SF_NUM_DETAIL_STRINGS; i++){
			if(details->strings[i]){free(details->strings[i]);}
		}
		return 1;
	}

	//Copy the savename here
	strncpy(details->strings[CRAY_SF_STRING_FILENAME], save_name, str_lengths[CRAY_SF_STRING_FILENAME]);

	//Update the VMU screen bitmap (Dreamcast only)
	details->valid_vmu_screens = crayon_savefile_dreamcast_get_screen_bitmap();

	details->save_device_id = -1;

	details->history = NULL;
	details->history_tail = NULL;
	details->num_vars = 0;

	details->default_values_func = default_values_func;
	details->update_savefile_func = update_savefile_func;

	return 0;
}

uint8_t crayon_savefile_add_icon(crayon_savefile_details_t *details, const char *image, const char *palette,
	uint8_t icon_anim_count, uint16_t icon_anim_speed){
	#ifdef _arch_dreamcast

	FILE *icon_data_file;
	int size_data;

	if(icon_anim_count > 3){
		return 1;
	}

	if(!(icon_data_file = fopen(image, "rb"))){
		return 1;
	}

	//Get the size of the file
	fseek(icon_data_file, 0, SEEK_END);
	size_data = ftell(icon_data_file);	//This will account for multi-frame icons
	fseek(icon_data_file, 0, SEEK_SET);

	if(!(details->icon_data = malloc(size_data))){
		fclose(icon_data_file);
		return 1;
	}

	fread(details->icon_data, size_data, 1, icon_data_file);
	fclose(icon_data_file);


	//--------------------------------

	FILE *icon_palette_file;
	int size_palette;

	if(!(icon_palette_file = fopen(palette, "rb"))){
		return 1;
	}

	fseek(icon_palette_file, 0, SEEK_END);
	size_palette = ftell(icon_palette_file);
	fseek(icon_palette_file, 0, SEEK_SET);

	if(!(details->icon_palette = malloc(size_palette))){
		fclose(icon_palette_file);
		return 1;
	}

	fread(details->icon_palette, size_palette, 1, icon_palette_file);
	fclose(icon_palette_file);

	details->icon_anim_count = icon_anim_count;
	details->icon_anim_speed = icon_anim_speed;

	#endif

	return 0;
}

uint8_t crayon_savefile_add_eyecatcher(crayon_savefile_details_t *details, const char *eyecatcher_path){
	#ifdef _arch_dreamcast

	FILE *eyecatcher_data_file = fopen(eyecatcher_path, "rb");
	if(!eyecatcher_data_file){
		return 1;
	}

	//Get the size of the file
	fseek(eyecatcher_data_file, 0, SEEK_END);
	int size_data = ftell(eyecatcher_data_file);	//Confirming its the right size
	fseek(eyecatcher_data_file, 0, SEEK_SET);

	switch(size_data){
		case 8064:
			details->eyecatcher_type = VMUPKG_EC_16BIT; break;
		case 4544:
			details->eyecatcher_type = VMUPKG_EC_256COL; break;
		case 2048:
			details->eyecatcher_type = VMUPKG_EC_16COL; break;
		default:
			fclose(eyecatcher_data_file);
			return 1;
	}

	if(!(details->eyecatcher_data = malloc(size_data))){
		fclose(eyecatcher_data_file);
		return 1;
	}

	uint8_t result = (fread(details->eyecatcher_data, size_data, 1, eyecatcher_data_file) != 1);
	fclose(eyecatcher_data_file);
	
	if(result){
		return 1;
	}

	#endif

	return 0;
}

int32_t crayon_savefile_add_variable(crayon_savefile_details_t *details, void *data_ptr, uint8_t data_type, 
	uint32_t length, crayon_savefile_version_t version_added, crayon_savefile_version_t version_removed){

	if(details->num_vars < 0){
		printf("You managed to make 2,147,483,648 unique variables\nI think you're doing something wrong\n");
		return -1;
	}

	// printf("id is: %d\n", details->num_vars);

	//data_type id doesn't map to any of our types
	if(data_type >= CRAY_NUM_TYPES){
		return -1;
	}

	crayon_savefile_history_t *var = malloc(sizeof(crayon_savefile_history_t));
	if(!var){return -1;}

	//Add the new variable
	var->next = NULL;
	if(details->history_tail){	//Non-empty linked list
		details->history_tail->next = var;
		details->history_tail = var;
	}
	else{	//Empty linked list
		details->history = var;
		details->history_tail = var;
	}

	var->id = details->num_vars;

	var->data_type = data_type;
	var->data_length = length;
	var->version_added = version_added;
	var->version_removed = version_removed;

	details->savedata.lengths[var->data_type] += var->data_length;

	//Store a pointer to the type and the default value
	switch(var->data_type){
		case CRAY_TYPE_DOUBLE:
			var->data_ptr.t_double = (double**)data_ptr;
			break;
		case CRAY_TYPE_FLOAT:
			var->data_ptr.t_float = (float**)data_ptr;
			break;
		case CRAY_TYPE_UINT32:
			var->data_ptr.t_u32 = (uint32_t**)data_ptr;
			break;
		case CRAY_TYPE_SINT32:
			var->data_ptr.t_s32 = (int32_t**)data_ptr;
			break;
		case CRAY_TYPE_UINT16:
			var->data_ptr.t_u16 = (uint16_t**)data_ptr;
			break;
		case CRAY_TYPE_SINT16:
			var->data_ptr.t_s16 = (int16_t**)data_ptr;
			break;
		case CRAY_TYPE_UINT8:
			var->data_ptr.t_u8 = (uint8_t**)data_ptr;
			break;
		case CRAY_TYPE_SINT8:
			var->data_ptr.t_s8 = (int8_t**)data_ptr;
			break;
		case CRAY_TYPE_CHAR:
			var->data_ptr.t_char = (char**)data_ptr;
			break;
	}

	return details->num_vars++;
}

void *crayon_savefile_get_variable_ptr(crayon_savefile_old_variable_t *array, uint32_t index){
	switch(array[index].data_type){
		case CRAY_TYPE_DOUBLE:
			return array[index].t_double;
		case CRAY_TYPE_FLOAT:
			return array[index].t_float;
		case CRAY_TYPE_UINT32:
			return array[index].t_u32;
		case CRAY_TYPE_SINT32:
			return array[index].t_s32;
		case CRAY_TYPE_UINT16:
			return array[index].t_u16;
		case CRAY_TYPE_SINT16:
			return array[index].t_s16;
		case CRAY_TYPE_UINT8:
			return array[index].t_u8;
		case CRAY_TYPE_SINT8:
			return array[index].t_s8;
		case CRAY_TYPE_CHAR:
			return array[index].t_char;
		default:
			return NULL;
	}
}

uint8_t crayon_savefile_solidify(crayon_savefile_details_t *details){
	uint32_t *lengths = details->savedata.lengths;
	uint32_t indexes[CRAY_NUM_TYPES] = {0};

	uint8_t error = 0;

	//Don't both allocating space for these if we aren't using them
	if(lengths[CRAY_TYPE_DOUBLE]){
		details->savedata.doubles = malloc(sizeof(double) * lengths[CRAY_TYPE_DOUBLE]);
		if(!details->savedata.doubles){error = 1;}
	}
	if(lengths[CRAY_TYPE_FLOAT]){
		details->savedata.floats = malloc(sizeof(float) * lengths[CRAY_TYPE_FLOAT]);
		if(!details->savedata.floats){error = 1;}
	}
	if(lengths[CRAY_TYPE_UINT32]){
		details->savedata.u32 = malloc(sizeof(uint32_t) * lengths[CRAY_TYPE_UINT32]);
		if(!details->savedata.u32){error = 1;}
	}
	if(lengths[CRAY_TYPE_SINT32]){
		details->savedata.s32 = malloc(sizeof(int32_t) * lengths[CRAY_TYPE_SINT32]);
		if(!details->savedata.s32){error = 1;}
	}
	if(lengths[CRAY_TYPE_UINT16]){
		details->savedata.u16 = malloc(sizeof(uint16_t) * lengths[CRAY_TYPE_UINT16]);
		if(!details->savedata.u16){error = 1;}
	}
	if(lengths[CRAY_TYPE_SINT16]){
		details->savedata.s16 = malloc(sizeof(int16_t) * lengths[CRAY_TYPE_SINT16]);
		if(!details->savedata.s16){error = 1;}
	}
	if(lengths[CRAY_TYPE_UINT8]){
		details->savedata.u8 = malloc(sizeof(uint8_t) * lengths[CRAY_TYPE_UINT8]);
		if(!details->savedata.u8){error = 1;}
	}
	if(lengths[CRAY_TYPE_SINT8]){
		details->savedata.s8 = malloc(sizeof(int8_t) * lengths[CRAY_TYPE_SINT8]);
		if(!details->savedata.s8){error = 1;}
	}
	if(lengths[CRAY_TYPE_CHAR]){
		details->savedata.chars = malloc(sizeof(char) * lengths[CRAY_TYPE_CHAR]);
		if(!details->savedata.chars){error = 1;}
	}

	if(error){
		crayon_savefile_free_savedata(&details->savedata);	//Note this also resets the lengths
		return 1;
	}

	uint32_t i;
	crayon_savefile_history_t *var = details->history;
	while(var){
		if(var->version_removed > details->latest_version){	//We only give space to vars that still exist
			switch(var->data_type){
				case CRAY_TYPE_DOUBLE:
					*var->data_ptr.t_double = &details->savedata.doubles[indexes[var->data_type]];
					break;
				case CRAY_TYPE_FLOAT:
					*var->data_ptr.t_float = &details->savedata.floats[indexes[var->data_type]];
					break;
				case CRAY_TYPE_UINT32:
					*var->data_ptr.t_u32 = &details->savedata.u32[indexes[var->data_type]];
					break;
				case CRAY_TYPE_SINT32:
					*var->data_ptr.t_s32 = &details->savedata.s32[indexes[var->data_type]];
					break;
				case CRAY_TYPE_UINT16:
					*var->data_ptr.t_u16 = &details->savedata.u16[indexes[var->data_type]];
					break;
				case CRAY_TYPE_SINT16:
					*var->data_ptr.t_s16 = &details->savedata.s16[indexes[var->data_type]];
					break;
				case CRAY_TYPE_UINT8:
					*var->data_ptr.t_u8 = &details->savedata.u8[indexes[var->data_type]];
					break;
				case CRAY_TYPE_SINT8:
					*var->data_ptr.t_s8 = &details->savedata.s8[indexes[var->data_type]];
					break;
				case CRAY_TYPE_CHAR:
					*var->data_ptr.t_char = &details->savedata.chars[indexes[var->data_type]];
				break;
			}
			indexes[var->data_type] += var->data_length;
		}
		var = var->next;
	}

	//Set the default values with the user's function
	(*details->default_values_func)();

	details->savedata_size = sizeof(crayon_savefile_version_t) +
		(lengths[CRAY_TYPE_DOUBLE] * sizeof(double)) +
		(lengths[CRAY_TYPE_FLOAT] * sizeof(float)) +
		(lengths[CRAY_TYPE_UINT32] * sizeof(uint32_t)) +
		(lengths[CRAY_TYPE_SINT32] * sizeof(int32_t)) +
		(lengths[CRAY_TYPE_UINT16] * sizeof(uint16_t)) +
		(lengths[CRAY_TYPE_SINT16] * sizeof(int16_t)) +
		(lengths[CRAY_TYPE_UINT8] * sizeof(uint8_t)) +
		(lengths[CRAY_TYPE_SINT8] * sizeof(int8_t)) +
		(lengths[CRAY_TYPE_CHAR] * sizeof(char));

	printf("Solidify saves update\n");
	crayon_savefile_update_valid_saves(details);	//Need to double check this
	
	//If no savefile was found, then set our save device to the first valid memcard
	if(details->save_device_id == -1){
		for(i = 0; i < CRAY_SF_NUM_SAVE_DEVICES; i++){
			if(crayon_savefile_get_device_bit(details->present_devices, i)){
				details->save_device_id = i;
				break;
			}
		}
	}

	return 0;
}

void crayon_savefile_update_valid_saves(crayon_savefile_details_t *details){
	uint8_t present_devices = 0;
	uint8_t present_savefiles = 0;
	uint8_t current_savefiles = 0;

	FILE *fp;

	uint8_t i;
	for(i = 0; i < CRAY_SF_NUM_SAVE_DEVICES; i++){
		#if defined(_arch_dreamcast)

		//Check if device is a memory card
		if(crayon_savefile_check_device_for_function(MAPLE_FUNC_MEMCARD, i)){
			continue;
		}

		#endif

		//Check everything went fine, it returns 0
		uint8_t check_result = crayon_savefile_check_savedata(details, i);
		if(check_result){
			//If a savefile is newer than our system, then we leave the whole device alone
			if(check_result != 2 ||
				details->savefile_versions[i] > details->latest_version){
				continue;
			}

			char *savename = crayon_savefile_get_full_path(details, i);
			if(!savename){
				continue;
			}

			//Calculate the size of the savefile to make sure if we have enough space
			uint32_t size_bytes;
			fp = fopen(savename, "rb");
			free(savename);
			if(!fp){	//This usually means check_result was equal to 2
				continue;
			}

			fseek(fp, 0, SEEK_END);
			size_bytes = ftell(fp);
			fclose(fp);

			#if defined(_arch_pc)
			
			//Add a check here to see if the path is valid and if it can be writen to
			;

			#endif

			crayon_savefile_set_device_bit(&present_devices, i);
			if(crayon_savefile_get_device_free_space(i) >= size_bytes){
				crayon_savefile_set_device_bit(&present_devices, i);
			}
		}
		else{
			if(details->savefile_versions[i] <= details->latest_version){
				crayon_savefile_set_device_bit(&present_devices, i);
				crayon_savefile_set_device_bit(&present_savefiles, i);
				if(details->savefile_versions[i] == details->latest_version){
					crayon_savefile_set_device_bit(&current_savefiles, i);
				}
			}
			//If its greater than, then we just leave it alone and say the device isn't present
		}
	}

	details->present_devices = present_devices;
	details->present_savefiles = present_savefiles;
	details->current_savefiles = current_savefiles;

	return;
}

uint8_t crayon_savefile_load_savedata(crayon_savefile_details_t *details){
	//Device isn't present, can't do anything with it
	if(!crayon_savefile_get_device_bit(details->present_devices, details->save_device_id)){
		printf("Test1\n");
		return 1;
	}

	char *savename = crayon_savefile_get_full_path(details, details->save_device_id);
	if(!savename){
		printf("Test2\n");
		return 1;
	}

	printf("PATH: %s\n", savename);

	//If the savefile DNE, this will fail
	FILE *fp = fopen(savename, "rb");
	free(savename);
	if(!fp){
		printf("Test3\n");
		return 1;
	}

	fseek(fp, 0, SEEK_END);
	int pkg_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	uint8_t *data = malloc(pkg_size);
	if(!data){
		fclose(fp);
		printf("Test4\n");
		return 1;
	}

	fread(data, pkg_size, 1, fp);
	fclose(fp);

	#if defined(_arch_dreamcast)

	vmu_pkg_t pkg;
	if(vmu_pkg_parse(data, &pkg)){
		//CRC is incorrect
		;
	}

	//Check to see if pkg.data_len is actually correct or not

	//Read the pkg data into my struct
	uint8_t deserialise_result = crayon_savefile_deserialise(details, (uint8_t *)pkg.data, (uint32_t)pkg.data_len);

	#else

	//Call the endian function on "data"
	;

	//Obtain the data length
	crayon_savefile_hdr_t hdr;

	crayon_misc_encode_to_buffer((uint8_t*)&hdr.name, data, sizeof(hdr.name));
	crayon_misc_encode_to_buffer((uint8_t*)&hdr.app_id, data + sizeof(hdr.name), sizeof(hdr.app_id));

	// crayon_misc_encode_to_buffer((uint8_t*)&hdr.app_id, data, 16);
	// crayon_misc_encode_to_buffer((uint8_t*)&hdr.short_desc, data + 16, 16);
	// crayon_misc_encode_to_buffer((uint8_t*)&hdr.long_desc, data + 32, 32);
	// crayon_misc_encode_to_buffer((uint8_t*)&hdr.data_size, data + 64, 4);

	//Either it has the wrong size or the app ids somehow don't match
	//(The later should never trigger if you use this library right)
	if(strcmp(hdr.app_id, details->strings[CRAY_SF_STRING_APP_ID])){
		free(data);
		printf("Test5\n");
		return 1;
	}

	//Read the pkg data into my struct
	//We add CRAY_SF_HDR_SIZE to skip the header
	uint8_t deserialise_result = crayon_savefile_deserialise(details, data + CRAY_SF_HDR_SIZE,
		pkg_size - CRAY_SF_HDR_SIZE);

	#endif

	printf("Test6 %d\n", deserialise_result);

	free(data);
	return deserialise_result;
}

uint8_t crayon_savefile_save_savedata(crayon_savefile_details_t *details){
	//Device isn't present, can't do anything with it
	if(!crayon_savefile_get_device_bit(details->present_devices, details->save_device_id)){
		return 1;
	}

	char *savename = crayon_savefile_get_full_path(details, details->save_device_id);
	if(!savename){
		return 1;
	}

	FILE *fp;

	uint32_t length = details->savedata_size;
	uint8_t *data = malloc(length);
	if(!data){
		free(savename);
		return 1;
	}

	crayon_savefile_serialise(details, data);

	#if defined(_arch_dreamcast)

	vmu_pkg_t pkg;
	sprintf(pkg.desc_long, details->strings[CRAY_SF_STRING_LONG_DESC]);
	strncpy(pkg.desc_short, details->strings[CRAY_SF_STRING_SHORT_DESC], 16);
	strncpy(pkg.app_id, details->strings[CRAY_SF_STRING_APP_ID], 16);
	pkg.icon_cnt = details->icon_anim_count;
	pkg.icon_anim_speed = details->icon_anim_speed;
	memcpy(pkg.icon_pal, details->icon_palette, 32);
	pkg.icon_data = details->icon_data;
	pkg.eyecatch_type = details->eyecatcher_type;
	pkg.eyecatch_data = details->eyecatcher_data;	//If the type is zero, this will be NULL anyways
	pkg.data_len = length;
	pkg.data = data;

	int pkg_size;
	uint8_t *pkg_out; //Allocated in function below
	vmu_pkg_build(&pkg, &pkg_out, &pkg_size);
	free(data);	//No longer needed

	//Check if a file exists with that name, since we'll overwrite it.
	uint32_t bytes_freed = 0;
	if((fp = fopen(savename, "rb"))){
		fseek(fp, 0, SEEK_END);
		bytes_freed = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		fclose(fp);
	}

	//Make sure there's enough free space on the VMU.
	// if(crayon_savefile_get_device_free_space(details->save_device_id) + bytes_freed <
	// 	crayon_savefile_bytes_to_blocks(pkg_size)){
	// 	free(pkg_out);
	// 	free(savename);
	// 	return 1;
	// }

	//Can't open file for some reason
	fp = fopen(savename, "wb");
	free(savename);
	if(!fp){
		free(pkg_out);
		return 1;
	}

	fwrite(pkg_out, sizeof(uint8_t), pkg_size, fp);
	free(pkg_out);

	#else

	fp = fopen(savename, "wb");
	free(savename);
	if(!fp){
		free(data);
		return 1;
	}

	char string_buffer[32] = {0};
	strncpy(string_buffer, "CRAYON SAVEFILE", sizeof(((crayon_savefile_hdr_t*) 0)->name));
	fwrite(string_buffer, sizeof(char), sizeof(((crayon_savefile_hdr_t*) 0)->name), fp);

	uint8_t i;
	for(i = CRAY_SF_STRING_APP_ID; i < CRAY_SF_NUM_DETAIL_STRINGS; i++){
		strncpy(string_buffer, details->strings[i], crayon_savefile_detail_string_length(i));
		fwrite(string_buffer, sizeof(char), crayon_savefile_detail_string_length(i), fp);
	}

	fwrite(data, sizeof(uint8_t), length, fp);

	free(data);
	
	#endif

	fclose(fp);

	crayon_savefile_set_device_bit(&details->present_savefiles, details->save_device_id);
	crayon_savefile_set_device_bit(&details->current_savefiles, details->save_device_id);

	return 0;
}

uint8_t crayon_savefile_delete_savedata(crayon_savefile_details_t *details){
	//Device isn't present, can't do anything with it
	if(!crayon_savefile_get_device_bit(details->present_devices, details->save_device_id)){
		return 1;
	}

	char *savename = crayon_savefile_get_full_path(details, details->save_device_id);
	if(!savename){
		return 1;
	}

	#if defined(_arch_dreamcast)

	if(crayon_savefile_check_device_for_function(MAPLE_FUNC_MEMCARD, details->save_device_id)){
		free(savename);
		return 1;
	}
	
	//int vmufs_delete(maple_device_t *dev, const char *fn);

	#elif defined (_arch_pc)

	;

	#endif

	free(savename);

	return 1;
}

void crayon_savefile_free_base_path(){
	if(__savefile_path){free(__savefile_path);}
	return;
}

void crayon_savefile_free(crayon_savefile_details_t *details){
	crayon_savefile_free_icon(details);
	crayon_savefile_free_eyecatcher(details);

	//Free up history
	crayon_savefile_history_t *curr = details->history;
	crayon_savefile_history_t *prev = curr;
	while(curr){
		curr = curr->next;
		free(prev);
		prev = curr;
	}

	details->history = NULL;
	details->history_tail = NULL;

	//Free up the actual save data;
	crayon_savefile_free_savedata(&details->savedata);

	uint8_t i;
	for(i = 0; i < CRAY_SF_NUM_DETAIL_STRINGS; i++){
		if(details->strings[i]){free(details->strings[i]);}
		details->strings[i] = NULL;
	}

	return;
}

void crayon_savefile_free_icon(crayon_savefile_details_t *details){
	#ifdef _arch_dreamcast

	if(details->icon_data){free(details->icon_data);}
	if(details->icon_palette){free(details->icon_palette);}
	details->icon_anim_count = 0;
	details->icon_anim_speed = 0;

	#endif

	return;
}

void crayon_savefile_free_eyecatcher(crayon_savefile_details_t *details){
	#ifdef _arch_dreamcast

	if(details->eyecatcher_data){free(details->eyecatcher_data);}
	details->eyecatcher_type = 0;

	#endif

	return;
}

void crayon_savefile_free_savedata(crayon_savefile_data_t *savedata){
	if(savedata->doubles){free(savedata->doubles);}
	if(savedata->floats){free(savedata->floats);}
	if(savedata->u32){free(savedata->u32);}
	if(savedata->s32){free(savedata->s32);}
	if(savedata->u16){free(savedata->u16);}
	if(savedata->s16){free(savedata->s16);}
	if(savedata->u8){free(savedata->u8);}
	if(savedata->s8){free(savedata->s8);}
	if(savedata->chars){free(savedata->chars);}

	savedata->doubles = NULL;
	savedata->floats = NULL;
	savedata->u32 = NULL;
	savedata->s32 = NULL;
	savedata->u16 = NULL;
	savedata->s16 = NULL;
	savedata->u8 = NULL;
	savedata->s8 = NULL;
	savedata->chars = NULL;

	uint8_t i;
	for(i = 0; i < CRAY_NUM_TYPES; i++){
		savedata->lengths[i] = 0;
	}

	return;
}
