#include <stdio.h>
#include "tft_drivers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct
{
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; // No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} TFT_init_cmd_t;

// 初始化时指令ST7796
DRAM_ATTR static const TFT_init_cmd_t tft_init_cmds[] = {
    {0xC0, {0x10, 0x10}, 2},
    {0xC1, {0x41}, 1},
    {0xC5, {0x00, 0x22, 0x80, 0x40}, 4},
    {0x36, {0x48}, 1},
    {0xB0, {0x00}, 1},
    {0xB1, {0xB0, 0x11}, 2},
    {0xB4, {0x02}, 1},
    {0xB6, {0x02, 0x02, 0x3B}, 3},
    {0xB7, {0xC6}, 1},
    {0x3A, {0x55}, 1},
    {0xF7, {0xA9, 0x51, 0x2C, 0x82}, 4},
    {0x11, {0}, 0x80},
    {0x29, {0}, 0x80},
    {0, {0}, 0xff},
};

void tftInit(TFTDev_t *dev)
{
    // Initialize non-SPI GPIOs
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = ((1ULL << dev->devspi.pin_dc) | (1ULL << dev->pinrst) | (1ULL << dev->pinbckl));
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = true;
    gpio_config(&io_conf);

    // Reset the display
    gpio_set_level(dev->pinrst, 0);
    vTaskDelay(100 / portTICK_RATE_MS);
    gpio_set_level(dev->pinrst, 1);
    vTaskDelay(100 / portTICK_RATE_MS);

    // Send all the commands
    int cmd = 0;
    DevSPI_t *devspi = &dev->devspi;
    while (tft_init_cmds[cmd].databytes != 0xff)
    {
        spi_write_cmd(devspi, tft_init_cmds[cmd].cmd);
        spi_write_datas(devspi, tft_init_cmds[cmd].data, tft_init_cmds[cmd].databytes & 0x1F);
        if (tft_init_cmds[cmd].databytes & 0x80)
        {
            vTaskDelay(100 / portTICK_RATE_MS);
        }
        cmd++;
    }

    /// Enable backlight
    gpio_set_level(dev->pinbckl, 1);

    printf("LCD ST7796 initialization.\n");
}

void tftSetDirect(TFTDev_t *dev)
{
}

void tftSetWindow(TFTDev_t *dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    DevSPI_t *devspi = &dev->devspi;

    // 填充坐标
    uint16_t _x1 = x1;
    uint16_t _x2 = x2 >= dev->width ? dev->width - 1 : x2;
    uint16_t _y1 = y1;
    uint16_t _y2 = y2 >= dev->height ? dev->height - 1 : y2;

    // 清屏区域
    spi_write_cmd(devspi, 0x2A); // set column(x) address
    const uint16_t datax[2] = {_x1, _x2};
    spi_write_data_words(devspi, datax, 2);
    spi_write_cmd(devspi, 0x2B); // set Page(y) address
    const uint16_t datay[2] = {_y1, _y2};
    spi_write_data_words(devspi, datay, 2);
    spi_write_cmd(devspi, 0x2C); // Memory Write
}

void tftClear(TFTDev_t *dev, uint16_t color)
{
    DevSPI_t *devspi = &dev->devspi;

    // 填充坐标
    uint16_t x1 = 0;
    uint16_t x2 = dev->width - 1;
    uint16_t y1 = 0;
    uint16_t y2 = dev->height - 1;

    // 清屏区域
    tftSetWindow(dev, x1, y1, x2, y2);

    // 区域着色
    for (int i = x1; i <= x2; i++)
    {
        // 逐行发送
        uint16_t size = y2 - y1 + 1;
        // 方法一：下面字节（8位）发送
        uint8_t Byte[1024];
        int index = 0;
        for (int j = 0; j < size; j++)
        {
            Byte[index++] = (color >> 8) & 0xFF;
            Byte[index++] = color & 0xFF;
        }
        spi_write_datas(devspi, Byte, size * 2);

        // 方法二：按字（16位）发送
        // uint16_t datas[1024] = {0};
        // for (int j = 0; j < size; j++)
        // {
        //     datas[j] = color;
        // }
        // spi_write_data_words(devspi, datas, size);
    }
}