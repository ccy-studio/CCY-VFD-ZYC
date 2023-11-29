/*
 * @Description:
 * @Blog: saisaiwa.com
 * @Author: ccy
 * @Date: 2023-09-04 10:46:28
 * @LastEditTime: 2023-10-10 15:40:52
 */
#ifndef __RX8025_H
#define __RX8025_H

#include "sys.h"

#define RX8025_ADDR 0x32

typedef struct {
    u8 year;
    u8 month;
    u8 day;
    u8 week;
    u8 hour;
    u8 min;
    u8 sec;
} rx8025_timeinfo;

/**
 * init
 */
void rx8025t_init();

/**
 * set timeinfo
 */
void rx8025_set_time(u8 year,
                     u8 month,
                     u8 day,
                     u8 week,
                     u8 hour,
                     u8 min,
                     u8 sec);

/**
 * get latest time
 */
void rx8025_time_get(rx8025_timeinfo* timeinfo);

/**
 * formart HHmmss
 */
void formart_time(rx8025_timeinfo* timeinfo, char* buf);
/**
 * formart YYMMdd
 */
void formart_date(rx8025_timeinfo* timeinfo, char* buf);

#endif