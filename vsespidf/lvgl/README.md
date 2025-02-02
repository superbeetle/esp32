# 创建工程

1、打开vscode，F1从模板创建，选hello_world创建

2、example_hello.main修改成lvgl_main.c

3、project(lvgl_demo)

4、新建components



# 集成LVGL

采用lvgl8.3.11，较稳定

1、https://github.com/lvgl/lvgl/tree/v8.3.11

2、https://github.com/lvgl/lvgl_esp32_drivers

3、解压到components，**先vscode编译下**，sdkconfig将出现LVGL configuration、LVGL ESP Drivers

4、main/CMakeLists.txt修改，REQUIRES lvgl_esp32_drivers lvgl

5、sdkconfig配置Enable the examples to be built先去掉勾

6、Select predefined board pinouts、选esp-30pin开发板

7、Select a display controller model，驱动选st7796

​	   Swap the 2 bytes of RGB565 color. Useful if the display has an 8-bit interface (e.g. SPI).勾上

​		TFT SPI Duplex Mode，选HALF DUPLEX（半双工），去掉勾选Use custom SPI clock frequency.

​		或者勾选后选26MHz（FPS实测有点低，实际才7fps），按理vspi接口达到40Mhz-80MHz，但出现花屏或重启，调低频率

​		Set IRAM as LV_ATTRIBUTE_FAST_MEM，勾上提升fps

8、Other/Show CPU usage and FPS count. 勾选显示fps

9、Demos，你可以勾选一些测试程序

10、接线配置

|     TFT     |    ESP32    | ESP32-S2/S3 | ESP32-C2/C3 |           |
| :---------: | :---------: | :---------: | :---------: | :-------: |
|     VCC     |    3.3V     |    3.3V     |     3V3     |           |
|     GND     |     GND     |     GND     |     GND     |           |
|     CS      |   GPIO15    |   GPIO34    |    GPIO2    |           |
|    REST     |   GPIO33    |   GPIO41    |    GPIO4    |   (*1)    |
|     D/C     |   GPIO27    |   GPIO40    |    GPIO3    |   (*1)    |
|    MOSI     | **GPIO13**  |   GPIO35    |    GPIO0    | (*1) (*2) |
|     SCK     | **GPIO14**  |   GPIO36    |    GPIO1    | (*1) (*2) |
| LED（背光） | 3.3V/GPIO32 |    3.3V     |    3.3V     | (*1) (*3) |
|    MISO     |     N/C     |     N/C     |     N/C     |           |
|    T_CLK    | **GPIO14**  |   GPIO36    |    GPIO1    | (*1) (*2) |
|    T_CS     |   GPIO21    |   GPIO38    |    GPIO7    | (*1) (*4) |
|    T_DIN    | **GPIO13**  |   GPIO35    |    GPIO0    | (*1) (*2) |
|    T_OUT    |   GPIO12    |   GPIO37    |    GPIO6    | (*1) (*2) |
|    T_IRQ    |   GPIO22    |   GPIO39    |    GPIO8    |           |

Swap XY.选项去掉打勾	Invert X coordinate value.选项去掉打勾	Invert Y coordinate value.选项打勾

| SDCARD | ESP32  |      |
| ------ | ------ | ---- |
| CS     | GPIO5  |      |
| MOSI   | GPIO23 |      |
| CLK    | GPIO18 |      |
| MISO   | GPIO19 |      |



# 编译修复错误

1、mean 'LV_HOR_RES'?
 #define DISP_BUF_SIZE  (LV_HOR_RES_MAX * 40)

- [ ] lvgl_esp32_drivers/lvgl_helpers.h缺少这两个宏定义，加上

  #define LV_HOR_RES_MAX 320
  #define LV_VER_RES_MAX 480

2、ean 'GPIO_PORT_MAX'?
     assert((0 <= host) && (SPI_HOST_MAX > host));

直接定位注释掉

3、E (432) spi_hal: spi_hal_cal_clock_conf(102): When work in full-duplex mode at frequency > 26.7MHz, device cannot read correct data.
Try to use IOMUX pins to increase the frequency limit, or use the half duplex mode.
Please note the SPI master can only work at divisors of 80MHz, and the driver always tries to find the closest frequency to your configuration.

直接降频或接线重接，查看官方文档仅仅指定GPIO适合40-80MHz

https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/spi_master.html#gpio-matrix-and-iomux

| Pin Name | SPI 2 (GPIO Number) HSPI | SPI 3 (GPIO Number) VSPI |
| :------: | :----------------------: | :----------------------: |
|    CS    |            15            |            5             |
|   SCLK   |            14            |            18            |
|   MISO   |            12            |            19            |
|   MOSI   |            13            |            23            |
|  QUADWP  |            2             |            22            |
|  QUADHD  |            4             |            21            |

4、 st7796s增加颜色翻转配置，需要像ili9481那样增加颜色翻转配置，ili9481(我的屏是st7796，使用st7796s不成功，但ili9481颜色偏淡问题)

​		components/lvgl_esp32_drivers/lvgl_tft/Kconfig

​		640行增加LV_TFT_DISPLAY_CONTROLLER_ST7796s判断

​		config LV_INVERT_COLORS

​        `bool "Invert colors in display" if LV_TFT_DISPLAY_CONTROLLER_ILI9341 || LV_TFT_DISPLAY_CONTROLLER_ST7735S || LV_TFT_DISPLAY_CONTROLLER_ILI9481 || LV_TFT_DISPLAY_CONTROLLER_ST7796S || LV_TFT_DISPLAY_CONTROLLER_ST7789 || LV_TFT_DISPLAY_CONTROLLER_SSD1306 || LV_TFT_DISPLAY_CONTROLLER_SH1107 || LV_TFT_DISPLAY_CONTROLLER_HX8357 || LV_TFT_DISPLAY_CONTROLLER_GC9A01 || LV_TFT_DISPLAY_CONTROLLER_ILI9163C`

​		components/lvgl_esp32_drivers/lvgl_tft/st7796s.c中116行已经存在此判断，所以增加配置，进行配置即可



# 提升FPS

1、配置ESP32-specific，CPU frequency设置240MHz，原fps14提升到fps16

2、配置Serial flasher config，Flash SPI mode原DIO设置QIO，Flash SPI speed原40Mhz设置80Mhz，原fps16提升到fps18

3、配置Compiler options，原-DebugOg改-O2；原fps18提升到fps20

4、配置Bootloader config，原-Os改-O2（此项配置后编译不通过，可选）；配置Partition Table

5、Swap the 2 bytes of RGB565 color. Useful if the display has an 8-bit interface (e.g. SPI).提升到fps31

6、HAL Settings-Default display refresh period (ms).	 配置FPS，10000/x，得出帧数。配10，即100FPS，实际上只测到极限达到fps32

7、使用hspi，spi频率40MHz，fps45，项目使用足够了；spi频率80MHz，fps97，很夸张，存在花屏现象，所以只作测试使用



# 集成SDCard

1、新建组件，F1选择>ESP-IDF Create new ESP-IDF Component，组件名lvgl_sdcard

- [ ] CMakeLists.txt 加入 REQUIRES fatfs
- [ ] 加入Kconfig，可配menuconfig，包括管脚配置、DMA通道、HSPI或VSPI，方便以后组件复用（SDCard SPI onfiguration）
- [ ] 参照sd_card_example例子编写sdcard初始化代码

2、lv_fs_if集成

- [ ] https://github.com/lvgl/lv_fs_if，下载组件放到components

- [ ] 参照此Readme，lv_fs_if.h加入

  /*File system interface*/

  \#define LV_USE_FS_IF 1

  \#if LV_USE_FS_IF

  \#define LV_FS_IF_FATFS 'S'

  \#define LV_FS_IF_PC '\0'

  \#define LV_FS_IF_POSIX '\0'

  \#endif /*LV_USE_FS_IF*/

- [ ] 新建CMakeLists.txt，引用写好的lvgl_sdcard组件

- [ ] lv_fs_fatfs.c，

  ```c
  #include "lvgl_sdcard.h"
  
  /**
  exists code
  ...
  ...
  */
  
  /* Initialize your Storage device and File system. */
  
  static void fs_init(void)
  
  {
  
      /* Initialize the SD card and FatFS itself.
  
       \* Better to do it in your code to keep this library utouched for easy updating*/
  
  
  
      // mysdcard init
  
      sdcard_init();
  
  }
  
  
  /**
  exists code
  ...
  ...
  */
  
  /**
   * Initialize a 'fs_read_dir_t' variable for directory reading
   * @param drv pointer to a driver where this function belongs
   * @param dir_p pointer to a 'fs_read_dir_t' variable
   * @param path path to a directory
   * @return LV_FS_RES_OK or any error from lv_fs_res_t enum
   */
  static void *fs_dir_open(lv_fs_drv_t *drv, const char *path)
  {
      // DIR *d = lv_mem_alloc(sizeof(DIR));
      // 修改FFDIR
      FF_DIR *d = lv_mem_alloc(sizeof(FF_DIR));
      if (d == NULL)
          return NULL;
  
      FRESULT res = f_opendir(d, path);
      if (res != FR_OK)
      {
          lv_mem_free(d);
          d = NULL;
      }
      return d;
  }
  ```

3、main集成测试

- [ ] CMakeLists.txt中引入lv_fs_if组件

- [ ] lvgl 初始化后，初始化sdcard

  ```c
   lv_init();          // lvgl内核初始化
  
   lv_fs_if_init();    // sdcard 初始化
  
   lvgl_driver_init(); // lvgl显示接口初始化
  ```




# 显示图片

下载一张图片，然后https://lvgl.io/tools/imageconverter转成bin，复制到sdcard

- [ ] Color Format选CF_TRUE_COLOR
- [ ] Output Format 选Binary RGB565 SWAP
- [ ] Options 勾选Dither images

# espidf版本问题

官方版https://github.com/lvgl/lvgl_esp32_drivers，使用的是4.x版本太旧了

5.x以上https://github.com/hiruna/lvgl_esp32_drivers/tree/develop/lvgl_8.3.7_idf_5.2
