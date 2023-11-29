#include "sys.h"

u32 data _systick_ccr = 0;

#ifdef DEV_PLATFROM

bit busy = 0;
/**
 * �ؼ���code��51��Ƭ�����йؼ���
 * ������code�ؼ��������¶���ı���������unsigned char code i��
 * ������洢�ڵ�Ƭ������洢�ռ�FLASH�У���ʡ��Ƭ��RAM��Դ�����ڳ����в��ܸ�����Щ������ֵ��
 */
char* code STCISPCMD = "@STCISP#";
u8 rx_index = 0;

void hal_init_uart(void) {
    SCON = 0x50;   // 8λ����,�ɱ䲨����
    AUXR |= 0x40;  // ��ʱ��ʱ��1Tģʽ
    AUXR &= 0xFE;  // ����1ѡ��ʱ��1Ϊ�����ʷ�����
    TMOD &= 0x0F;  // ���ö�ʱ��ģʽ
    TL1 = 0xD0;    // ���ö�ʱ��ʼֵ
    TH1 = 0xFF;    // ���ö�ʱ��ʼֵ
    ET1 = 0;       // ��ֹ��ʱ���ж�
    TR1 = 1;       // ��ʱ��1��ʼ��ʱ
    ES = 1;        // ʹ�ܴ���1�ж�
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
                IAP_CONTR = 0x60;  // ��λ��ISP��������
            }
        } else {
            // ��ƥ�����¿�ʼ
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
    // 1����@22.1184MHz
    AUXR |= 0x80;  // ��ʱ��ʱ��1Tģʽ
    TMOD = 0xF3;   // ���ö�ʱ��ģʽ
    TL0 = 0x9A;    // ���ö�ʱ��ʼֵ
    TH0 = 0xA9;    // ���ö�ʱ��ʼֵ
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
    P3M0 = 0x00;
    P3M1 = 0x78;
    P1M0 = 0x02;
    P1M1 = 0x00;
    // I2C�ڲ�����
    P1PU = 0xE0;
    // Key�ڲ�����
    P3PU = 0x78;
    EA = 1;  // �����ж�
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