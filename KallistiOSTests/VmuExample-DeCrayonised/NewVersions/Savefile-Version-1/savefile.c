#include "savefile.h"

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

//Note this assumes the vmu chosen is valid
	//THIS CAN BE OPTIMISED
uint8_t crayon_savefile_check_savedata(crayon_savefile_details_t *details, int8_t save_device_id){
	#if defined( _arch_dreamcast)
	uint8_t *pkg_out;
	int pkg_size;
	FILE *fp;
	vmu_pkg_t pkg;

	vec2_s8_t port_and_slot = crayon_savefile_dreamcast_get_port_and_slot(save_device_id);
	if(port_and_slot.x < 0){return 1;}

	//Non-Dreamcast check
	// if(save_device_id < 0 || save_device_id >= CRAY_SF_NUM_SAVE_DEVICES){return 1;}

	//port/port_and_slot.x gets converted to a, b, c or d. port_and_slot.y is the slot (0 or 1)
	//Its more efficient to do it this way than with sprintf
	char *savename = malloc(64);
	if(!savename){
		return 1;
	}

	strcpy(savename, __savefile_base_path);
	savename[__savefile_base_path_length    ] = port_and_slot.x + 'a';
	savename[__savefile_base_path_length + 1] = port_and_slot.y + '0';
	savename[__savefile_base_path_length + 2] = '/';
	savename[__savefile_base_path_length + 3] = '\0';
	strlcat(savename, details->strings[CRAY_SF_STRING_FILENAME], 64);

	//File DNE
	fp = fopen(savename, "rb");
	free(savename);
	if(!fp){
		return 1;
	}

	fseek(fp, 0, SEEK_END);
	pkg_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	//Surely instead of doing the below I can just read the header and hence the app id?

	pkg_out = malloc(pkg_size);
	fread(pkg_out, pkg_size, 1, fp);
	fclose(fp);

	vmu_pkg_parse(pkg_out, &pkg);

	free(pkg_out);

	//If the IDs don't match, then thats an error
	if(strcmp(pkg.app_id, details->strings[CRAY_SF_STRING_APP_ID])){
		return 1;
	}

	return 0;

	#else
	
	if(save_device_id < 0 || save_device_id >= CRAY_SF_NUM_SAVE_DEVICES){return 1;}

	uint16_t length = crayon_savefile_detail_string_length(CRAY_SF_STRING_APP_ID);
	char *app_id_buffer = malloc(sizeof(char) * length);
	if(!app_id_buffer){return 1;}

	char *savename = malloc(__savefile_base_path_length +
		crayon_savefile_detail_string_length(CRAY_SF_STRING_FILENAME) + 1);
	if(!savename){
		free(app_id_buffer);
		return 1;
	}

	strcpy(savename, __savefile_base_path);
	strcat(savename, details->strings[CRAY_SF_STRING_FILENAME]);

	FILE *fp = fopen(details->strings[CRAY_SF_STRING_FILENAME], "rb");
	free(savename);
	if(!fp){
		free(app_id_buffer);
		return 1;
	}

	fread(app_id_buffer, length, 1, fp);	//The app id is at the start of the hdr on PC
	fclose(fp);

	uint8_t rv = 0;
	if(strcmp(app_id_buffer, details->strings[CRAY_SF_STRING_APP_ID])){
		rv = 1;
	}

	free(app_id_buffer);
	return rv;

	#endif
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
		
			return 20 - 8;	//The 8 is the "/vmu/XX/" and the name itself can only be 12 chars (All caps)
		
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
void __attribute__((weak)) crayon_savefile_serialise(crayon_savefile_data_t *sf_data, uint8_t *pkg_data){
	return;
}

void __attribute__((weak)) crayon_savefile_deserialise(crayon_savefile_data_t *sf_data, uint8_t *pkg_data, uint32_t pkg_size){
	return;
}

uint16_t crayon_savefile_device_free_blocks(int8_t port, int8_t slot){
	#ifdef _arch_dreamcast
	
	maple_device_t *vmu;
	vmu = maple_enum_dev(port, slot);
	return vmufs_free_blocks(vmu);
	
	#else

	return 0;
	
	#endif
}

uint8_t crayon_savefile_get_memcard_bit(uint8_t memcard_bitmap, uint8_t save_device_id){
	return (memcard_bitmap >> save_device_id) & 1;
}

void crayon_savefile_set_memcard_bit(uint8_t *memcard_bitmap, uint8_t save_device_id){
	*memcard_bitmap |= (1 << save_device_id);
	return;
}

uint8_t crayon_savefile_set_base_path(char * path){
	#ifdef _arch_dreamcast
	(void)path;

	if(!(__savefile_base_path = malloc(6))){	// "/vmu/"
		return 1;
	}

	strcpy("/vmu/", __savefile_base_path);

	#else
	uint16_t length = strlen(path);

	if(!(__savefile_base_path = malloc(length))){
		return 1;
	}

	strcpy(path, __savefile_base_path);
	#endif

	__savefile_base_path_length = strlen(__savefile_base_path);
	return 0;
}

//Make sure to call this first (And call the save icon and eyecatcher functions after since this overides them)
uint8_t crayon_savefile_init_savefile_details(crayon_savefile_details_t *details, const char *save_name,
	crayon_savefile_version_t version){
	details->version = version;

	details->save_data.u8 = NULL;
	details->save_data.u16 = NULL;
	details->save_data.u32 = NULL;
	details->save_data.s8 = NULL;
	details->save_data.s16 = NULL;
	details->save_data.s32 = NULL;
	details->save_data.floats = NULL;
	details->save_data.doubles = NULL;
	details->save_data.chars = NULL;
	uint8_t i;
	for(i = 0; i < 9; i++){
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

	//Copy the savename here
	strlcpy(details->strings[CRAY_SF_STRING_FILENAME], save_name, str_lengths[CRAY_SF_STRING_FILENAME]);

	//Update the savefile and memcards stuff
	details->valid_vmu_screens = crayon_savefile_get_valid_screens();

	details->save_device_id = -1;

	details->history = NULL;
	details->history_tail = NULL;

	return 0;
}

uint8_t crayon_savefile_set_string(crayon_savefile_details_t *details, const char *string, uint8_t string_id){
	uint16_t max_length = crayon_savefile_detail_string_length(string_id);
	if(max_length == 0){return 1;}

	strlcpy(details->strings[string_id], string, max_length);
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

	if(fread(details->eyecatcher_data, size_data, 1, eyecatcher_data_file) != 1){
		return 1;
	}
	fclose(eyecatcher_data_file);

	#endif

	return 0;
}

crayon_savefile_history_t *crayon_savefile_add_variable(crayon_savefile_details_t *details, void *data_ptr,
	uint8_t data_type, uint16_t length, const void *default_value, crayon_savefile_version_t version){
	//data_type id doesn't map to any of our types
	if(data_type > CRAY_TYPE_CHAR){
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
		case CRAY_TYPE_UINT8:
			var->data_ptr.u8 = (uint8_t**)data_ptr;
			var->default_value.u8 = *((uint8_t*)default_value);
			break;
		case CRAY_TYPE_UINT16:
			var->data_ptr.u16 = (uint16_t**)data_ptr;
			var->default_value.u16 = *((uint16_t*)default_value);
			break;
		case CRAY_TYPE_UINT32:
			var->data_ptr.u32 = (uint32_t**)data_ptr;
			var->default_value.u32 = *((uint32_t*)default_value);
			break;
		case CRAY_TYPE_SINT8:
			var->data_ptr.s8 = (int8_t**)data_ptr;
			var->default_value.s8 = *((int8_t*)default_value);
			break;
		case CRAY_TYPE_SINT16:
			var->data_ptr.s16 = (int16_t**)data_ptr;
			var->default_value.s16 = *((int16_t*)default_value);
			break;
		case CRAY_TYPE_SINT32:
			var->data_ptr.s32 = (int32_t**)data_ptr;
			var->default_value.s32 = *((int32_t*)default_value);
			break;
		case CRAY_TYPE_FLOAT:
			var->data_ptr.floats = (float**)data_ptr;
			var->default_value.floats = *((float*)default_value);
			break;
		case CRAY_TYPE_DOUBLE:
			var->data_ptr.doubles = (double**)data_ptr;
			var->default_value.doubles = *((double*)default_value);
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

void crayon_savefile_solidify(crayon_savefile_details_t *details){
	uint16_t *lengths = details->save_data.lengths;
	uint16_t indexes[9] = {0};

	//Don't both allocating space for these if we aren't using them
	if(lengths[CRAY_TYPE_UINT8]){details->save_data.u8 = malloc(sizeof(uint8_t) * lengths[CRAY_TYPE_UINT8]);}
	if(lengths[CRAY_TYPE_UINT16]){details->save_data.u16 = malloc(sizeof(uint16_t) * lengths[CRAY_TYPE_UINT16]);}
	if(lengths[CRAY_TYPE_UINT32]){details->save_data.u32 = malloc(sizeof(uint32_t) * lengths[CRAY_TYPE_UINT32]);}
	if(lengths[CRAY_TYPE_SINT8]){details->save_data.s8 = malloc(sizeof(int8_t) * lengths[CRAY_TYPE_SINT8]);}
	if(lengths[CRAY_TYPE_SINT16]){details->save_data.s16 = malloc(sizeof(int16_t) * lengths[CRAY_TYPE_SINT16]);}
	if(lengths[CRAY_TYPE_SINT32]){details->save_data.s32 = malloc(sizeof(int32_t) * lengths[CRAY_TYPE_SINT32]);}
	if(lengths[CRAY_TYPE_FLOAT]){details->save_data.floats = malloc(sizeof(float) * lengths[CRAY_TYPE_FLOAT]);}
	if(lengths[CRAY_TYPE_DOUBLE]){details->save_data.doubles = malloc(sizeof(double) * lengths[CRAY_TYPE_DOUBLE]);}
	if(lengths[CRAY_TYPE_CHAR]){details->save_data.chars = malloc(sizeof(char) * lengths[CRAY_TYPE_CHAR]);}

	crayon_savefile_history_t *var = details->history;
	while(var){
		if(var->version_removed == 0){	//We only give space to vars that still exist
			switch(var->data_type){
				case CRAY_TYPE_UINT8:
					*var->data_ptr.u8 = &details->save_data.u8[indexes[var->data_type]];
					break;
				case CRAY_TYPE_UINT16:
					*var->data_ptr.u16 = &details->save_data.u16[indexes[var->data_type]];
					break;
				case CRAY_TYPE_UINT32:
					*var->data_ptr.u32 = &details->save_data.u32[indexes[var->data_type]];
					break;
				case CRAY_TYPE_SINT8:
					*var->data_ptr.s8 = &details->save_data.s8[indexes[var->data_type]];
					break;
				case CRAY_TYPE_SINT16:
					*var->data_ptr.s16 = &details->save_data.s16[indexes[var->data_type]];
					break;
				case CRAY_TYPE_SINT32:
					*var->data_ptr.s32 = &details->save_data.s32[indexes[var->data_type]];
					break;
				case CRAY_TYPE_FLOAT:
					*var->data_ptr.floats = &details->save_data.floats[indexes[var->data_type]];
					break;
				case CRAY_TYPE_DOUBLE:
					*var->data_ptr.doubles = &details->save_data.doubles[indexes[var->data_type]];
					break;
				case CRAY_TYPE_CHAR:
					*var->data_ptr.chars = &details->save_data.chars[indexes[var->data_type]];
				break;
			}
			indexes[var->data_type] += var->data_length;
		}

		var = var->next;
	}

	details->save_size = (lengths[CRAY_TYPE_UINT8] * sizeof(uint8_t)) +
		(lengths[CRAY_TYPE_UINT16] * sizeof(uint16_t)) +
		(lengths[CRAY_TYPE_UINT32] * sizeof(uint32_t)) +
		(lengths[CRAY_TYPE_SINT8] * sizeof(int8_t)) +
		(lengths[CRAY_TYPE_SINT16] * sizeof(int16_t)) +
		(lengths[CRAY_TYPE_SINT32] * sizeof(int32_t)) +
		(lengths[CRAY_TYPE_FLOAT] * sizeof(float)) +
		(lengths[CRAY_TYPE_DOUBLE] * sizeof(double)) +
		(lengths[CRAY_TYPE_CHAR] * sizeof(char));

	crayon_savefile_update_valid_saves(details, CRAY_SAVEFILE_UPDATE_MODE_BOTH);	//Need to double check this
	return;
}

void crayon_savefile_update_valid_saves(crayon_savefile_details_t *details, uint8_t modes){
	uint8_t valid_saves = 0;	//a1a2b1b2c1c2d1d2
	uint8_t valid_memcards = 0;	//Note: These are memcards that either can or do contain savefiles

	uint8_t get_saves = 0;
	uint8_t get_memcards = 0;

	vec2_s8_t port_and_slot;

	if(modes & CRAY_SAVEFILE_UPDATE_MODE_SAVE_PRESENT){
		get_saves = 1;
	}
	if(modes & CRAY_SAVEFILE_UPDATE_MODE_MEMCARD_PRESENT){
		get_memcards = 1;
	}

	int i;
	for(i = 0; i < CRAY_SF_NUM_SAVE_DEVICES; i++){
		//Check if device is a memory card
		if(crayon_savefile_check_device_for_function(CRAY_SF_MEMCARD, i)){
			continue;
		}

		//Check if a save file DNE. Returns 1 if DNE
		if(crayon_savefile_check_savedata(details, i)){
			if(!get_memcards){continue;}

			port_and_slot = crayon_savefile_dreamcast_get_port_and_slot(i);
			if(crayon_savefile_device_free_blocks(port_and_slot.x, port_and_slot.y) >=
				crayon_savefile_get_save_block_count(details)){
				crayon_savefile_set_memcard_bit(&valid_memcards, i);
			}
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
	FILE *fp;

	//If you call everying in the right order, this is redundant. But just incase you didn't, here it is
	//Also we use check for a memory card and not a savefile because the rest of the load code can automatically
	//check if a save exists so its faster this way (Since this function and crayon_savefile_check_savedata()
	//share alot of the same code)
	if(crayon_savefile_check_device_for_function(CRAY_SF_MEMCARD, details->save_device_id)){
		return 1;
	}

	#if defined(_arch_dreamcast)
	vec2_s8_t port_and_slot = crayon_savefile_dreamcast_get_port_and_slot(details->save_device_id);

	//port/port_and_slot.x gets converted to a, b, c or d. port_and_slot.y is the slot (0 or 1)
	//Its more efficient to do it this way than with sprintf
	char *savename = malloc(64);
	if(!savename){
		return 1;
	}

	strcpy(savename, __savefile_base_path);
	savename[__savefile_base_path_length    ] = port_and_slot.x + 'a';
	savename[__savefile_base_path_length + 1] = port_and_slot.y + '0';
	savename[__savefile_base_path_length + 2] = '/';
	savename[__savefile_base_path_length + 3] = '\0';
	strlcat(savename, details->strings[CRAY_SF_STRING_FILENAME], 64);

	#else

	char *savename = malloc(__savefile_base_path_length +
		crayon_savefile_detail_string_length(CRAY_SF_STRING_FILENAME) + 1);
	if(!savename){
		return 1;
	}

	strcpy(savename, __savefile_base_path);
	strcat(savename, details->strings[CRAY_SF_STRING_FILENAME]);

	#endif

	//If the savefile DNE, this will fail
	if(!(fp = fopen(savename, "rb"))){
		free(savename);
		return 1;
	}

	fseek(fp, 0, SEEK_END);
	int pkg_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	uint8_t *pkg_out = malloc(pkg_size);
	if(!pkg_out){
		fclose(fp);
		return 1;
	}
	fread(pkg_out, pkg_size, 1, fp);
	fclose(fp);

	#if defined(_arch_dreamcast)

	vmu_pkg_t pkg;
	vmu_pkg_parse(pkg_out, &pkg);

	//Read the pkg data into my struct
	crayon_savefile_deserialise(&details->save_data, (uint8_t *)pkg.data, (uint32_t)pkg.data_len);

	#else

	//Read the pkg data into my struct
	//We add CRAY_SF_HDR_SIZE to skip the header
	crayon_savefile_deserialise(&details->save_data, pkg_out + CRAY_SF_HDR_SIZE, pkg_size - CRAY_SF_HDR_SIZE);

	#endif

	free(pkg_out);
	return 0;
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

	FILE *fp;

	#if defined(_arch_dreamcast)
	vec2_s8_t port_and_slot = crayon_savefile_dreamcast_get_port_and_slot(details->save_device_id);

	//port/port_and_slot.x gets converted to a, b, c or d. port_and_slot.y is the slot (0 or 1)
	//Its more efficient to do it this way than with sprintf
	char *savename = malloc(64);
	if(!savename){
		return 1;
	}

	strcpy(savename, __savefile_base_path);
	savename[__savefile_base_path_length    ] = port_and_slot.x + 'a';
	savename[__savefile_base_path_length + 1] = port_and_slot.y + '0';
	savename[__savefile_base_path_length + 2] = '/';
	savename[__savefile_base_path_length + 3] = '\0';
	strlcat(savename, details->strings[CRAY_SF_STRING_FILENAME], 64);
	#else

	char *savename = malloc(__savefile_base_path_length +
		crayon_savefile_detail_string_length(CRAY_SF_STRING_FILENAME) + 1);
	if(!savename){
		return 1;
	}

	strcpy(savename, __savefile_base_path);
	strcat(savename, details->strings[CRAY_SF_STRING_FILENAME]);

	#endif

	uint8_t *data = malloc(details->save_size);
	if(!data){
		free(savename);
		return 1;
	}

	crayon_savefile_serialise(&details->save_data, data);

	#if defined(_arch_dreamcast)
	vmu_pkg_t pkg;
	sprintf(pkg.desc_long, details->strings[CRAY_SF_STRING_LONG_DESC]);
	strlcpy(pkg.desc_short, details->strings[CRAY_SF_STRING_SHORT_DESC], 16);
	strlcpy(pkg.app_id, details->strings[CRAY_SF_STRING_APP_ID], 16);
	pkg.icon_cnt = details->icon_anim_count;
	pkg.icon_anim_speed = details->icon_anim_speed;
	memcpy(pkg.icon_pal, details->icon_palette, 32);
	pkg.icon_data = details->icon_data;
	pkg.eyecatch_type = details->eyecatcher_type;
	if(pkg.eyecatch_type){
		pkg.eyecatch_data = details->eyecatcher_data;
	}
	pkg.data_len = details->save_size;
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
	if(crayon_savefile_device_free_blocks(port_and_slot.x, port_and_slot.y) + blocks_freed <
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

	uint8_t i;
	for(i = CRAY_SF_STRING_APP_ID; i < CRAY_SF_NUM_DETAIL_STRINGS; i++){
		fwrite(details->strings[CRAY_SF_STRING_APP_ID], sizeof(char), crayon_savefile_detail_string_length(i), fp);
	}
	fwrite(data, sizeof(uint8_t), details->save_size, fp);
	#endif

	fclose(fp);

	return 0;
}

uint8_t crayon_savefile_delete_savedata(crayon_savefile_details_t *details){
	//int vmufs_delete(maple_device_t *dev, const char *fn);
	return 1;
}

void crayon_savefile_free_base_path(){
	free(__savefile_base_path);
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
	if(details->save_data.u8){free(details->save_data.u8);}
	if(details->save_data.u16){free(details->save_data.u16);}
	if(details->save_data.u32){free(details->save_data.u32);}
	if(details->save_data.s8){free(details->save_data.s8);}
	if(details->save_data.s16){free(details->save_data.s16);}
	if(details->save_data.s32){free(details->save_data.s32);}
	if(details->save_data.floats){free(details->save_data.floats);}
	if(details->save_data.doubles){free(details->save_data.doubles);}
	if(details->save_data.chars){free(details->save_data.chars);}

	details->save_data.u8 = NULL;
	details->save_data.u16 = NULL;
	details->save_data.u32 = NULL;
	details->save_data.s8 = NULL;
	details->save_data.s16 = NULL;
	details->save_data.s32 = NULL;
	details->save_data.floats = NULL;
	details->save_data.doubles = NULL;
	details->save_data.chars = NULL;
	uint8_t i;
	for(i = 0; i < 9; i++){
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
