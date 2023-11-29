/*
 * @Description:
 * @Blog: saisaiwa.com
 * @Author: ccy
 * @Date: 2023-11-02 13:45:35
 * @LastEditTime: 2023-11-02 16:31:48
 */
#include "pt6315.h"

void writeData(uint8_t dat) {
    size_t i = 0;
    CLK_0;
    for (i = 0; i < 8; i++) {
        delay_us(10);
        if (dat & 0x01) {
            DIN_1;
        } else {
            DIN_0;
        }
        delay_us(10);
        CLK_1;
        delay_us(10);
        CLK_0;
        dat >>= 1;
    }
}

void setModeWirteDisplayMode(uint8_t addressMode) {
    uint8_t command = 0x40;
    if (addressMode) {
        command |= 0x4;
    }
    STB_1;
    delay_us(10);
    STB_0;
    delay_us(10);
    writeData(command);
    delay_us(10);
    STB_1;
}

/**
 * COMMANDS 1 ��ʾģʽ��������
 * 0000 �� 4λ�� 24��
 * 0001 ��5λ�� 23��
 * 0010 �� 6λ���֣� 22��
 * 0011 �� 7λ�� 21��
 * 0100 �� 8λ�� 20��
 * 0101 �� 9λ�� 19��
 * 0110 �� 10λ�� 18��
 * 0111 �� 11λ�� 17��
 * 1XXX �� 12λ�� 16��
 */
void setDisplayMode(uint8_t digit) {
    STB_1;
    delay_us(10);
    STB_0;
    delay_us(10);
    writeData(digit);
    delay_us(10);
    STB_1;
}
/**
 * ��ʾ��������  COMMANDS 4
 * @param onOff 0��ʾ�رգ�1������ʾ
 * @param brightnessVal ����ռ�ձ� 0~7����
 * 000��������= 1/16 0
 * 001��������= 2/16 1
 * 010 ��������= 4/16 0x2
 * 011 ��������= 10/16 3
 * 100��������= 11/16  4
 * 101 ��������= 12/16 0x5
 * 110��������= 13/16 6
 * 111��������= 14/16 0x7
 */
void ptSetDisplayLight(uint8_t onOff, uint8_t brightnessVal) {
    uint8_t command = 0x80 | brightnessVal;
    if (onOff) {
        command |= 0x8;
    }
    STB_1;
    delay_us(10);
    STB_0;
    delay_us(10);
    // 0x8f
    writeData(command);
    delay_us(10);
    STB_1;
}

void sendDigAndData(uint8_t dig, const uint8_t* dat, size_t len) {
    size_t i = 0;
    STB_1;
    delay_us(10);
    STB_0;
    delay_us(10);
    writeData(0xc0 | dig);
    delay_us(10);
    for (i = 0; i < len; i++) {
        writeData(dat[i]);
    }
    delay_us(10);
    STB_1;
}
