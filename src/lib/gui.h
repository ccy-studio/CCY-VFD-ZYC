#ifndef __VFD_GUI_
#define __VFD_GUI_

#include "pt6315.h"

// VFD位数
#define VFD_DIG_LEN 9

#define PWM_ARR 1000  // 重载值
#define PWM_CCR 150   // 比较值

/**
 * 初始化
 */
void vfd_gui_init();

/**
 * 停止关闭显示、灯丝将停止驱动
 */
void vfd_gui_stop();

/**
 * 清空VFD屏幕显示,循环刷新如果使用vfd_gui_set_text方法不需要使用它。
 */
void vfd_gui_clear();

/**
 * 显示一串文字，从0位开始。
 * (自动清空覆盖显示，方便每次不用调用clear防止闪屏出现)
 * @param colon 是否显示冒号
 */
void vfd_gui_set_text(const char* string,
                      const u8 colon,
                      const u8 left_first_conlon);

/**
 * 要点亮的ICON图标，宏定义传参
 */
void vfd_gui_set_icon(u32 buf);

/**
 * 设置亮度等级 1~7
 */
void vfd_gui_set_blk_level(size_t level);

/**
 * acg动画
 */
void vfd_gui_acg_update();

/**
 * 屏幕保护程序
 */
void vfd_gui_display_protect_exec();

long map(long x, long in_min, long in_max, long out_min, long out_max);
#endif