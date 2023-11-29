/*
 * @Description:
 * @Blog: saisaiwa.com
 * @Author: ccy
 * @Date: 2023-11-02 11:19:34
 * @LastEditTime: 2023-11-29 11:08:33
 */
#include "gui.h"
#include "rx8025.h"

#define SC_P 0x01
#define SC_L 0x02
#define SC_M 0x03

/**
 * 0x0n 一级页面
 * 0x1x 二级页面(设置子页面)
 */
#define PAGE_FLAG_CLOCK_TIME 0x01  // 时间显示页面
#define PAGE_FLAG_CLOCK_DATE 0x02  // 日期显示页面
#define PAGE_FLAG_SET_CLOCK 0x10   // 设置时间子页面

/**
 * 屏幕保护程序间隔执行时间,单位毫秒默认2分钟一次
 */
// #define SCREEN_SAVER_TIME 120000UL
#define SCREEN_SAVER_TIME 5000UL

u8 vfd_brightness_level[3] = {1, 2, 7};  // VFD亮度等级
u8 vfd_brightness = 2;                   // VFD亮度等级Level下标
bool acg_open = true;                    // vfd动画特效开关
bool vfd_saver_open = true;              // vfd屏幕保护程序开关

u8 page_display_flag = PAGE_FLAG_CLOCK_TIME;  // 页面显示内容
rx8025_timeinfo timeinfo;
rx8025_timeinfo set_timeinfo_cache;          // 设置timeinfo时的缓存
bool save_timeinfo_flag = false;             // 触发保存时间的操作flag
u8 data buffer[10];                          // vfd显示缓存
bool colon_flag = 0, left_first_colon_flag;  // vfd冒号显示状态

u8 set_clock_action_flag = 0;
u8* set_clock_num_p;
u8 set_clock_item = 1, set_prefix[8];

u32 data time_wait_count;
u32 data page_wait_count;
u32 data acg_wait_count;
u32 data saver_wait_count;
u32 data last_key_press_time;
/**
 * 按钮状态标记位[按下?1，松开?2]
 */
u8 data key1_press = 0;
u8 data key2_press = 0;
u8 data key3_press = 0;
u8 data key3_press_count = 0;

extern u32 data _systick_ccr;

bool interval_check(u32 select, u32 t);
void page_home();
void menu_display_refresh();

void main() {
    P_SW2 |= 0x80;  // 使能EAXFR寄存器 XFR
    hal_init_all_gpio();
    hal_init_systick();
    // hal_init_uart();
    rx8025t_init();
    vfd_gui_init();

    // 开启按键扫描定时器
    AUXR |= 0x40;  // 定时器时钟1T模式
    TMOD &= 0x0F;  // 设置定时器模式
    TL1 = 0x9A;    // 设置定时初始值
    TH1 = 0xA9;    // 设置定时初始值
    TF1 = 0;       // 清除TF1标志
    TR1 = 1;       // 定时器1开始计时
    ET1 = 1;       // 使能定时器1中断

    // while (1) {
    //     delay_ms(1000);
    //     vfd_gui_set_text("123456789", 0);
    // }

    while (1) {
        // 动态亮度调整
        vfd_gui_set_blk_level(vfd_brightness_level[vfd_brightness]);
        // 主页面内容筛选
        if (page_display_flag == PAGE_FLAG_CLOCK_TIME ||
            page_display_flag == PAGE_FLAG_CLOCK_DATE) {
            if (interval_check(time_wait_count, 500)) {
                page_home();
                time_wait_count = _systick_ccr;
            }
        } else {
            // 菜单子页面内容刷新
            if (interval_check(page_wait_count, 300)) {
                if (set_clock_action_flag) {
                    u8 max, min;
                    memset(set_prefix, 0x00, sizeof(set_prefix));
                    if (set_clock_action_flag == 255) {
                        memcpy(&set_timeinfo_cache, &timeinfo,
                               sizeof(timeinfo));
                    }
                    if (set_clock_action_flag == SC_M) {
                        set_clock_item++;
                        if (set_clock_item > 6) {
                            set_clock_item = 1;
                        }
                    }
                    switch (set_clock_item) {
                        case 1:
                            set_clock_num_p = &set_timeinfo_cache.year;
                            max = 50;
                            min = 23;
                            strcpy(set_prefix, "year-20");
                            break;
                        case 2:
                            set_clock_num_p = &set_timeinfo_cache.month;
                            max = 12;
                            min = 1;
                            strcpy(set_prefix, "month-");
                            break;
                        case 3:
                            set_clock_num_p = &set_timeinfo_cache.day;
                            max = 31;
                            min = 1;
                            strcpy(set_prefix, "day-");
                            break;
                        case 4:
                            set_clock_num_p = &set_timeinfo_cache.hour;
                            max = 23;
                            min = 0;
                            strcpy(set_prefix, "hour-");
                            break;
                        case 5:
                            set_clock_num_p = &set_timeinfo_cache.min;
                            max = 59;
                            min = 0;
                            strcpy(set_prefix, "minute-");
                            break;
                        case 6:
                            set_clock_num_p = &set_timeinfo_cache.sec;
                            max = 59;
                            min = 0;
                            strcpy(set_prefix, "second-");
                            break;
                    }
                    if (set_clock_action_flag == SC_L) {
                        if (*set_clock_num_p == 0 ||
                            (*set_clock_num_p - 1) < min) {
                            *set_clock_num_p = max;
                        } else {
                            *set_clock_num_p -= 1;
                        }
                    } else if (set_clock_action_flag == SC_P) {
                        if ((*set_clock_num_p + 1) > max) {
                            *set_clock_num_p = min;
                        } else {
                            *set_clock_num_p += 1;
                        }
                    }
                    memset(buffer, 0x00, sizeof(buffer));
                    sprintf(buffer, "%s%02bd", set_prefix,
                    (*set_clock_num_p)); set_clock_action_flag = 0;
                }
                acg_open = false;
                vfd_gui_set_text(buffer, 0);
                page_wait_count = _systick_ccr;
            }
        }

        // 时间保存设定
        if (save_timeinfo_flag) {
            rx8025_set_time(set_timeinfo_cache.year,
            set_timeinfo_cache.month,
                            set_timeinfo_cache.day, 1,
                            set_timeinfo_cache.hour, set_timeinfo_cache.min,
                            set_timeinfo_cache.sec);
            save_timeinfo_flag = false;
        }

        // vfd特效动画
        if (acg_open) {
            if (interval_check(acg_wait_count, 100)) {
                vfd_gui_acg_update();
                acg_wait_count = _systick_ccr;
            }
        }

        // 屏幕保护程序
        if (vfd_saver_open) {
            if (interval_check(saver_wait_count, SCREEN_SAVER_TIME)) {
                vfd_gui_display_protect_exec();
                saver_wait_count = _systick_ccr;
            }
        }
    }
}

bool interval_check(u32 select, u32 t) {
    return select > _systick_ccr || (_systick_ccr - select) >= t;
}

void page_home() {
    rx8025_time_get(&timeinfo);
    memset(buffer, 0, sizeof(buffer));
    if (page_display_flag == PAGE_FLAG_CLOCK_TIME) {
        colon_flag = !colon_flag;
        formart_time(&timeinfo, &buffer);
    } else if (page_display_flag == PAGE_FLAG_CLOCK_DATE) {
        colon_flag = 0;
        formart_date(&timeinfo, &buffer);
    }
    vfd_gui_set_text(buffer, colon_flag);
}

void btn_scan_isr(void) interrupt 3 {
    // P33- P34+ P35M
    last_key_press_time++;
    if (last_key_press_time >= 50) {
        last_key_press_time = 0;
        if (!P33) {
            key1_press = 1;

        } else if (!P34) {
            key2_press = 1;

        } else if (!P35) {
            key3_press = 1;
            key3_press_count++;
        }
    }
    //---------------按键处理逻辑---------------------//
    // Key1按键
    if (P33 && key1_press) {
        key1_press = 0;
        if (page_display_flag != PAGE_FLAG_SET_CLOCK) {
            page_display_flag = (page_display_flag == PAGE_FLAG_CLOCK_DATE
                                     ? PAGE_FLAG_CLOCK_TIME
                                     : PAGE_FLAG_CLOCK_DATE);
        } else {
            set_clock_action_flag = SC_L;
        }
    }
    // Key2按键
    if (P34 && key2_press) {
        key2_press = 0;
        if (page_display_flag == PAGE_FLAG_SET_CLOCK) {
            set_clock_action_flag = SC_P;
        } else {
            // 亮度调整
            vfd_brightness = (vfd_brightness + 1) % 3;
        }
    }
    // Key3按键
    if (P35 && key3_press) {
        key3_press = 0;
        if (key3_press_count >= 40) {
            // 长按
            if (page_display_flag == PAGE_FLAG_SET_CLOCK) {
                page_display_flag = PAGE_FLAG_CLOCK_TIME;
                save_timeinfo_flag = true;
            } else {
                page_display_flag = PAGE_FLAG_SET_CLOCK;
                set_clock_item = 1;
                set_clock_action_flag = 255;
            }
        } else {
            // 短按
            if (page_display_flag == PAGE_FLAG_SET_CLOCK) {
                set_clock_action_flag = SC_M;
            } else {
                acg_open = !acg_open;
            }
        }
        key3_press_count = 0;
    }

    TF1 = 0;  // 清除TF1标志
}