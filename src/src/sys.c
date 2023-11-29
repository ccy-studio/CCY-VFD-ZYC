#include "sys.h"

u32 data _systick_ccr = 0;

#ifdef DEV_PLATFROM

bit busy = 0;
/**
 * 关键字code是51单片机特有关键字
 * 而运用code关键字修饰下定义的变量，比如unsigned char code i；
 * 它们则存储在单片机程序存储空间FLASH中，节省单片机RAM资源，但在程序中不能更改这些变量的值。
 */
char* code STCISPCMD = "@STCISP#";
u8 rx_index = 0;

void hal_init_uart(void) {
    SCON = 0x50;   // 8位数据,可变波特率
    AUXR |= 0x40;  // 定时器时钟1T模式
    AUXR &= 0xFE;  // 串口1选择定时器1为波特率发生器
    TMOD &= 0x0F;  // 设置定时器模式
    TL1 = 0xD0;    // 设置定时初始值
    TH1 = 0xFF;    // 设置定时初始值
    ET1 = 0;       // 禁止定时器中断
    TR1 = 1;       // 定时器1开始计时
    ES = 1;        // 使能串口1中断
}
void hal_uart_isr() interrupt 4 {
    char dat;
    if (TI) {
        TI = 0;
        busy = 0;
    }
    if (RI) {
        RI = 0;
        dat = SBUF;
        if (dat == STCISPCMD[rx_index]) {
            rx_index++;
            if (STCISPCMD[rx_index] == '\0') {
                IAP_CONTR = 0x60;  // 软复位到ISP进行下载
            }
        } else {
            // 不匹配重新开始
            rx_index++;
            if (dat == STCISPCMD[rx_index]) {
                rx_index++;
            }
        }
    }
}
char putchar(char ch) {
    while (busy)
        ;
    busy = 1;
    SBUF = ch;
    return ch;
}
#else
void hal_init_uart(void) {}
#endif

void hal_init_systick() {
    // 1毫秒@22.1184MHz
    AUXR |= 0x80;  // 定时器时钟1T模式
    TMOD = 0xF3;   // 设置定时器模式
    TL0 = 0x9A;    // 设置定时初始值
    TH0 = 0xA9;    // 设置定时初始值
    TF0 = 0;       // 清除TF0标志
    TR0 = 1;       // 定时器0开始计时
    ET0 = 1;       // 使能定时器0中断
}

u32 hal_systick_get() {
    return _systick_ccr;
}

void timer0_Isr(void) interrupt 1 {
    _systick_ccr++;
    TF0 = 0;  // 清除TF0标志
}

void hal_init_all_gpio(void) {
    P3M0 = 0x00;
    P3M1 = 0x78;
    P1M0 = 0x02;
    P1M1 = 0x00;
    // I2C内部上拉
    P1PU = 0xE0;
    // Key内部上拉
    P3PU = 0x78;
    EA = 1;  // 开总中断
}

void delay_ms(u32 ms) {
    unsigned char data i, j;
    do {
        i = 29;
        j = 183;
        do {
            while (--j)
                ;
        } while (--i);
    } while (--ms);
}

void delay_us(u32 us) {
    unsigned char data i;

    do {
        _nop_();
        i = 5;
        while (--i)
            ;
    } while (--us);
}