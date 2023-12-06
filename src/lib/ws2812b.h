#ifndef __WS2812B_H
#define __WS2812B_H

#include "sys.h"

#define RGB_LED_COUNT 2  // WS2812B 

void rgb_init();
void rgb_set_color(u8 index, u8 r, u8 g, u8 b);
void rgb_update(u8 brightness);
void rgb_clear();
void rgb_frame_update(u8 brightness,u8 type);

#endif