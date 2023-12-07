#include "sys.h"

u32 data _systick_ccr = 0;

void hal_init_systick() {
    // 1����@24.000MHz
    AUXR |= 0x80;  // ��ʱ��ʱ��1Tģʽ
    TMOD = 0xF3;   // ���ö�ʱ��ģʽ
    TL0 = 0x40;    // ���ö�ʱ��ʼֵ
    TH0 = 0xA2;    // ���ö�ʱ��ʼֵ
    TF0 = 0;       // ���TF0��־
    TR0 = 1;       // ��ʱ��0��ʼ��ʱ
    ET0 = 1;       // ʹ�ܶ�ʱ��0�ж�
}

u32 hal_systick_get() {
    return _systick_ccr;
}

void timer0_Isr(void) interrupt 1 {
    _systick_ccr++;
    TF0 = 0;  // ���TF0��־
}

void hal_init_all_gpio(void) {
    P1M0 = 0xf8;
    P1M1 = 0x33;
    P1PU = 0x33;

    P3M0 = 0x00;
    P3M1 = 0x80;
    P3PU = 0x80;
    P3SR = 0xef;

    P5M0 = 0x10;
    P5M1 = 0x00;

    EA = 1;  // �����ж�
}

void delay_ms(u32 ms) {
    unsigned char data i, j;
    do {
        i = 24;
        j = 85;
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
        _nop_();
        i = 3;
        while (--i)
            ;
    } while (--us);
}

u8 btn_gpio_read(btn_gpio_t gpio) {
    if (gpio == BTN_P) {
        return P37;
    } else if (gpio == BTN_S) {
        return P10;
    } else if (gpio == BTN_M) {
        return P11;
    }
    return 0;
}