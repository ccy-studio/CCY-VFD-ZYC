#include "gui.h"

u8 lightLevel = 1;  // 亮度级别
static u8 data send_buf[3 * VFD_DIG_LEN] = {0};
static u32 data dig_buf[5] = {0};
const u32 xdata fonts_s1[12] = {
    0x770000,  // ASCII:0,ASCII_N:48 index:0
    0x240000,  // ASCII:1,ASCII_N:49 index:1
    0x6b0000,  // ASCII:2,ASCII_N:50 index:2
    0x6d0000,  // ASCII:3,ASCII_N:51 index:3
    0x3c0000,  // ASCII:4,ASCII_N:52 index:4
    0x5d0000,  // ASCII:5,ASCII_N:53 index:5
    0x5f0000,  // ASCII:6,ASCII_N:54 index:6
    0x640000,  // ASCII:7,ASCII_N:55 index:7
    0x7f0000,  // ASCII:8,ASCII_N:56 index:8
    0x7d0000,  // ASCII:9,ASCII_N:57 index:9
    0x000100,  // ASCII::,ASCII_N:58 index:10
    0x080000,  // ASCII:-
};

const u32 xdata fonts_s2[11] = {
    0x00ee00,  // ASCII:0,ASCII_N:48 index:0
    0x004800,  // ASCII:1,ASCII_N:49 index:1
    0x00d600,  // ASCII:2,ASCII_N:50 index:2
    0x00da00,  // ASCII:3,ASCII_N:51 index:3
    0x007800,  // ASCII:4,ASCII_N:52 index:4
    0x00ba00,  // ASCII:5,ASCII_N:53 index:5
    0x00be00,  // ASCII:6,ASCII_N:54 index:6
    0x00c800,  // ASCII:7,ASCII_N:55 index:7
    0x00fe00,  // ASCII:8,ASCII_N:56 index:8
    0x00fa00,  // ASCII:9,ASCII_N:57 index:9
    0x001000,  // ASCII:-
};

u32 gui_get_font_1(char c);
u32 gui_get_font_2(char c);

void start_pwm() {
    PWMA_CCER1 = 0x00;    // 写CCMRx前必须先清零CCERx关闭通道
    PWMA_CCMR2 = 0x68;    // 设置CC2为PWMA输出模式
    PWMA_CCER1 = 0x10;    // 使能CC2通道
    PWMA_CCR2 = PWM_CCR;  // 设置占空比时间
    PWMA_ARR = PWM_ARR;   // 设置周期时间
    PWMA_ENO = 0x04;      // 使能PWM2P端口输出
    PWMA_BKR = 0x80;      // 使能主输出
    PWMA_CR1 |= 0x81;     // 开始计时
}

void stop_pwm() {
    PWMA_CR1 &= 0xFE;  // 关闭定时器
    PWMA_ENO = 0x00;   // 禁止使能
}

void vfd_gui_init() {
    VFD_EN = 1;
    start_pwm();
    // VFD Setting
    setDisplayMode(2);
    setModeWirteDisplayMode(0);
    vfd_gui_set_blk_level(7);
    vfd_gui_clear();
}

void vfd_gui_stop() {
    VFD_EN = 0;
    stop_pwm();
    vfd_gui_clear();
}

void vfd_gui_clear() {
    memset(send_buf, 0x00, sizeof(send_buf));
    sendDigAndData(0, send_buf, sizeof(send_buf));
}

void vfd_gui_set_dig(u8 dig, u32 buf) {
    uint8_t arr[3] = {0};
    memset(arr, 0x00, sizeof(arr));
    if (buf) {
        arr[0] = (buf >> 16) & 0xFF;
        arr[1] = (buf >> 8) & 0xFF;
        arr[2] = buf & 0xFF;
    }
    sendDigAndData(dig * 3, arr, 3);
}

void vfd_gui_set_text(char* string, const u8 colon) {
    /**
     * 这个屏的位和段真是tm的变态，代码还得tm单独处理你艹。垃圾国产屏~
     */
    size_t str_len = strlen(string);
    size_t index = 0, i = 0, k = 0;
    size_t len = str_len > 9 ? 9 : str_len;
    memset(send_buf, 0x00, sizeof(send_buf));
    memset(dig_buf, 0, sizeof(dig_buf));

    if (len != 0) {
        for (i = 0; i < 5; i++) {
            if (string[k] != '\0') {
                if (k != 8 && (k + 1) % 2 != 0) {
                    dig_buf[i] = gui_get_font_1(string[k]);
                } else {
                    dig_buf[i] = gui_get_font_2(string[k]);
                }
            }
            ++k;
            if (k >= len) {
                break;
            }
            if (string[k] != '\0') {
                if (k != 8 && (k + 1) % 2 != 0) {
                    dig_buf[i] = gui_get_font_1(string[k]) | dig_buf[i];
                } else {
                    dig_buf[i] = gui_get_font_2(string[k]) | dig_buf[i];
                }
            }
            ++k;
            if (k >= len) {
                break;
            }
        }
    }
    if (colon) {
        dig_buf[3] |= 0x080000;
        dig_buf[1] |= 0x001000;
    }
    // 设置点亮[dts]图标
    dig_buf[4] |= 0x020000;
    for (i = 0; i < 5; i++) {
        send_buf[index++] = (dig_buf[i] >> 16) & 0xFF;
        send_buf[index++] = (dig_buf[i] >> 8) & 0xFF;
        send_buf[index++] = dig_buf[i] & 0xFF;
    }

    sendDigAndData(0, send_buf, sizeof(send_buf));
}

/**
 * 亮度调节 1~7
 */
void vfd_gui_set_blk_level(size_t level) {
    if (level == lightLevel) {
        return;
    }
    lightLevel = level;
    ptSetDisplayLight(1, lightLevel);
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
    const long dividend = out_max - out_min;
    const long divisor = in_max - in_min;
    const long delta = x - in_min;
    return (delta * dividend + (divisor / 2)) / divisor + out_min;
}

u32 gui_get_font_1(char c) {
    if (c == ' ') {
        return 0;
    }
    if (c == '-') {
        return fonts_s1[11];
    }
    if (c >= 48 && c <= 58) {
        return fonts_s1[map(c, 48, 58, 0, 10)];
    } else {
        return 0;
    }
}

u32 gui_get_font_2(char c) {
    if (c == ' ') {
        return 0;
    }
    if (c == '-') {
        return fonts_s2[10];
    }
    if (c >= 48 && c <= 58) {
        return fonts_s2[map(c, 48, 57, 0, 9)];
    } else {
        return 0;
    }
}

u8 bit_reversed(u8 old) {
    // 颠倒所有比特位
    uint8_t reversedValue = 0, i;
    for (i = 0; i < 8; ++i) {
        reversedValue |= ((old >> i) & 1) << (7 - i);
    }
    return reversedValue;
}

/**
 * acg动画
 */
void vfd_gui_acg_update() {
    static u8 pos = 0;
    static u32 acg1_u32s[12] = {0};
    if (pos >= 12) {
        pos = 0;
    }
    if (!acg1_u32s[11]) {
        u32 b;
        u8 u1, u2, u3;
        if (pos >= 12) {
            pos = 0;
        }
        b = 0x800000 >> pos;
        u1 = bit_reversed((b >> 16) & 0xFF);
        u2 = bit_reversed((b >> 8) & 0xFF);
        u3 = bit_reversed(b & 0xFF);
        b = (uint32_t)u1 << 16 | u2 << 8 | u3;
        // 设置方框的动画时间特效
        if (pos < 4) {
            b |= 0x004000;
        } else if (pos >= 4 && pos < 8) {
            b |= 0x002000;
        } else if (pos >= 8) {
            b |= 0x001000;
        }
        acg1_u32s[pos] = b;
    }
    vfd_gui_set_dig(5, acg1_u32s[pos]);
    pos++;
}

/**
 * 屏幕保护程序
 */
void vfd_gui_display_protect_exec() {
    u8 i, j;
    u8 buf[12];
    for (i = 1; i <= 10; i++) {
        memset(buf, 0x00, sizeof(buf));
        for (j = 0; j < 10; j++) {
            u8 rn = (hal_systick_get() << i) % 10;
            sprintf((&buf) + j, "%bd", rn);
            delay_ms(1);
        }
        vfd_gui_set_text(buf, 0);
        delay_ms(10);
    }
    vfd_gui_clear();
}
