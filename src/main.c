/*
 * @Description:
 * @Blog: saisaiwa.com
 * @Author: ccy
 * @Date: 2023-11-02 11:19:34
 * @LastEditTime: 2023-11-30 17:58:43
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
u8 last_page_display_flag;                    // 上次页面显示内容
rx8025_timeinfo timeinfo;                     // 主动态更新的时间对象

rx8025_timeinfo set_timeinfo_cache;  // 设置timeinfo时的缓存
u8 max, min;  // 设置时间的时候的临时变量，记录当前设置值的最大和最小阈值范围
bool save_timeinfo_flag = false;  // 触发保存时间的操作flag
u8 data buffer[10];               // vfd显示缓存
bool colon_flag = 0;              // vfd冒号显示状态

u8 set_clock_action_flag = 0;
u8* set_clock_num_p;
u8 set_clock_item = 1;

// 时间等待记录变量
u32 data time_wait_count;
u32 data page_wait_count;
u32 data acg_wait_count;
u32 data saver_wait_count;

/**
 * 按钮定义
 */
static btn_t data btn_arr[3] = {
    {BTN_P33, BTN_RELEASE, 0, 0},
    {BTN_P34, BTN_RELEASE, 0, 0},
    {BTN_P35, BTN_RELEASE, 0, 0},
};

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

    // 开启按键扫描定时器 1毫秒@22.1184MHz
    AUXR |= 0x40;  // 定时器时钟1T模式
    TMOD &= 0x0F;  // 设置定时器模式
    TL1 = 0x9A;    // 设置定时初始值
    TH1 = 0xA9;    // 设置定时初始值
    TF1 = 0;       // 清除TF1标志
    TR1 = 1;       // 定时器1开始计时
    ET1 = 1;       // 使能定时器1中断

    while (1) {
        // 按钮的触发扫描
        // P33- P34+ P35M
        if (btn_arr[0].falg) {
            // P33
            if (btn_arr[0].btn_type == BTN_PRESS) {
                if (page_display_flag != PAGE_FLAG_SET_CLOCK) {
                    page_display_flag =
                        (page_display_flag == PAGE_FLAG_CLOCK_DATE
                             ? PAGE_FLAG_CLOCK_TIME
                             : PAGE_FLAG_CLOCK_DATE);
                } else {
                    // set_clock_action_flag = SC_L;
                }
            } else if (btn_arr[0].btn_type == BTN_LONG) {
            }
            btn_arr[0].falg = 0;
        }
        if (btn_arr[1].falg) {
            // P34
            if (btn_arr[1].btn_type == BTN_PRESS) {
                if (page_display_flag == PAGE_FLAG_SET_CLOCK) {
                    // set_clock_action_flag = SC_P;
                } else {
                    // 亮度调整
                    vfd_brightness = (vfd_brightness + 1) % 3;
                }
            } else if (btn_arr[1].btn_type == BTN_LONG) {
            }
            btn_arr[1].falg = 0;
        }
        if (btn_arr[2].falg) {
            // P35
            if (btn_arr[2].btn_type == BTN_PRESS) {
                if (page_display_flag == PAGE_FLAG_SET_CLOCK) {
                    // set_clock_action_flag = SC_M;
                } else {
                    acg_open = !acg_open;
                }
            } else if (btn_arr[2].btn_type == BTN_LONG) {
                if (page_display_flag == PAGE_FLAG_SET_CLOCK) {
                    page_display_flag = PAGE_FLAG_CLOCK_TIME;
                    save_timeinfo_flag = true;
                } else {
                    last_page_display_flag = page_display_flag;
                    page_display_flag = PAGE_FLAG_SET_CLOCK;
                    set_clock_item = 1;
                    memcpy(&set_timeinfo_cache, &timeinfo, sizeof(timeinfo));
                    // if (last_page_display_flag == PAGE_FLAG_CLOCK_DATE) {

                    // } else if (last_page_display_flag ==
                    // PAGE_FLAG_CLOCK_TIME) {
                    // }
                }
            }
            btn_arr[2].falg = 0;
        }

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
            // 时间设置
            if (interval_check(page_wait_count, 300)) {
                static u8 flicker = 0;  // 闪烁标记
                u8 offset;
                flicker = !flicker;
                memset(buffer, 0x00, sizeof(buffer));
                if (last_page_display_flag == PAGE_FLAG_CLOCK_TIME) {
                    // 设置时间的
                    if (set_clock_item == 1) {
                        set_clock_num_p = &set_timeinfo_cache.hour;
                        max = 23;
                        min = 0;
                        offset = 0;
                    } else if (set_clock_item == 2) {
                        set_clock_num_p = &set_timeinfo_cache.min;
                        max = 59;
                        min = 0;
                        offset = 3;
                    } else if (set_clock_item == 3) {
                        set_clock_num_p = &set_timeinfo_cache.sec;
                        max = 59;
                        min = 0;
                        offset = 6;
                    }
                    if (flicker) {
                        sprintf(buffer + offset, " %02bd", *set_clock_num_p);
                    } else {
                        strcpy(buffer + offset, "   ");
                    }
                } else if (last_page_display_flag == PAGE_FLAG_CLOCK_DATE) {
                    // 设置日期的
                    if (set_clock_item == 1) {
                        set_clock_num_p = &set_timeinfo_cache.year;
                        max = 99;
                        min = 23;
                        if (flicker) {
                            sprintf(buffer, "20%02bd", *set_clock_num_p);
                        } else {
                            strcpy(buffer, "    ");
                        }
                    } else if (set_clock_item == 2) {
                        set_clock_num_p = &set_timeinfo_cache.month;
                        max = 12;
                        min = 1;
                        if (flicker) {
                            sprintf(buffer + 4, "%02bd", *set_clock_num_p);
                        } else {
                            strcpy(buffer + 4, "  ");
                        }
                    } else if (set_clock_item == 3) {
                        set_clock_num_p = &set_timeinfo_cache.day;
                        max = 31;
                        min = 1;
                        if (flicker) {
                            sprintf(buffer + 7, "%02bd", *set_clock_num_p);
                        } else {
                            strcpy(buffer + 7, "  ");
                        }
                    }
                    strcpy(buffer + 6, "-");
                }
                acg_open = false;
                vfd_gui_set_text(
                    buffer,
                    last_page_display_flag == PAGE_FLAG_CLOCK_DATE ? 0 : 1);
                page_wait_count = _systick_ccr;

                // if (set_clock_action_flag) {
                //     u8 max, min;
                //     memset(set_prefix, 0x00, sizeof(set_prefix));
                //     if (set_clock_action_flag == 255) {
                //         memcpy(&set_timeinfo_cache, &timeinfo,
                //                sizeof(timeinfo));
                //     }
                //     if (set_clock_action_flag == SC_M) {
                //         set_clock_item++;
                //         if (set_clock_item > 6) {
                //             set_clock_item = 1;
                //         }
                //     }
                //     switch (set_clock_item) {
                //         case 1:
                //             set_clock_num_p = &set_timeinfo_cache.year;
                //             max = 50;
                //             min = 23;
                //             strcpy(set_prefix, "year-20");
                //             break;
                //         case 2:
                //             set_clock_num_p = &set_timeinfo_cache.month;
                //             max = 12;
                //             min = 1;
                //             strcpy(set_prefix, "month-");
                //             break;
                //         case 3:
                //             set_clock_num_p = &set_timeinfo_cache.day;
                //             max = 31;
                //             min = 1;
                //             strcpy(set_prefix, "day-");
                //             break;
                //         case 4:
                //             set_clock_num_p = &set_timeinfo_cache.hour;
                //             max = 23;
                //             min = 0;
                //             strcpy(set_prefix, "hour-");
                //             break;
                //         case 5:
                //             set_clock_num_p = &set_timeinfo_cache.min;
                //             max = 59;
                //             min = 0;
                //             strcpy(set_prefix, "minute-");
                //             break;
                //         case 6:
                //             set_clock_num_p = &set_timeinfo_cache.sec;
                //             max = 59;
                //             min = 0;
                //             strcpy(set_prefix, "second-");
                //             break;
                //     }
                //     if (set_clock_action_flag == SC_L) {
                //         if (*set_clock_num_p == 0 ||
                //             (*set_clock_num_p - 1) < min) {
                //             *set_clock_num_p = max;
                //         } else {
                //             *set_clock_num_p -= 1;
                //         }
                //     } else if (set_clock_action_flag == SC_P) {
                //         if ((*set_clock_num_p + 1) > max) {
                //             *set_clock_num_p = min;
                //         } else {
                //             *set_clock_num_p += 1;
                //         }
                //     }
                //     memset(buffer, 0x00, sizeof(buffer));
                //     sprintf(buffer, "%s%02bd", set_prefix,
                //     (*set_clock_num_p)); set_clock_action_flag = 0;
                // }
                // acg_open = false;
                // vfd_gui_set_text(buffer, 0);
                // page_wait_count = _systick_ccr;
            }
        }

        // 时间保存设定
        if (save_timeinfo_flag) {
            rx8025_set_time(set_timeinfo_cache.year, set_timeinfo_cache.month,
                            set_timeinfo_cache.day, 1, set_timeinfo_cache.hour,
                            set_timeinfo_cache.min, set_timeinfo_cache.sec);
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

void btn_handler(btn_t* btn) {
    if (btn->last_press_time >= BTN_LONG_PRESS_MS &&
        btn->btn_type == BTN_PRESS) {
        btn->last_press_time += BTN_SCAN_MS;
        return;
    }
    if (btn_gpio_read(btn->gpio)) {
        // 按下触发
        if (btn->btn_type == BTN_PRESS) {
            btn->last_press_time += BTN_SCAN_MS;
        } else if (btn->btn_type == BTN_RELEASE) {
            btn->last_press_time = 0;
            btn->btn_type = BTN_PRESS;
            // send message
            btn->falg = 1;
            return;
        }
        if (btn->btn_type == BTN_PRESS &&
            btn->last_press_time >= BTN_LONG_PRESS_MS) {
            // long press event
            btn->btn_type = BTN_LONG;
            btn->last_press_time += BTN_SCAN_MS;
            // send message
            btn->falg = 1;
            return;
        }
    } else {
        // release
        btn->btn_type = BTN_RELEASE;
        btn->last_press_time = 0;
    }
}

void btn_scan_isr(void) interrupt 3 {
    // P33- P34+ P35M
    btn_handler(&btn_arr[0]);
    btn_handler(&btn_arr[1]);
    btn_handler(&btn_arr[2]);
    // last_key_press_time++;
    // if (last_key_press_time >= 50) {
    //     last_key_press_time = 0;
    //     if (!P33) {
    //         key1_press = 1;

    //     } else if (!P34) {
    //         key2_press = 1;

    //     } else if (!P35) {
    //         key3_press = 1;
    //         key3_press_count++;
    //     }
    // }
    // //---------------按键处理逻辑---------------------//
    // // Key1按键
    // if (P33 && key1_press) {
    //     key1_press = 0;
    //     if (page_display_flag != PAGE_FLAG_SET_CLOCK) {
    //         page_display_flag = (page_display_flag == PAGE_FLAG_CLOCK_DATE
    //                                  ? PAGE_FLAG_CLOCK_TIME
    //                                  : PAGE_FLAG_CLOCK_DATE);
    //     } else {
    //         set_clock_action_flag = SC_L;
    //     }
    // }
    // // Key2按键
    // if (P34 && key2_press) {
    //     key2_press = 0;
    //     if (page_display_flag == PAGE_FLAG_SET_CLOCK) {
    //         set_clock_action_flag = SC_P;
    //     } else {
    //         // 亮度调整
    //         vfd_brightness = (vfd_brightness + 1) % 3;
    //     }
    // }
    // // Key3按键
    // if (P35 && key3_press) {
    //     key3_press = 0;
    //     if (key3_press_count >= 40) {
    //         // 长按
    //         if (page_display_flag == PAGE_FLAG_SET_CLOCK) {
    //             page_display_flag = PAGE_FLAG_CLOCK_TIME;
    //             save_timeinfo_flag = true;
    //         } else {
    //             page_display_flag = PAGE_FLAG_SET_CLOCK;
    //             set_clock_item = 1;
    //             set_clock_action_flag = 255;
    //         }
    //     } else {
    //         // 短按
    //         if (page_display_flag == PAGE_FLAG_SET_CLOCK) {
    //             set_clock_action_flag = SC_M;
    //         } else {
    //             acg_open = !acg_open;
    //         }
    //     }
    //     key3_press_count = 0;
    // }

    TF1 = 0;  // 清除TF1标志
}