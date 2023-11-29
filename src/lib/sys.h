/*
 * @Description:
 * @Blog: saisaiwa.com
 * @Author: ccy
 * @Date: 2023-11-02 11:23:32
 * @LastEditTime: 2023-11-03 10:19:27
 */
#ifndef __MSYS_H
#define __MSYS_H

#include <intrins.h>
#include "STC/STC8H.H"
#include "STDIO.H"
#include "STDLIB.H"
#include "STRING.H"

// #define DEV_PLATFROM

// #define SYS_FOSC 22118400UL  // 22.1184Mhz

typedef unsigned char uint8_t;
typedef unsigned int uint16_t;
typedef unsigned long uint32_t;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint8_t bool;
#define false 0
#define true 1

#define I2C_SDA P16
#define I2C_SCL P15
#define RX8025_INT P17
#define VFD_EN P11
#define P_STB P14
#define P_CLK P10
#define P_DIN P37

void hal_init_systick();
u32 hal_systick_get();
void hal_init_uart(void);
void hal_init_all_gpio(void);
void delay_ms(u32 ms);
void delay_us(u32 us);

#endif