#ifndef MS_SETUP_H
#define MS_SETUP_H

#include "extra_structs.h"
#include "crayon.h"

#include <stdlib.h>
#include <string.h>

uint8_t setup_savefile(crayon_savefile_details_t * details);

int16_t setup_vmu_icon_load(uint8_t ** vmu_lcd_icon, char * icon_path);
void setup_vmu_icon_apply(uint8_t * vmu_lcd_icon, uint8_t valid_vmu_screens);

#endif
