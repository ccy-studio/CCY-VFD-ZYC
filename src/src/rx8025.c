/*
 * @Description:
 * @Blog: saisaiwa.com
 * @Author: ccy
 * @Date: 2023-09-04 10:53:37
 * @LastEditTime: 2023-11-27 14:16:13
 */
#include <rx8025.h>

#define RX8025T_ADDR_W 0x64
#define RX8025T_ADDR_R 0x65

#define SDA_1 I2C_SDA = 1
#define SDA_0 I2C_SDA = 0

#define SCL_1 I2C_SCL = 1
#define SCL_0 I2C_SCL = 0

void i2c_sda_out() {
    P1M1 |= 0x40;
    P1M0 |= 0x40;
}

void i2c_sda_in() {
    P1M1 |= 0x40;
    P1M0 &= 0xbf;
}

void i2c_init() {
    P1M1 |= 0x20;
    P1M0 |= 0x20;
    P1M1 |= 0x80;
    P1M0 &= 0x7F;

    i2c_sda_out();
}

void i2c_start() {
    i2c_sda_out();
    SDA_1;
    SCL_1;
    delay_us(4);
    SDA_0;
    delay_us(4);
    SCL_0;
}

void i2c_stop() {
    i2c_sda_out();
    SCL_0;
    SDA_0;
    SCL_1;
    delay_us(4);
    SDA_1;
    delay_us(4);
}

void i2c_nack() {
    i2c_sda_out();
    SDA_1;
    SCL_0;
    delay_us(4);
    SCL_1;
    delay_us(4);
    SCL_0;
}

void i2c_ack() {
    i2c_sda_out();
    SDA_0;
    SCL_0;
    delay_us(4);
    SCL_1;
    delay_us(4);
    SCL_0;
}

u8 i2c_check_ack() {
    uint8_t ack = 0, errorRetry = 3;
    SDA_1;
    delay_us(1);
    i2c_sda_in();
    delay_us(1);
    SCL_1;
    delay_us(1);
    while (I2C_SDA) {
        if (errorRetry <= 0) {
            break;
        }
        delay_us(1);
        errorRetry--;
    }
    if (errorRetry) {
        ack = 1;
    }
    if (!ack) {
        i2c_stop();
    }
    SCL_0;
    return ack;
}

u8 i2c_write(u8 buf) {
    u8 i;
    i2c_sda_out();
    SCL_0;
    delay_us(4);
    for (i = 0; i < 8; i++) {
        if (buf & 0x80) {
            SDA_1;
        } else {
            SDA_0;
        }
        delay_us(4);
        SCL_1;
        delay_us(4);
        SCL_0;
        buf <<= 1;
    }
    return i2c_check_ack();
}

u8 i2c_read(u8 ack) {
    uint8_t receiveData = 0, i;
    i2c_sda_in();
    for (i = 0; i < 8; i++) {
        SCL_0;
        delay_us(4);
        SCL_1;
        receiveData <<= 1;
        if (I2C_SDA) {
            receiveData |= 0x01;
        }
        delay_us(1);
    }
    SCL_0;
    ack ? i2c_ack() : i2c_nack();
    return receiveData;
}

void rx8025_read(u8 address, u8* buf, u8 len) {
    u8 i;
    i2c_start();
    if (!i2c_write(RX8025T_ADDR_W)) {
        return;
    }
    if (!i2c_write(address)) {
        return;
    }
    i2c_start();
    if (!i2c_write(RX8025T_ADDR_R)) {
        return;
    }
    for (i = 0; i < len; i++) {
        buf[i] = i2c_read((i == len - 1) ? 0 : 1);
    }
    i2c_stop();
}

void rx8025_write(u8 address, u8* buf, u8 len) {
    u8 i;
    i2c_start();
    if (!i2c_write(RX8025T_ADDR_W)) {
        return;
    }
    if (!i2c_write(address)) {
        return;
    }
    for (i = 0; i < len; i++) {
        if (!i2c_write(buf[i])) {
            return;
        }
    }
    i2c_stop();
}

void rx8025t_init() {
    i2c_init();
}

u8 toBcd(u8 val) {
    return ((val / 10) << 4) | (val % 10);
}

u8 toDec(u8 bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

void rx8025_set_time(u8 year,
                     u8 month,
                     u8 day,
                     u8 week,
                     u8 hour,
                     u8 min,
                     u8 sec) {
    u8 command[7];
    i2c_init();
    command[0] = toBcd(sec);
    command[1] = toBcd(min);
    command[2] = toBcd(hour);
    command[3] = (0x00 >> week) | 0x01;
    command[4] = toBcd(day);
    command[5] = toBcd(month);
    command[6] = toBcd(year);
    rx8025_write(0x00, command, 7);
}

void rx8025_time_get(rx8025_timeinfo* timeinfo) {
    u8 buf[7];
    i2c_init();
    rx8025_read(0x00, buf, 7);
    timeinfo->sec = toDec(buf[0]);
    timeinfo->min = toDec(buf[1]);
    timeinfo->hour = toDec(buf[2]);
    timeinfo->day = toDec(buf[4]);
    timeinfo->month = toDec(buf[5]);
    timeinfo->year = toDec(buf[6]);
    if (timeinfo->year > 99) {
        timeinfo->year = 23;
    }
    if (timeinfo->month > 12) {
        timeinfo->year = 12;
    }
    timeinfo->week = (-35 + timeinfo->year + (timeinfo->year / 4) +
                      (13 * (timeinfo->month + 1) / 5) + timeinfo->day - 1) %
                     7;
    if (timeinfo->week > 7) {
        timeinfo->week = 7;
    }
    if (timeinfo->week == 0) {
        timeinfo->week = 7;
    }
    if (timeinfo->hour >= 24) {
        timeinfo->hour = 0;
    }
    if (timeinfo->min >= 60) {
        timeinfo->min = 0;
    }
    if (timeinfo->sec >= 60) {
        timeinfo->min = 0;
    }
}

void formart_time(rx8025_timeinfo* timeinfo, char* buf) {
    sprintf(buf, "W%bd %02bd%02bd%02bd", timeinfo->week, timeinfo->hour,
            timeinfo->min, timeinfo->sec);
}

/**
 *  YYMMdd
 */
void formart_date(rx8025_timeinfo* timeinfo, char* buf) {
    sprintf(buf, "20%bd %02bd%02bd", timeinfo->year, timeinfo->month,
            timeinfo->day);
}