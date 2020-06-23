#include "savefile.h"

//Note this assumes the vmu chosen is valid
	//THIS CAN BE OPTIMISED
uint8_t crayon_savefile_check_for_save(crayon_savefile_details_t * details, int8_t save_port, int8_t save_slot){
	vmu_pkg_t pkg;
	uint8 *pkg_out;
	int pkg_size;
	FILE *fp;

	//Only 25 charaters allowed at max (26 if you include '\0')
	//port gets converted to a, b, c or d. unit is unit
	//Its more efficient to do it this way than with sprintf
	char savename[32] = "/vmu/";
	savename[5] = save_port + 'a';
	savename[6] = save_slot + '0';
	savename[7] = '/';
	savename[8] = '\0';
	strcat(savename, details->save_name);

	//File DNE
	if(!(fp = fopen(savename, "rb"))){
		return 1;
	}

	fseek(fp, 0, SEEK_END);
	pkg_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	//Surely instead of doing the below I can just read the header and hence the app id?

	pkg_out = (uint8_t *)malloc(pkg_size);
	fread(pkg_out, pkg_size, 1, fp);
	fclose(fp);

	vmu_pkg_parse(pkg_out, &pkg);

	free(pkg_out);

	//If the IDs don't match, then thats an error
	if(strcmp(pkg.app_id, details->app_id)){
		return 2;
	}
	return 0;
}

//Returns true if device has certain function/s
uint8_t crayon_savefile_check_for_device(int8_t port, int8_t slot, uint32_t function){
	maple_device_t *vmu;

	//Invalid controller/port
	if(port < 0 || port > 3 || slot < 1 || slot > 2){
		return 1;
	}

	//Make sure there's a device in the port/slot
	if(!(vmu = maple_enum_dev(port, slot))){
		return 2;
	}

	//Check the device is valid and it has a certain function
	if(!vmu->valid || !(vmu->info.functions & function)){
		return 3;
	}

	return 0;
}

//Rounds the number down to nearest multiple of 512, then adds 1 if there's a remainder
uint16_t crayon_savefile_bytes_to_blocks(size_t bytes){
	return (bytes >> 9) + !!(bytes & ((1 << 9) - 1));
}

//I used the "vmu_pkg_build" function's source as a guide for this. Doesn't work with compressed saves
int16_t crayon_savefile_get_save_block_count(crayon_savefile_details_t * details){
	int pkg_size, data_len;
	data_len = details->save_size;
	uint16_t eyecatch_size  = 0;
	switch(details->eyecatch_type){
		case VMUPKG_EC_NONE:
			eyecatch_size = 0; break;
		case VMUPKG_EC_16BIT:
			eyecatch_size = 8064; break;
		case VMUPKG_EC_256COL:
			eyecatch_size = 512 + 4032; break;
		case VMUPKG_EC_16COL:
			eyecatch_size = 32 + 2016; break;
		default:
			return -1;
	}

	//Get the total number of bytes. Keep in mind we need to think about the icon/s and EC
	pkg_size = sizeof(vmu_hdr_t) + (512 * details->icon_anim_count) +
		eyecatch_size + data_len;

	return crayon_savefile_bytes_to_blocks(pkg_size);
}

//DON'T FORGET THE PKG DATA ALSO CONTAINS THE VERSION NUMBER
void crayon_savefile_serialise(crayon_savefile_data_t * sf_data, uint8_t * pkg_data){
	return;
}

void crayon_savefile_deserialise(crayon_savefile_data_t * sf_data, uint8_t * pkg_data, uint32_t pkg_size){
	return;
}

uint8_t crayon_savefile_get_vmu_bit(uint8_t vmu_bitmap, int8_t save_port, int8_t save_slot){
	return !!(vmu_bitmap & (1 << ((2 - save_slot) + 6 - (2 * save_port))));
}

void crayon_savefile_set_vmu_bit(uint8_t * vmu_bitmap, int8_t save_port, int8_t save_slot){
	*vmu_bitmap |= (1 << ((2 - save_slot) + 6 - (2 * save_port)));
	return;
}

//Make sure to call this first (And call the save icon and eyecatcher functions after since this overides them)
void crayon_savefile_init_savefile_details(crayon_savefile_details_t * details, char * desc_long, char * desc_short,
	char * app_id, char * save_name, crayon_savefile_version_t version){
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
	details->eyecatch_type = VMUPKG_EC_NONE;	//The default

	strlcpy(details->desc_long, desc_long, 32);
	strlcpy(details->desc_short, desc_short, 16);
	strlcpy(details->app_id, app_id, 16);
	strlcpy(details->save_name, save_name, 26);

	//Update the savefile and memcards stuff
	details->valid_vmu_screens = crayon_savefile_get_valid_screens();

	details->save_port = -1;
	details->save_slot = -1;

	details->history = NULL;
	details->history_tail = NULL;

	return;
}

uint8_t crayon_savefile_add_icon(crayon_savefile_details_t * details, char * image, char * palette,
	uint8_t icon_anim_count, uint16_t icon_anim_speed){
	FILE * icon_data_file;
	int size_data;

	if(icon_anim_count > 3){
		return 1;
	}

	icon_data_file = fopen(image, "rb");

	//Get the size of the file
	fseek(icon_data_file, 0, SEEK_END);
	size_data = ftell(icon_data_file);	//This will account for multi-frame icons
	fseek(icon_data_file, 0, SEEK_SET);

	details->icon = (unsigned char *) malloc(size_data);
	fread(details->icon, size_data, 1, icon_data_file);
	fclose(icon_data_file);


	//--------------------------------

	FILE * icon_palette_file;
	int size_palette;

	icon_palette_file = fopen(palette, "rb");

	fseek(icon_palette_file, 0, SEEK_END);
	size_palette = ftell(icon_palette_file);
	fseek(icon_palette_file, 0, SEEK_SET);

	details->icon_palette = (unsigned short *) malloc(size_palette);
	fread(details->icon_palette, size_palette, 1, icon_palette_file);
	fclose(icon_palette_file);

	details->icon_anim_count = icon_anim_count;
	details->icon_anim_speed = icon_anim_speed;

	return 0;
}

uint8_t crayon_savefile_add_eyecatcher(crayon_savefile_details_t * details, char * eyecatch_path){
	FILE * eyecatch_data_file = fopen(eyecatch_path, "rb");
	if(!eyecatch_data_file){
		return 1;
	}

	//Get the size of the file
	fseek(eyecatch_data_file, 0, SEEK_END);
	int size_data = ftell(eyecatch_data_file);	//Confirming its the right size
	fseek(eyecatch_data_file, 0, SEEK_SET);

	switch(size_data){
		case 8064:
			details->eyecatch_type = VMUPKG_EC_16BIT; break;
		case 4544:
			details->eyecatch_type = VMUPKG_EC_256COL; break;
		case 2048:
			details->eyecatch_type = VMUPKG_EC_16COL; break;
		default:
			return 2;
	}

	details->eyecatch_data = (uint8_t *) malloc(size_data);
	if(fread(details->eyecatch_data, size_data, 1, eyecatch_data_file) != 1){
		return 4;
	}
	fclose(eyecatch_data_file);

	return 0;
}

crayon_savefile_history_t * crayon_savefile_add_variable(crayon_savefile_details_t * details, void * data_ptr,
	uint8_t data_type, uint16_t length, const void * default_value, crayon_savefile_version_t version){

	if(data_type > CRAY_TYPE_CHAR){
		return NULL;
	}

	//Add the new variable
	crayon_savefile_history_t * var = malloc(sizeof(crayon_savefile_history_t));
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
			var->data_ptr.u8 = (uint8_t*)data_ptr;
			var->default_value.u8 = *((uint8_t*)default_value);
			break;
		case CRAY_TYPE_UINT16:
			var->data_ptr.u16 = (uint16_t*)data_ptr;
			var->default_value.u16 = *((uint16_t*)default_value);
			break;
		case CRAY_TYPE_UINT32:
			var->data_ptr.u32 = (uint32_t*)data_ptr;
			var->default_value.u32 = *((uint32_t*)default_value);
			break;
		case CRAY_TYPE_SINT8:
			var->data_ptr.s8 = (int8_t*)data_ptr;
			var->default_value.s8 = *((int8_t*)default_value);
			break;
		case CRAY_TYPE_SINT16:
			var->data_ptr.s16 = (int16_t*)data_ptr;
			var->default_value.s16 = *((int16_t*)default_value);
			break;
		case CRAY_TYPE_SINT32:
			var->data_ptr.s32 = (int32_t*)data_ptr;
			var->default_value.s32 = *((int32_t*)default_value);
			break;
		case CRAY_TYPE_FLOAT:
			var->data_ptr.floats = (float*)data_ptr;
			var->default_value.floats = *((float*)default_value);
			break;
		case CRAY_TYPE_DOUBLE:
			var->data_ptr.doubles = (double*)data_ptr;
			var->default_value.doubles = *((double*)default_value);
			break;
		case CRAY_TYPE_CHAR:
			var->data_ptr.chars = (char*)data_ptr;
			var->default_value.chars = *((char*)default_value);
			break;
	}

	return var;
}

crayon_savefile_history_t * crayon_savefile_remove_variable(crayon_savefile_details_t * details,
	crayon_savefile_history_t * target_node, uint8_t remove_command, crayon_savefile_history_t * transfer_var,
	crayon_savefile_version_t version){

	crayon_savefile_history_t * var = details->history;
	uint8_t found;

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

	return NULL;
}

void crayon_savefile_solidify(crayon_savefile_details_t * details){
	uint16_t * lengths = details->save_data.lengths;
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

	//STILL A WIP
	crayon_savefile_history_t * var = details->history;
	while(var){
		if(var->version_removed == 0){	//We only give space to vars that still exist
			switch(var->data_type){
				case CRAY_TYPE_UINT8:
					*var->data_ptr.u8 = details->save_data.u8[indexes[var->data_type]];
					break;
				case CRAY_TYPE_UINT16:
					*var->data_ptr.u16 = details->save_data.u16[indexes[var->data_type]];
					break;
				case CRAY_TYPE_UINT32:
					*var->data_ptr.u32 = details->save_data.u32[indexes[var->data_type]];
					break;
				case CRAY_TYPE_SINT8:
					*var->data_ptr.s8 = details->save_data.s8[indexes[var->data_type]];
					break;
				case CRAY_TYPE_SINT16:
					*var->data_ptr.s16 = details->save_data.s16[indexes[var->data_type]];
					break;
				case CRAY_TYPE_SINT32:
					*var->data_ptr.s32 = details->save_data.s32[indexes[var->data_type]];
					break;
				case CRAY_TYPE_FLOAT:
					*var->data_ptr.floats = details->save_data.floats[indexes[var->data_type]];
					break;
				case CRAY_TYPE_DOUBLE:
					*var->data_ptr.doubles = details->save_data.doubles[indexes[var->data_type]];
					break;
				case CRAY_TYPE_CHAR:
					*var->data_ptr.chars = details->save_data.chars[indexes[var->data_type]];
				break;
			}
			indexes[var->data_type] += var->data_length;
		}
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

void crayon_savefile_update_valid_saves(crayon_savefile_details_t * details, uint8_t modes){
	maple_device_t *vmu;
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

	int i, j;
	for(i = 0; i <= 3; i++){
		for(j = 1; j <= 2; j++){
			//Check if device is a memory card
			if(crayon_savefile_check_for_device(i, j, MAPLE_FUNC_MEMCARD)){
				continue;
			}

			//Check if a save file DNE. Returns 1 if DNE
			if(crayon_savefile_check_for_save(details, i, j)){
				if(!get_memcards){continue;}

				//If we have enough space for a new savefile
				vmu = maple_enum_dev(i, j);
				if(vmufs_free_blocks(vmu) >= crayon_savefile_get_save_block_count(details)){
					crayon_savefile_set_vmu_bit(&valid_memcards, i, j);
				}
			}
			else{
				if(get_memcards){crayon_savefile_set_vmu_bit(&valid_memcards, i, j);}
				if(get_saves){crayon_savefile_set_vmu_bit(&valid_saves, i, j);}
			}
		}
	}

	if(get_saves){details->valid_saves = valid_saves;}
	if(get_memcards){details->valid_memcards = valid_memcards;}

	return;
}

uint8_t crayon_savefile_get_valid_function(uint32_t function){
	uint8_t valid_function = 0;	//a1a2b1b2c1c2d1d2

	int i, j;
	for(i = 0; i <= 3; i++){
		for(j = 1; j <= 2; j++){
			//Check if device contains this function bitmap
			if(crayon_savefile_check_for_device(i, j, function)){
				continue;
			}

			//If we got here, we have a screen
			crayon_savefile_set_vmu_bit(&valid_function, i, j);
		}
	}

	return valid_function;
}

uint8_t crayon_savefile_load(crayon_savefile_details_t * details){
	vmu_pkg_t pkg;
	uint8 *pkg_out;
	int pkg_size;
	FILE *fp;

	//If you call everying in the right order, this is redundant. But just incase you didn't, here it is
	//Also we use device and not save because the rest of the load code can automatically check if a save exists so its faster this way
	//(Since this function and check_for_save share alot of the same code)
	if(crayon_savefile_check_for_device(details->save_port, details->save_slot, MAPLE_FUNC_MEMCARD)){
		return 1;
	}

	//Only 25 charaters allowed at max (26 if you include '\0')
	//port gets converted to a, b, c or d. unit is unit
	//Its more efficient to do it this way than with sprintf
	char savename[32] = "/vmu/";
	savename[5] = details->save_port + 'a';
	savename[6] = details->save_slot + '0';
	savename[7] = '/';
	savename[8] = '\0';
	strlcat(savename, details->save_name, 32);

	//If the savefile DNE, this will fail
	if(!(fp = fopen(savename, "rb"))){
		return 2;
	}

	fseek(fp, 0, SEEK_END);
	pkg_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	pkg_out = (uint8_t *)malloc(pkg_size);
	fread(pkg_out, pkg_size, 1, fp);
	fclose(fp);

	vmu_pkg_parse(pkg_out, &pkg);

	//Read the pkg data into my struct
	crayon_savefile_deserialise(&details->save_data, (uint8_t *)pkg.data, (uint32_t)pkg.data_len);
	free(pkg_out);

	return 0;
}

uint8_t crayon_savefile_save(crayon_savefile_details_t * details){
	//The requested VMU is not a valid memory card
	if(!crayon_savefile_get_vmu_bit(details->valid_memcards, details->save_port, details->save_slot)){
		return 1;
	}

	//If you call everying in the right order, this is redundant
	//But just incase you didn't, here it is
	if(crayon_savefile_check_for_device(details->save_port, details->save_slot, MAPLE_FUNC_MEMCARD)){
		return 2;
	}

	vmu_pkg_t pkg;
	uint8_t *pkg_out, *data;	//pkg_out is allocated in vmu_pkg_build
	int pkg_size;
	FILE *fp;
	file_t f;
	maple_device_t *vmu;
	uint16_t blocks_freed = 0;
	uint8_t rv = 0;

	vmu = maple_enum_dev(details->save_port, details->save_slot);

	//Only 25 charaters allowed at max (26 if you include '\0')
	//port gets converted to a, b, c or d. unit is unit
	//Its more efficient to do it this way than with sprintf
	char savename[32] = "/vmu/";
	savename[5] = details->save_port + 'a';
	savename[6] = details->save_slot + '0';
	savename[7] = '/';
	savename[8] = '\0';
	strcat(savename, details->save_name);

	int filesize = details->save_size;
	data = (uint8_t *) malloc(filesize);
	if(data == NULL){
		free(data);
		return 3;
	}

	crayon_savefile_serialise(&details->save_data, data);

	sprintf(pkg.desc_long, details->desc_long);
	strlcpy(pkg.desc_short, details->desc_short, 16);
	strlcpy(pkg.app_id, details->app_id, 16);
	pkg.icon_cnt = details->icon_anim_count;
	pkg.icon_anim_speed = details->icon_anim_speed;
	memcpy(pkg.icon_pal, details->icon_palette, 32);
	pkg.icon_data = details->icon;
	pkg.eyecatch_type = details->eyecatch_type;
	if(pkg.eyecatch_type){
		pkg.eyecatch_data = details->eyecatch_data;
	}
	pkg.data_len = details->save_size;
	pkg.data = data;

	vmu_pkg_build(&pkg, &pkg_out, &pkg_size);

	//Check if a file exists with that name, since we'll overwrite it.
	f = fs_open(savename, O_RDONLY);
	if(f != FILEHND_INVALID){
		int fs_size = fs_total(f);
		blocks_freed = crayon_savefile_bytes_to_blocks(fs_size);;
		fs_close(f);
	}

	//Make sure there's enough free space on the VMU.
	if(vmufs_free_blocks(vmu) + blocks_freed < crayon_savefile_bytes_to_blocks(pkg_size)){
		free(pkg_out);
		free(data);
		return 4;
	}

	//Can't open file for some reason
	if(!(fp = fopen(savename, "wb"))){
		free(pkg_out);
		free(data);
		return 5;
	}

	if(fwrite(pkg_out, 1, pkg_size, fp) != (size_t)pkg_size){
		rv = 6;
	}

	fclose(fp);

	free(pkg_out);
	free(data);

	return rv;
}

void crayon_savefile_free(crayon_savefile_details_t * details){
	crayon_savefile_free_icon(details);
	crayon_savefile_free_eyecatcher(details);

	//Free up history
	crayon_savefile_history_t * curr = details->history;
	crayon_savefile_history_t * prev = curr;
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

	return;
}

void crayon_savefile_free_icon(crayon_savefile_details_t * details){
	free(details->icon);
	free(details->icon_palette);
	return;
}

void crayon_savefile_free_eyecatcher(crayon_savefile_details_t * details){
	free(details->eyecatch_data);
	details->eyecatch_type = 0;
	return;
}

//Why is valid screens a part of Savefile details, but this function isn't?
void crayon_vmu_display_icon(uint8_t vmu_bitmap, void * icon){
	maple_device_t *vmu;
	uint8_t i, j;
	for(i = 0; i <= 3; i++){
		for(j = 1; j <= 2; j++){
			if(crayon_savefile_get_vmu_bit(vmu_bitmap, i, j)){	//We want to display on this VMU
				if(!(vmu = maple_enum_dev(i, j))){	//Device not present
					continue;
				}
				vmu_draw_lcd(vmu, icon);
			}
		}
	}

	return;
}
