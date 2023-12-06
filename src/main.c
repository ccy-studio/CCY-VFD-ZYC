/*
 * @Description:
 * @Blog: saisaiwa.com
 * @Author: ccy
 * @Date: 2023-11-02 11:19:34
 * @LastEditTime: 2023-12-06 17:32:55
 */
#include "gui.h"
#include "rx8025.h"
#include "ws2812b.h"

/**
 * 0x0n һ��ҳ��
 * 0x1x ����ҳ��(������ҳ��)
 */
#define PAGE_FLAG_CLOCK_TIME 0x01  // ʱ����ʾҳ��
#define PAGE_FLAG_CLOCK_DATE 0x02  // ������ʾҳ��
#define PAGE_FLAG_SET_CLOCK 0x10   // ����ʱ����ҳ��

/**
 * ��Ļ����������ִ��ʱ��,��λ����Ĭ��2����һ��
 */
#define SCREEN_SAVER_TIME 120000UL
// #define SCREEN_SAVER_TIME 10000UL

u8 vfd_brightness_level[3] = {1, 2, 7};  // VFD���ȵȼ�
u8 vfd_brightness = 2;                   // VFD���ȵȼ�Level�±�
bool acg_open = true;                    // vfd������Ч����
bool vfd_saver_open = true;              // vfd��Ļ�������򿪹�

u8 data page_display_flag = PAGE_FLAG_CLOCK_TIME;  // ҳ����ʾ����
u8 last_page_display_flag;                         // �ϴ�ҳ����ʾ����
rx8025_timeinfo timeinfo;  // ����̬���µ�ʱ�����

rx8025_timeinfo set_timeinfo_cache;  // ����timeinfoʱ�Ļ���
u8 max, min;  // ����ʱ���ʱ�����ʱ��������¼��ǰ����ֵ��������С��ֵ��Χ
bool save_timeinfo_flag = false;  // ��������ʱ��Ĳ���flag
u8 data buffer[10];               // vfd��ʾ����
bool colon_flag = 0;              // vfdð����ʾ״̬

// RGB����
u8 rgb_brightness = 255;  // RGB����
u8 rgb_open = 1;          // RGB����
u8 rgb_type = 0;          // RGB��˸����

u8* set_clock_num_p;    // ��¼ʱ�����õ�ָ�����
u8 set_clock_item = 1;  // ��¼ʱ�����õ�ʱ����

// ʱ��ȴ���¼����
u32 data time_wait_count;
u32 data page_wait_count;
u32 data acg_wait_count;
u32 data saver_wait_count;
u32 data rgb_wait_count;

/**
 * ��ť����
 */
static btn_t data btn_arr[3] = {
    {BTN_P, BTN_RELEASE, 0, 0},
    {BTN_S, BTN_RELEASE, 0, 0},
    {BTN_M, BTN_RELEASE, 0, 0},
};

extern u32 data _systick_ccr;  // ϵͳ�δ�ʱ����ʱ

bool interval_check(u32 select, u32 t);  // ���ʱ���Ƿ���������
void page_home();

void main() {
    P_SW2 |= 0x80;  // ʹ��EAXFR�Ĵ��� XFR
    hal_init_all_gpio();
    hal_init_systick();
    // hal_init_uart();
    rx8025t_init();
    vfd_gui_init();
    rgb_init();

    // ��������ɨ�趨ʱ�� 1����@22.1184MHz
    AUXR |= 0x40;  // ��ʱ��ʱ��1Tģʽ
    TMOD &= 0x0F;  // ���ö�ʱ��ģʽ
    TL1 = 0x40;    // ���ö�ʱ��ʼֵ
    TH1 = 0xA2;    // ���ö�ʱ��ʼֵ
    TF1 = 0;       // ���TF1��־
    TR1 = 1;       // ��ʱ��1��ʼ��ʱ
    ET1 = 1;       // ʹ�ܶ�ʱ��1�ж�

    while (1) {
        //====>>>>>>��ť�Ĵ���ɨ�� -> Start
        if (btn_arr[0].falg) {
            // P33
            if (btn_arr[0].btn_type == BTN_PRESS) {
                if (page_display_flag != PAGE_FLAG_SET_CLOCK) {
                    page_display_flag =
                        (page_display_flag == PAGE_FLAG_CLOCK_DATE
                             ? PAGE_FLAG_CLOCK_TIME
                             : PAGE_FLAG_CLOCK_DATE);
                } else {
                    if (*set_clock_num_p == 0 || (*set_clock_num_p - 1) < min) {
                        *set_clock_num_p = max;
                    } else {
                        *set_clock_num_p -= 1;
                    }
                }
            } else if (btn_arr[0].btn_type == BTN_LONG) {
            }
            btn_arr[0].falg = 0;
        }
        if (btn_arr[1].falg) {
            // P34
            if (btn_arr[1].btn_type == BTN_PRESS) {
                if (page_display_flag == PAGE_FLAG_SET_CLOCK) {
                    if ((*set_clock_num_p + 1) > max) {
                        *set_clock_num_p = min;
                    } else {
                        *set_clock_num_p += 1;
                    }
                } else {
                    // ���ȵ���
                    vfd_brightness = (vfd_brightness + 1) % 3;
                }
            } else if (btn_arr[1].btn_type == BTN_LONG) {
                // ��������RGB�Ŀ�������Ч���͸ı�
                rgb_open = !rgb_open;
                rgb_type = (rgb_type + 1) % 3;
                if (!rgb_open) {
                    rgb_clear();
                }
            }
            btn_arr[1].falg = 0;
        }
        if (btn_arr[2].falg) {
            // P35
            if (btn_arr[2].btn_type == BTN_PRESS) {
                if (page_display_flag == PAGE_FLAG_SET_CLOCK) {
                    set_clock_item++;
                    if (set_clock_item > 3) {
                        set_clock_item = 1;
                    }
                } else {
                    acg_open = !acg_open;
                }
            } else if (btn_arr[2].btn_type == BTN_LONG) {
                if (page_display_flag == PAGE_FLAG_SET_CLOCK) {
                    page_display_flag = PAGE_FLAG_CLOCK_TIME;
                    save_timeinfo_flag = true;
                    memset(buffer, 0, sizeof(buffer));
                } else {
                    last_page_display_flag = page_display_flag;
                    page_display_flag = PAGE_FLAG_SET_CLOCK;
                    set_clock_item = 1;
                    memcpy(&set_timeinfo_cache, &timeinfo, sizeof(timeinfo));
                }
            }
            btn_arr[2].falg = 0;
        }
        //====>>>>>>��ť�Ĵ���ɨ�� -> End

        // ��̬���ȵ���
        vfd_gui_set_blk_level(vfd_brightness_level[vfd_brightness]);
        // ��ҳ������ɸѡ
        if (page_display_flag == PAGE_FLAG_CLOCK_TIME ||
            page_display_flag == PAGE_FLAG_CLOCK_DATE) {
            if (interval_check(time_wait_count, 500)) {
                page_home();
                time_wait_count = _systick_ccr;
            }
        } else {
            // ʱ������
            if (interval_check(page_wait_count, 300)) {
                static u8 flicker = 0;  // ��˸���
                flicker = !flicker;
                memset(buffer, 0x00, sizeof(buffer));
                if (last_page_display_flag == PAGE_FLAG_CLOCK_TIME) {
                    sprintf(buffer, " %02bd %02bd %02bd",
                            set_timeinfo_cache.hour, set_timeinfo_cache.min,
                            set_timeinfo_cache.sec);
                    // ����ʱ���
                    if (set_clock_item == 1) {
                        set_clock_num_p = &set_timeinfo_cache.hour;
                        max = 23;
                        min = 0;
                        if (flicker) {
                            memcpy(buffer + 1, "  ", 2);
                        }
                    } else if (set_clock_item == 2) {
                        set_clock_num_p = &set_timeinfo_cache.min;
                        max = 59;
                        min = 0;
                        if (flicker) {
                            memcpy(buffer + 4, "  ", 2);
                        }
                    } else if (set_clock_item == 3) {
                        set_clock_num_p = &set_timeinfo_cache.sec;
                        max = 59;
                        min = 0;
                        if (flicker) {
                            memcpy(buffer + 7, "  ", 2);
                        }
                    }
                } else if (last_page_display_flag == PAGE_FLAG_CLOCK_DATE) {
                    // �������ڵ�
                    sprintf(buffer, "20%bd%02bd-%02bd", set_timeinfo_cache.year,
                            set_timeinfo_cache.month, set_timeinfo_cache.day);
                    if (set_clock_item == 1) {
                        set_clock_num_p = &set_timeinfo_cache.year;
                        max = 99;
                        min = 23;
                        if (flicker) {
                            memcpy(buffer + 2, "  ", 2);
                        }
                    } else if (set_clock_item == 2) {
                        set_clock_num_p = &set_timeinfo_cache.month;
                        max = 12;
                        min = 1;
                        if (flicker) {
                            memcpy(buffer + 4, "  ", 2);
                        }
                    } else if (set_clock_item == 3) {
                        set_clock_num_p = &set_timeinfo_cache.day;
                        max = 31;
                        min = 1;
                        if (flicker) {
                            memcpy(buffer + 7, "  ", 2);
                        }
                    }
                }
                acg_open = false;
                vfd_gui_set_text(
                    buffer,
                    last_page_display_flag == PAGE_FLAG_CLOCK_DATE ? 0 : 1);
                page_wait_count = _systick_ccr;
            }
        }

        // ʱ�䱣���趨
        if (save_timeinfo_flag) {
            if (last_page_display_flag == PAGE_FLAG_CLOCK_DATE) {
                // �������ڱ���ʱ��
                set_timeinfo_cache.hour = timeinfo.hour;
                set_timeinfo_cache.min = timeinfo.min;
                set_timeinfo_cache.sec = timeinfo.sec;
            } else {
                // ����ʱ�䱣������
                set_timeinfo_cache.year = timeinfo.year;
                set_timeinfo_cache.month = timeinfo.month;
                set_timeinfo_cache.day = timeinfo.day;
                set_timeinfo_cache.week = timeinfo.week;
            }
            rx8025_set_time(set_timeinfo_cache.year, set_timeinfo_cache.month,
                            set_timeinfo_cache.day, set_timeinfo_cache.week - 1,
                            set_timeinfo_cache.hour, set_timeinfo_cache.min,
                            set_timeinfo_cache.sec);
            memset(&set_timeinfo_cache, 0x00, sizeof(set_timeinfo_cache));
            save_timeinfo_flag = false;
        }

        // vfd��Ч����
        if (acg_open && page_display_flag != PAGE_FLAG_SET_CLOCK) {
            if (interval_check(acg_wait_count, 100)) {
                vfd_gui_acg_update();
                acg_wait_count = _systick_ccr;
            }
        }

        // ��Ļ��������
        if (vfd_saver_open && page_display_flag != PAGE_FLAG_SET_CLOCK) {
            if (interval_check(saver_wait_count, SCREEN_SAVER_TIME)) {
                vfd_gui_display_protect_exec();
                saver_wait_count = _systick_ccr;
            }
        }

        // RGB
        if (rgb_open) {
            if (interval_check(rgb_wait_count, 10)) {
                rgb_frame_update(rgb_brightness, rgb_type);
                rgb_wait_count = _systick_ccr;
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
    if (!btn_gpio_read(btn->gpio)) {
        // ���´���
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
    btn_handler(&btn_arr[0]);
    btn_handler(&btn_arr[1]);
    btn_handler(&btn_arr[2]);
    TF1 = 0;  // ���TF1��־
}