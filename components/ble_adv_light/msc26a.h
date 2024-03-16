#pragma once

#include <stdint.h>
#include <string.h>

void get_adv_data(uint8_t* mfg_data, uint8_t* args, uint8_t cmd, uint8_t sn, const uint8_t* uuid);