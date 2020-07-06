#include "savefile.h"

//Its a path to the folder where it will try to save to
char * __savefile_path = NULL;
uint16_t __savefile_path_length;

vec2_s8_t crayon_savefile_dreamcast_get_port_and_slot(int8_t save_device_id){
	vec2_s8_t values = {-1,-1};
	if(save_device_id < 0 || save_device_id >= 8){return values;}
	
	if(save_device_id % 2 == 0){
		values.y = 1;
	}
	else{
		values.y = 2;
	}

	values.x = save_device_id / 2;

	return values;
}

//Why is valid screens a part of Savefile details, but this function isn't?
void crayon_vmu_display_icon(uint8_t vmu_bitmap, void *icon){
	#ifdef _arch_dreamcast
	maple_device_t *vmu;
	vec2_s8_t port_and_slot;
	uint8_t i;
	for(i = 0; i < CRAY_SF_NUM_SAVE_DEVICES; i++){
		if(crayon_savefile_get_memcard_bit(vmu_bitmap, i)){	//We want to display on this VMU
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

//Returns true if device has certain function/s
uint8_t crayon_savefile_check_device_for_function(uint32_t function, int8_t save_device_id){
	#ifdef _arch_dreamcast

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

	#else

	//For PC we only have one slot and function is ignored for now
	if(save_device_id < 0 || save_device_id >= CRAY_SF_NUM_SAVE_DEVICES){
		return 1;
	}
	return 0;

	#endif
}

//Dreamcast: Rounds the number down to nearest multiple of 512 , then adds 1 if there's a remainder
//PC: Returns 0 (Function isn't needed on PC anyways)
uint16_t crayon_savefile_bytes_to_blocks(size_t bytes){
	return (bytes >> 9) + !!(bytes & ((1 << 9) - 1));
}

//I used the "vmu_pkg_build" function's source as a guide for this. Doesn't work with compressed saves
int16_t crayon_savefile_get_save_block_count(crayon_savefile_details_t *details){
	#ifdef _arch_dreamcast

	uint16_t eyecatcher_size  = 0;
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
			return -1;
	}

	//Get the total number of bytes. Keep in mind we need to think about the icon/s and EC
	size_t size = CRAY_SF_HDR_SIZE + (512 * details->icon_anim_count) + eyecatcher_size +
		sizeof(crayon_savefile_version_t) + details->save_size;

	return crayon_savefile_bytes_to_blocks(size);

	#else

	return 0;

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
		case CRAY_SF_STRING_SHORT_DESC:
			return 16;
		case CRAY_SF_STRING_LONG_DESC:
			return 32;
		default:
			return 0;
	}
}

//DON'T FORGET THE PKG DATA ALSO CONTAINS THE VERSION NUMBER
void __attribute__((weak)) crayon_savefile_serialise(crayon_savefile_details_t *details, uint8_t *buffer){
	crayon_savefile_data_t data = details->save_data;

	//Encode the version number
	crayon_misc_encode_to_buffer(buffer, (uint8_t*)&details->version, sizeof(crayon_savefile_version_t));
	buffer += sizeof(crayon_savefile_version_t);

	//Next encode all doubles, then floats, then unsigned 32-bit ints, then signed 32-bit ints
	//Then 16-bit ints, 8-bit ints and finally the characters. No need to encode the lengths
	//since the history can tell us that
	crayon_misc_encode_to_buffer(buffer, (uint8_t*)data.doubles, sizeof(double) * data.lengths[CRAY_TYPE_DOUBLE]);
	buffer += sizeof(double) * data.lengths[CRAY_TYPE_DOUBLE];

	crayon_misc_encode_to_buffer(buffer, (uint8_t*)data.floats, sizeof(float) * data.lengths[CRAY_TYPE_FLOAT]);
	buffer += sizeof(float) * data.lengths[CRAY_TYPE_FLOAT];

	crayon_misc_encode_to_buffer(buffer, (uint8_t*)data.u32, sizeof(uint32_t) * data.lengths[CRAY_TYPE_UINT32]);
	buffer += sizeof(uint32_t) * data.lengths[CRAY_TYPE_UINT32];

	crayon_misc_encode_to_buffer(buffer, (uint8_t*)data.s32, sizeof(int32_t) * data.lengths[CRAY_TYPE_SINT32]);
	buffer += sizeof(int32_t) * data.lengths[CRAY_TYPE_SINT32];

	crayon_misc_encode_to_buffer(buffer, (uint8_t*)data.u16, sizeof(uint16_t) * data.lengths[CRAY_TYPE_UINT16]);
	buffer += sizeof(uint16_t) * data.lengths[CRAY_TYPE_UINT16];

	crayon_misc_encode_to_buffer(buffer, (uint8_t*)data.s16, sizeof(int16_t) * data.lengths[CRAY_TYPE_SINT16]);
	buffer += sizeof(int16_t) * data.lengths[CRAY_TYPE_SINT16];

	crayon_misc_encode_to_buffer(buffer, data.u8, sizeof(uint8_t) * data.lengths[CRAY_TYPE_UINT8]);
	buffer += sizeof(uint8_t) * data.lengths[CRAY_TYPE_UINT8];

	crayon_misc_encode_to_buffer(buffer, (uint8_t*)data.s8, sizeof(int8_t) * data.lengths[CRAY_TYPE_SINT8]);
	buffer += sizeof(int8_t) * data.lengths[CRAY_TYPE_SINT8];

	crayon_misc_encode_to_buffer(buffer, (uint8_t*)data.chars, sizeof(char) * data.lengths[CRAY_TYPE_CHAR]);
	// buffer += sizeof(char) * data.lengths[CRAY_TYPE_CHAR];

	return;
}

uint8_t __attribute__((weak)) crayon_savefile_deserialise(crayon_savefile_details_t *details, uint8_t *buffer,
	uint32_t data_length){
	//Same as serialiser, but instead we extract the variables and version from the buffer
	//We'll also need to check if the version number is valid (So we'll need to call the endianness function)
	crayon_savefile_data_t data = details->save_data;

	//Decode the version number
	crayon_savefile_version_t version;
	crayon_misc_encode_to_buffer((uint8_t*)&version, buffer, sizeof(crayon_savefile_version_t));
	buffer += sizeof(crayon_savefile_version_t);

	crayon_misc_correct_endian((uint8_t*)&version, sizeof(crayon_savefile_version_t));
	if(version > details->version){
		return 1;
	}
	
	//If same version, deserialising is as simple as serialising
	if(version == details->version){
		crayon_misc_encode_to_buffer((uint8_t*)data.doubles, buffer, sizeof(double) * data.lengths[CRAY_TYPE_DOUBLE]);
		buffer += sizeof(double) * data.lengths[CRAY_TYPE_DOUBLE];

		crayon_misc_encode_to_buffer((uint8_t*)data.floats, buffer, sizeof(float) * data.lengths[CRAY_TYPE_FLOAT]);
		buffer += sizeof(float) * data.lengths[CRAY_TYPE_FLOAT];

		crayon_misc_encode_to_buffer((uint8_t*)data.u32, buffer, sizeof(uint32_t) * data.lengths[CRAY_TYPE_UINT32]);
		buffer += sizeof(uint32_t) * data.lengths[CRAY_TYPE_UINT32];

		crayon_misc_encode_to_buffer((uint8_t*)data.s32, buffer, sizeof(int32_t) * data.lengths[CRAY_TYPE_SINT32]);
		buffer += sizeof(int32_t) * data.lengths[CRAY_TYPE_SINT32];

		crayon_misc_encode_to_buffer((uint8_t*)data.u16, buffer, sizeof(uint16_t) * data.lengths[CRAY_TYPE_UINT16]);
		buffer += sizeof(uint16_t) * data.lengths[CRAY_TYPE_UINT16];

		crayon_misc_encode_to_buffer((uint8_t*)data.s16, buffer, sizeof(int16_t) * data.lengths[CRAY_TYPE_SINT16]);
		buffer += sizeof(int16_t) * data.lengths[CRAY_TYPE_SINT16];

		crayon_misc_encode_to_buffer(data.u8, buffer, sizeof(uint8_t) * data.lengths[CRAY_TYPE_UINT8]);
		buffer += sizeof(uint8_t) * data.lengths[CRAY_TYPE_UINT8];

		crayon_misc_encode_to_buffer((uint8_t*)data.s8, buffer, sizeof(int8_t) * data.lengths[CRAY_TYPE_SINT8]);
		buffer += sizeof(int8_t) * data.lengths[CRAY_TYPE_SINT8];

		crayon_misc_encode_to_buffer((uint8_t*)data.chars, buffer, sizeof(char) * data.lengths[CRAY_TYPE_CHAR]);
		// buffer += sizeof(char) * data.lengths[CRAY_TYPE_CHAR];
	}
	else{	//Need to use history to bring it up to speed
		printf("Backwards compatibility currently unsupported\n");
		return 1;
	}

	return 0;
}

uint16_t crayon_savefile_get_device_free_blocks(int8_t device_id){
	#if defined(_arch_dreamcast)
	
	vec2_s8_t port_and_slot = crayon_savefile_dreamcast_get_port_and_slot(device_id);

	maple_device_t *vmu;
	vmu = maple_enum_dev(port_and_slot.x, port_and_slot.y);
	return vmufs_free_blocks(vmu);
	
	#elif defined(_arch_pc)

	return 0;
	
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

uint8_t crayon_savefile_get_memcard_bit(uint8_t memcard_bitmap, uint8_t save_device_id){
	return (memcard_bitmap >> save_device_id) & 1;
}

void crayon_savefile_set_memcard_bit(uint8_t *memcard_bitmap, uint8_t save_device_id){
	*memcard_bitmap |= (1 << save_device_id);
	return;
}

//THIS CAN BE OPTIMISED
uint8_t crayon_savefile_check_savedata(crayon_savefile_details_t *details, int8_t save_device_id){
	char *savename = crayon_savefile_get_full_path(details, save_device_id);
	if(!savename){
		// printf("FAILED AT 1\n");
		return 1;
	}

	// printf("PATH: %s\n", savename);

	FILE *fp = fopen(savename, "rb");
	free(savename);
	//File DNE
	if(!fp){
		// printf("FAILED AT 2\n");
		return 1;
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
	vmu_pkg_parse(pkg_out, &pkg);

	free(pkg_out);

	//If the version IDs don't match, then can can't comprehend that savefile
	if(strcmp(pkg.app_id, details->strings[CRAY_SF_STRING_APP_ID])){
		printf("FAILED AT 3\n");
		return 1;
	}

	//Check to confirm the savefile version is not newer than it should be
	crayon_savefile_version_t sf_version;
	crayon_misc_encode_to_buffer((uint8_t*)&sf_version, (uint8_t*)pkg.data, sizeof(crayon_savefile_version_t));

	if(sf_version > details->version){
		printf("FAILED AT 4\n");
		return 2;
	}

	#else

	uint16_t length = crayon_savefile_detail_string_length(CRAY_SF_STRING_APP_ID);
	char *app_id_buffer = malloc(sizeof(char) * length);
	if(!app_id_buffer){
		fclose(fp);
		printf("FAILED AT 5\n");
		return 1;
	}

	fread(app_id_buffer, length, 1, fp);	//The app id is at the start of the hdr on PC

	crayon_savefile_version_t sf_version;
	fseek(fp, CRAY_SF_HDR_SIZE, SEEK_SET);
	fread(&sf_version, 4, 1, fp);

	fclose(fp);

	uint8_t rv = 0;
	if(strcmp(app_id_buffer, details->strings[CRAY_SF_STRING_APP_ID])){
		rv = 1;
	}
	free(app_id_buffer);

	if(rv){
		printf("FAILED AT 6\n");
		return 1;
	}

	// printf("read version %d. our version %d\n", sf_version, details->version);

	//Check to confirm the savefile version is not newer than it should be
	if(sf_version > details->version){
		printf("FAILED AT 7\n");
		return 2;
	}

	#endif

	return 0;
}

uint8_t crayon_savefile_set_path(char * path){
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
	crayon_savefile_version_t version){
	uint16_t save_name_length = strlen(save_name);

	details->version = version;

	details->save_data.doubles = NULL;
	details->save_data.floats = NULL;
	details->save_data.u32 = NULL;
	details->save_data.s32 = NULL;
	details->save_data.u16 = NULL;
	details->save_data.s16 = NULL;
	details->save_data.u8 = NULL;
	details->save_data.s8 = NULL;
	details->save_data.chars = NULL;
	uint8_t i;
	for(i = 0; i < CRAY_NUM_TYPES; i++){
		details->save_data.lengths[i] = 0;
	}

	details->save_size = 0;	//For now
	details->icon_anim_count = 0;

	#ifdef _arch_dreamcast
	
	details->eyecatcher_type = VMUPKG_EC_NONE;	//The default
	
	#else
	
	details->eyecatcher_type = 0;
	
	#endif

	details->icon_data = NULL;
	details->icon_palette = NULL;
	details->eyecatcher_data = NULL;

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

	//Given string is too big
	if(save_name_length > str_lengths[CRAY_SF_STRING_FILENAME] - 1){
		for(i = 0; i < CRAY_SF_NUM_DETAIL_STRINGS; i++){
			if(details->strings[i]){free(details->strings[i]);}
		}
		return 1;
	}

	//Copy the savename here
	strncpy(details->strings[CRAY_SF_STRING_FILENAME], save_name, save_name_length + 1);
	details->strings[CRAY_SF_STRING_FILENAME][str_lengths[CRAY_SF_STRING_FILENAME] - 1] = '\0';

	//Update the savefile and memcards stuff
	details->valid_vmu_screens = crayon_savefile_get_valid_screens();

	details->save_device_id = -1;

	details->history = NULL;
	details->history_tail = NULL;

	return 0;
}

uint8_t crayon_savefile_set_string(crayon_savefile_details_t *details, const char *string, uint8_t string_id){
	uint16_t max_length = crayon_savefile_detail_string_length(string_id);
	uint16_t string_length = strlen(string);
	if(max_length == 0 || string_length >= max_length){return 1;}

	strncpy(details->strings[string_id], string, string_length + 1);
	details->strings[string_id][max_length - 1] = '\0';

	return 0;
}

uint8_t crayon_savefile_add_icon(crayon_savefile_details_t *details, const char *image, const char *palette,
	uint8_t icon_anim_count, uint16_t icon_anim_speed){
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

crayon_savefile_history_t *crayon_savefile_add_variable(crayon_savefile_details_t *details, void *data_ptr,
	uint8_t data_type, uint32_t length, const void *default_value, crayon_savefile_version_t version){
	//data_type id doesn't map to any of our types
	if(data_type >= CRAY_NUM_TYPES){
		return NULL;
	}

	crayon_savefile_history_t *var = malloc(sizeof(crayon_savefile_history_t));
	if(!var){return NULL;}

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

	var->data_type = data_type;
	var->data_length = length;
	var->version_added = version;
	var->version_removed = 0;	//0 means it wasn't removed
	var->removal_command = 0;
	var->transfer_var = NULL;

	details->save_data.lengths[var->data_type] += var->data_length;

	//Store a pointer to the type and the default value
	switch(var->data_type){
		case CRAY_TYPE_DOUBLE:
			var->data_ptr.doubles = (double**)data_ptr;
			var->default_value.doubles = *((double*)default_value);
			break;
		case CRAY_TYPE_FLOAT:
			var->data_ptr.floats = (float**)data_ptr;
			var->default_value.floats = *((float*)default_value);
			break;
		case CRAY_TYPE_UINT32:
			var->data_ptr.u32 = (uint32_t**)data_ptr;
			var->default_value.u32 = *((uint32_t*)default_value);
			break;
		case CRAY_TYPE_SINT32:
			var->data_ptr.s32 = (int32_t**)data_ptr;
			var->default_value.s32 = *((int32_t*)default_value);
			break;
		case CRAY_TYPE_UINT16:
			var->data_ptr.u16 = (uint16_t**)data_ptr;
			var->default_value.u16 = *((uint16_t*)default_value);
			break;
		case CRAY_TYPE_SINT16:
			var->data_ptr.s16 = (int16_t**)data_ptr;
			var->default_value.s16 = *((int16_t*)default_value);
			break;
		case CRAY_TYPE_UINT8:
			var->data_ptr.u8 = (uint8_t**)data_ptr;
			var->default_value.u8 = *((uint8_t*)default_value);
			break;
		case CRAY_TYPE_SINT8:
			var->data_ptr.s8 = (int8_t**)data_ptr;
			var->default_value.s8 = *((int8_t*)default_value);
			break;
		case CRAY_TYPE_CHAR:
			var->data_ptr.chars = (char**)data_ptr;
			var->default_value.chars = *((char*)default_value);
			break;
	}

	return var;
}

crayon_savefile_history_t *crayon_savefile_remove_variable(crayon_savefile_details_t *details,
	crayon_savefile_history_t *target_node, uint8_t remove_command, crayon_savefile_history_t *transfer_var,
	crayon_savefile_version_t version){

	crayon_savefile_history_t *var = details->history;
	uint8_t found = 0;

	while(var){
		if(var == target_node){
			found = 1;
			break;
		}
		var = var->next;
	}

	if(!found){
		return NULL;
	}

	var->version_removed = version;
	var->removal_command = remove_command;
	var->transfer_var = transfer_var;

	details->save_data.lengths[var->data_type] -= var->data_length;

	return var;
}

uint8_t crayon_savefile_solidify(crayon_savefile_details_t *details){
	uint32_t *lengths = details->save_data.lengths;
	uint32_t indexes[CRAY_NUM_TYPES] = {0};

	//Don't both allocating space for these if we aren't using them
	if(lengths[CRAY_TYPE_DOUBLE]){details->save_data.doubles = malloc(sizeof(double) * lengths[CRAY_TYPE_DOUBLE]);}
	if(lengths[CRAY_TYPE_FLOAT]){details->save_data.floats = malloc(sizeof(float) * lengths[CRAY_TYPE_FLOAT]);}
	if(lengths[CRAY_TYPE_UINT32]){details->save_data.u32 = malloc(sizeof(uint32_t) * lengths[CRAY_TYPE_UINT32]);}
	if(lengths[CRAY_TYPE_SINT32]){details->save_data.s32 = malloc(sizeof(int32_t) * lengths[CRAY_TYPE_SINT32]);}
	if(lengths[CRAY_TYPE_UINT16]){details->save_data.u16 = malloc(sizeof(uint16_t) * lengths[CRAY_TYPE_UINT16]);}
	if(lengths[CRAY_TYPE_SINT16]){details->save_data.s16 = malloc(sizeof(int16_t) * lengths[CRAY_TYPE_SINT16]);}
	if(lengths[CRAY_TYPE_UINT8]){details->save_data.u8 = malloc(sizeof(uint8_t) * lengths[CRAY_TYPE_UINT8]);}
	if(lengths[CRAY_TYPE_SINT8]){details->save_data.s8 = malloc(sizeof(int8_t) * lengths[CRAY_TYPE_SINT8]);}
	if(lengths[CRAY_TYPE_CHAR]){details->save_data.chars = malloc(sizeof(char) * lengths[CRAY_TYPE_CHAR]);}

	//Add in malloc error support
	;

	uint32_t i;
	crayon_savefile_history_t *var = details->history;
	while(var){
		if(var->version_removed == 0){	//We only give space to vars that still exist
			switch(var->data_type){
				case CRAY_TYPE_DOUBLE:
					*var->data_ptr.doubles = &details->save_data.doubles[indexes[var->data_type]];
					for(i = 0; i < var->data_length; i++){(*var->data_ptr.doubles)[i] = var->default_value.doubles;}
					break;
				case CRAY_TYPE_FLOAT:
					*var->data_ptr.floats = &details->save_data.floats[indexes[var->data_type]];
					for(i = 0; i < var->data_length; i++){(*var->data_ptr.floats)[i] = var->default_value.floats;}
					break;
				case CRAY_TYPE_UINT32:
					*var->data_ptr.u32 = &details->save_data.u32[indexes[var->data_type]];
					for(i = 0; i < var->data_length; i++){(*var->data_ptr.u32)[i] = var->default_value.u32;}
					break;
				case CRAY_TYPE_SINT32:
					*var->data_ptr.s32 = &details->save_data.s32[indexes[var->data_type]];
					for(i = 0; i < var->data_length; i++){(*var->data_ptr.s32)[i] = var->default_value.s32;}
					break;
				case CRAY_TYPE_UINT16:
					*var->data_ptr.u16 = &details->save_data.u16[indexes[var->data_type]];
					for(i = 0; i < var->data_length; i++){(*var->data_ptr.u16)[i] = var->default_value.u16;}
					break;
				case CRAY_TYPE_SINT16:
					*var->data_ptr.s16 = &details->save_data.s16[indexes[var->data_type]];
					for(i = 0; i < var->data_length; i++){(*var->data_ptr.s16)[i] = var->default_value.s16;}
					break;
				case CRAY_TYPE_UINT8:
					*var->data_ptr.u8 = &details->save_data.u8[indexes[var->data_type]];
					for(i = 0; i < var->data_length; i++){(*var->data_ptr.u8)[i] = var->default_value.u8;}
					break;
				case CRAY_TYPE_SINT8:
					*var->data_ptr.s8 = &details->save_data.s8[indexes[var->data_type]];
					for(i = 0; i < var->data_length; i++){(*var->data_ptr.s8)[i] = var->default_value.s8;}
					break;
				case CRAY_TYPE_CHAR:
					*var->data_ptr.chars = &details->save_data.chars[indexes[var->data_type]];
					memset(*var->data_ptr.chars, var->default_value.chars, var->data_length);
				break;
			}
			indexes[var->data_type] += var->data_length;
		}

		var = var->next;
	}

	details->save_size = (lengths[CRAY_TYPE_DOUBLE] * sizeof(double)) +
		(lengths[CRAY_TYPE_FLOAT] * sizeof(float)) +
		(lengths[CRAY_TYPE_UINT32] * sizeof(uint32_t)) +
		(lengths[CRAY_TYPE_SINT32] * sizeof(int32_t)) +
		(lengths[CRAY_TYPE_UINT16] * sizeof(uint16_t)) +
		(lengths[CRAY_TYPE_SINT16] * sizeof(int16_t)) +
		(lengths[CRAY_TYPE_UINT8] * sizeof(uint8_t)) +
		(lengths[CRAY_TYPE_SINT8] * sizeof(int8_t)) +
		(lengths[CRAY_TYPE_CHAR] * sizeof(char));

	printf("Solidify saves update\n");
	crayon_savefile_update_valid_saves(details, CRAY_SAVEFILE_UPDATE_MODE_BOTH);	//Need to double check this
	
	//If no savefile was found, then set our save device to the first valid memcard
	if(details->save_device_id == -1){
		for(i = 0; i < CRAY_SF_NUM_SAVE_DEVICES; i++){
			if(crayon_savefile_get_memcard_bit(details->valid_memcards, i)){
				details->save_device_id = i;
				break;
			}
		}
	}

	return 0;
}

void crayon_savefile_update_valid_saves(crayon_savefile_details_t *details, uint8_t modes){
	uint8_t valid_saves = 0;	//a1a2b1b2c1c2d1d2
	uint8_t valid_memcards = 0;	//Note: These are memcards that either can or do contain savefiles

	uint8_t get_saves = 0;
	uint8_t get_memcards = 0;

	if(modes & CRAY_SAVEFILE_UPDATE_MODE_SAVE_PRESENT){
		get_saves = 1;
	}
	if(modes & CRAY_SAVEFILE_UPDATE_MODE_MEMCARD_PRESENT){
		get_memcards = 1;
	}

	uint8_t i, error;
	for(i = 0; i < CRAY_SF_NUM_SAVE_DEVICES; i++){
		//Check if device is a memory card
		if(crayon_savefile_check_device_for_function(CRAY_SF_MEMCARD, i)){
			continue;
		}

		//Check if a save file DNE. Returns 1 if DNE
		error = crayon_savefile_check_savedata(details, i);
		printf("ERROR %d\n", error);
		if(error){
			if(!get_memcards || error == 2){continue;}	//If 2 then the version is unsupported

			#if defined(_arch_pc)	//We assume there's enough space on PC

			crayon_savefile_set_memcard_bit(&valid_memcards, i);

			#else

			if(crayon_savefile_get_device_free_blocks(i) >=
				crayon_savefile_get_save_block_count(details)){
				crayon_savefile_set_memcard_bit(&valid_memcards, i);
			}

			#endif
		}
		else{
			if(get_memcards){crayon_savefile_set_memcard_bit(&valid_memcards, i);}
			if(get_saves){crayon_savefile_set_memcard_bit(&valid_saves, i);}
		}
	}

	if(get_saves){details->valid_saves = valid_saves;}
	if(get_memcards){details->valid_memcards = valid_memcards;}

	return;
}

uint8_t crayon_savefile_get_valid_function(uint32_t function){
	uint8_t valid_function = 0;	//a1a2b1b2c1c2d1d2

	int i;
	for(i = 0; i < CRAY_SF_NUM_SAVE_DEVICES; i++){
		//Check if device contains this function bitmap (Returns 0 on success)
		if(!crayon_savefile_check_device_for_function(function, i)){
			crayon_savefile_set_memcard_bit(&valid_function, i);
		}
		
	}

	return valid_function;
}

uint8_t crayon_savefile_load_savedata(crayon_savefile_details_t *details){
	//If you call everying in the right order, this is redundant. But just incase you didn't, here it is
	//Also we use check for a memory card and not a savefile because the rest of the load code can automatically
	//check if a save exists so its faster this way (Since this function and crayon_savefile_check_savedata()
	//share alot of the same code)
	if(crayon_savefile_check_device_for_function(CRAY_SF_MEMCARD, details->save_device_id)){
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
	vmu_pkg_parse(data, &pkg);

	//Read the pkg data into my struct
	uint8_t deserialise_result = crayon_savefile_deserialise(details, (uint8_t *)pkg.data,
		(uint32_t)pkg.data_len);

	#else

	//Obtain the data length
	crayon_savefile_hdr_t hdr;

	crayon_misc_encode_to_buffer((uint8_t*)&hdr.app_id, data, 16);
	crayon_misc_encode_to_buffer((uint8_t*)&hdr.short_desc, data + 16, 16);
	crayon_misc_encode_to_buffer((uint8_t*)&hdr.long_desc, data + 32, 32);
	crayon_misc_encode_to_buffer((uint8_t*)&hdr.data_size, data + 64, 4);

	//Either it has the wrong size or the app ids somehow don't match
	//(The later should never trigger if you use this library right)
	if(hdr.data_size != pkg_size - CRAY_SF_HDR_SIZE || strcmp(hdr.app_id, details->strings[CRAY_SF_STRING_APP_ID])){
		free(data);
		printf("%d, %d\n", hdr.data_size, pkg_size - CRAY_SF_HDR_SIZE);
		printf("%s %s\n", hdr.app_id, details->strings[CRAY_SF_STRING_APP_ID]);
		printf("Test5\n");
		return 1;
	}

	//Read the pkg data into my struct
	//We add CRAY_SF_HDR_SIZE to skip the header
	uint8_t deserialise_result = crayon_savefile_deserialise(details, data + CRAY_SF_HDR_SIZE,
		hdr.data_size);

	#endif

	printf("Test6 %d\n", deserialise_result);

	free(data);
	return deserialise_result;
}

uint8_t crayon_savefile_save_savedata(crayon_savefile_details_t *details){

	//The requested VMU is not a valid memory card
	if(!crayon_savefile_get_memcard_bit(details->valid_memcards, details->save_device_id)){
		return 1;
	}

	//If you call everying in the right order, this is redundant
	//But just incase you didn't, here it is
	if(crayon_savefile_check_device_for_function(CRAY_SF_MEMCARD, details->save_device_id)){
		return 1;
	}

	char *savename = crayon_savefile_get_full_path(details, details->save_device_id);
	if(!savename){
		return 1;
	}

	FILE *fp;

	uint32_t length = sizeof(crayon_savefile_version_t) + details->save_size;
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
	if(pkg.eyecatch_type){
		pkg.eyecatch_data = details->eyecatcher_data;
	}
	pkg.data_len = length;
	pkg.data = data;

	int pkg_size;
	uint8_t *pkg_out; //Allocated in function below
	vmu_pkg_build(&pkg, &pkg_out, &pkg_size);
	free(data);	//No longer needed

	//Check if a file exists with that name, since we'll overwrite it.
	uint16_t blocks_freed = 0;
	// file_t f = fs_open(savename, O_RDONLY);
	// if(f != FILEHND_INVALID){
	// 	int fs_size = fs_total(f);
	// 	blocks_freed = crayon_savefile_bytes_to_blocks(fs_size);
	// 	fs_close(f);
	// }

	//Delete the above version and replace it with this later
	if((fp = fopen(savename, "rb"))){
		fseek(fp, 0, SEEK_END);
		blocks_freed = crayon_savefile_bytes_to_blocks(ftell(fp));
		fseek(fp, 0, SEEK_SET);
		fclose(fp);
	}

	//Make sure there's enough free space on the VMU.
	if(crayon_savefile_get_device_free_blocks(details->save_device_id) + blocks_freed <
		crayon_savefile_bytes_to_blocks(pkg_size)){
		free(pkg_out);
		free(savename);
		return 1;
	}

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
	uint8_t i;
	for(i = CRAY_SF_STRING_APP_ID; i < CRAY_SF_NUM_DETAIL_STRINGS; i++){
		strncpy(string_buffer, details->strings[i], crayon_savefile_detail_string_length(i));
		fwrite(string_buffer, sizeof(char), crayon_savefile_detail_string_length(i), fp);
	}

	fwrite(&length, sizeof(uint32_t), 1, fp);

	fwrite(data, length, sizeof(uint8_t), fp);

	free(data);
	
	#endif

	fclose(fp);

	return 0;
}

uint8_t crayon_savefile_delete_savedata(crayon_savefile_details_t *details){
	if(crayon_savefile_check_device_for_function(CRAY_SF_MEMCARD, details->save_device_id)){
		return 1;
	}

	char *savename = crayon_savefile_get_full_path(details, details->save_device_id);
	if(!savename){
		return 1;
	}

	#if defined(_arch_dreamcast)
	
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
	if(details->save_data.doubles){free(details->save_data.doubles);}
	if(details->save_data.floats){free(details->save_data.floats);}
	if(details->save_data.u32){free(details->save_data.u32);}
	if(details->save_data.s32){free(details->save_data.s32);}
	if(details->save_data.u16){free(details->save_data.u16);}
	if(details->save_data.s16){free(details->save_data.s16);}
	if(details->save_data.u8){free(details->save_data.u8);}
	if(details->save_data.s8){free(details->save_data.s8);}
	if(details->save_data.chars){free(details->save_data.chars);}

	details->save_data.doubles = NULL;
	details->save_data.floats = NULL;
	details->save_data.u32 = NULL;
	details->save_data.s32 = NULL;
	details->save_data.u16 = NULL;
	details->save_data.s16 = NULL;
	details->save_data.u8 = NULL;
	details->save_data.s8 = NULL;
	details->save_data.chars = NULL;

	uint8_t i;
	for(i = 0; i < CRAY_NUM_TYPES; i++){
		details->save_data.lengths[i] = 0;
	}

	for(i = 0; i < CRAY_SF_NUM_DETAIL_STRINGS; i++){
		if(details->strings[i]){free(details->strings[i]);}
		details->strings[i] = NULL;
	}

	return;
}

void crayon_savefile_free_icon(crayon_savefile_details_t *details){
	if(details->icon_data){free(details->icon_data);}
	if(details->icon_palette){free(details->icon_palette);}
	details->icon_anim_count = 0;
	details->icon_anim_speed = 0;

	return;
}

void crayon_savefile_free_eyecatcher(crayon_savefile_details_t *details){
	if(details->eyecatcher_data){free(details->eyecatcher_data);}
	details->eyecatcher_type = 0;

	return;
}
