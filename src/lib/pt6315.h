#ifndef __PT6315__
#define __PT6315__

#include "sys.h"

#define CLK_1 P_CLK = 1
#define CLK_0 P_CLK = 0
#define DIN_1 P_DIN = 1
#define DIN_0 P_DIN = 0
#define STB_1 P_STB = 1
#define STB_0 P_STB = 0

void ptSetDisplayLight(uint8_t onOff, uint8_t brightnessVal);
void setModeWirteDisplayMode(uint8_t addressMode);
void setDisplayMode(uint8_t digit);
void sendDigAndData(uint8_t dig, uint8_t* dat, size_t len);
#endif