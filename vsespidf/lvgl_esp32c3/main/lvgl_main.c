#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_freertos_hooks.h"

#include "lvgl.h"
#include "lvgl_helpers.h"

#include "lv_demos.h"
#include "ui.h"
#include "my_clock.h"

/*-----------------函数声明-----------------------------------*/
void lvgl_test(void);
void lvgl_bg_color_test(void);
/*-----------------------------------------------------------*/

/* LVGL 移植部分 */
static void lv_tick_task(void *arg)
{
    (void)arg;
    lv_tick_inc(portTICK_PERIOD_MS);
}
SemaphoreHandle_t xGuiSemaphore;

static void gui_demo()
{
    // lvgl_bg_color_test();
    // lvgl_test();
    // lv_demo_widgets();
    // lv_demo_keypad_encoder();
    // lv_demo_music();
    // lv_demo_printer();
    // 以下2案例性能测试
    // lv_demo_benchmark();
    // lv_demo_stress();
    // ui_init();
    my_clock_init();
}

/* UI 任务 */
static void gui_task(void *arg)
{
    xGuiSemaphore = xSemaphoreCreateMutex();
    lv_init();          // lvgl内核初始化
    lvgl_driver_init(); // lvgl显示接口初始化

    static lv_disp_draw_buf_t draw_buf;
    uint16_t buf_size = (LV_HOR_RES_MAX * LV_VER_RES_MAX) / 10;
    lv_color_t *buf1 = heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_color_t *buf2 = heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buf_size); /*Initialize the display buffer*/

    static lv_disp_drv_t disp_drv;         /*A variable to hold the drivers. Must be static or global.*/
    lv_disp_drv_init(&disp_drv);           /*Basic initialization*/
    disp_drv.draw_buf = &draw_buf;         /*Set an initialized buffer*/
    disp_drv.flush_cb = disp_driver_flush; /*Set a flush callback to draw to the display*/
    disp_drv.hor_res = LV_HOR_RES_MAX;     /*Set the horizontal resolution in pixels*/
    disp_drv.ver_res = LV_VER_RES_MAX;     /*Set the vertical resolution in pixels*/

    /* When using a monochrome display we need to register the callbacks:
     * - rounder_cb
     * - set_px_cb */
#ifdef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    disp_drv.rounder_cb = disp_driver_rounder;
    disp_drv.set_px_cb = disp_driver_set_px;
#endif
    lv_disp_drv_register(&disp_drv); /*Register the driver and save the created display objects*/

    /* Register an input device when enabled on the menuconfig */
#if CONFIG_LV_TOUCH_CONTROLLER != TOUCH_CONTROLLER_NONE
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.read_cb = touch_driver_read;
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    lv_indev_drv_register(&indev_drv);
#endif

    esp_register_freertos_tick_hook((void *)lv_tick_task);

    gui_demo();

    while (1)
    {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));

        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
        {
            lv_timer_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
    }

    /* A task should NEVER return */
    free(buf1);
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    free(buf2);
#endif
    vTaskDelete(NULL);
}

void lvgl_bg_color_test(void)
{
    // base bg color
    static lv_style_t style;
    lv_style_init(&style);
    // lv_style_set_bg_color(&style, lv_palette_lighten(LV_PALETTE_RED, 1));// 颜色纯度
    lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_RED)); // 单色

    lv_obj_t *obj = lv_obj_create(lv_scr_act()); // 创建对象
    lv_obj_add_style(obj, &style, 0);            // 设置对象的样式
    lv_obj_set_size(obj, LV_HOR_RES_MAX / 3, LV_VER_RES_MAX);
    lv_obj_set_pos(obj, 0, 0);

    lv_obj_t *obj1 = lv_obj_create(lv_scr_act());                          // 创建对象
    lv_obj_add_style(obj1, &style, 0);                                     // 设置对象的样式
    lv_obj_set_style_bg_color(obj1, lv_palette_main(LV_PALETTE_GREEN), 0); // 单色
    lv_obj_set_size(obj1, LV_HOR_RES_MAX / 3, LV_VER_RES_MAX);
    lv_obj_set_pos(obj1, LV_HOR_RES_MAX / 3, 0);

    lv_obj_t *obj2 = lv_obj_create(lv_scr_act());                         // 创建对象
    lv_obj_add_style(obj2, &style, 0);                                    // 设置对象的样式
    lv_obj_set_style_bg_color(obj2, lv_palette_main(LV_PALETTE_BLUE), 0); // 单色
    lv_obj_set_size(obj2, LV_HOR_RES_MAX / 3, LV_VER_RES_MAX);
    lv_obj_set_pos(obj2, (LV_HOR_RES_MAX / 3) * 2, 0);
}

// lvgl测试
void lvgl_test(void)
{
    lv_obj_t *label1 = lv_label_create(lv_scr_act());
    lv_label_set_long_mode(label1, LV_LABEL_LONG_WRAP); /*Break the long lines*/
    lv_label_set_recolor(label1, true);                 /*Enable re-coloring by commands in the text*/
    lv_label_set_text(label1, "#0000ff Re-color# #ff00ff words# #ff0000 of a# label, align the lines to the center"
                              "and  wrap long text automatically.");
    lv_obj_set_width(label1, 120); /*Set smaller width to make the lines wrap*/
    lv_obj_set_style_text_align(label1, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label1, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t *label2 = lv_label_create(lv_scr_act());
    lv_label_set_long_mode(label2, LV_LABEL_LONG_SCROLL_CIRCULAR); /*Circular scroll*/
    lv_obj_set_width(label2, 120);
    lv_label_set_text(label2, "It is a circularly scrolling text. ");
    lv_obj_align(label2, LV_ALIGN_CENTER, 0, 40);
}

// 主函数
void app_main(void)
{
    xTaskCreatePinnedToCore(gui_task, "gui task", 1024 * 4, NULL, 1, NULL, 0);
}
