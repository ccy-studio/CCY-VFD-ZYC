/*
 * @Description:
 * @Blog: saisaiwa.com
 * @Author: ccy
 * @Date: 2023-11-02 11:23:32
 * @LastEditTime: 2023-11-30 16:37:04
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

typedef enum {
    BTN_P33 = 0,
    BTN_P34,
    BTN_P35,
} btn_gpio_t;

typedef enum {
    BTN_PRESS = 0,
    BTN_RELEASE,
    BTN_LONG,
} btn_state_t;

typedef struct {
    btn_gpio_t gpio;
    btn_state_t btn_type;
    u32 last_press_time;
    u8 falg;  // 是否被处理
} btn_t;
#define BTN_SCAN_MS 1
#define BTN_LONG_PRESS_MS 1000
#define BTN_LONG_DOUBLE_MS 500

void hal_init_systick();
u32 hal_systick_get();
void hal_init_uart(void);
void hal_init_all_gpio(void);
void delay_ms(u32 ms);
void delay_us(u32 us);
u8 btn_gpio_read(btn_gpio_t gpio);

#endif