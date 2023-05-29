/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "driver/gpio.h"
#include "esp_sleep.h"
#include <time.h>
#include <sys/time.h>
#include "esp_timer.h"

#include "esp_task_wdt.h"

#include "ds_test.h"
#include "ds_wifi_scan.h"
#include "ds_ft6336.h"
#include "ds_screen.h"
#include "ds_spi.h"
#include "ds_wifi_ap.h"
#include "ds_wifi_sta.h"
#include "ds_system_data.h"
#include "ds_nvs.h"
#include "ds_https_request.h"
#include "ds_http_request.h"
#include "ds_timer.h"
#include "ds_spiffs.h"
#include "ds_ui_timepage.h"
#include "ds_ui_page_manage.h"
#include "ds_ui_weatherpage.h"
#include "ds_ui_wordpage.h"
#include "ds_ui_tomatopage.h"
#include "ds_ui_fans.h"
#include "ds_font.h"
#include "ds_paint.h"
#include "ds_wifi_ap_sta.h"
#include "ds_pwm.h"
#include "ds_gpio.h"
#include "ds_http_server.h"

#ifdef CONFIG_IDF_TARGET_ESP32
#define CHIP_NAME "ESP32"
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S2BETA
#define CHIP_NAME "ESP32-S2 Beta"
#endif

#define BUTTON_GPIO_NUM_DEFAULT  4
#define TWDT_TIMEOUT_S          3
#define SLEEP_TIME_RESET 600

static void sleep_mode_init(){
    /*
    set_tp_wackup_timeleft(600);设置屏幕唤醒时进入低功耗模式的倒计时
    在ds_system_data.c中g_system_data被初始化为SYSTRM_DATA_T结构体的全局变量
    tp_wackup_timeleft是SYSTRM_DATA_T结构体中一个无符号32位变量，用于配置指定的倒计时时间单位为秒
    */
    set_tp_wackup_timeleft(SLEEP_TIME_RESET); //设置屏幕唤醒时进入低功耗模式的倒计时
    while (true) {
        //ap&sta关闭、且当前在主页、且超过5min未触摸时才进入低功耗
        do {
            /*
            调用此函数为当前运行的任务重置Task Watchdog Timer (TWDT) 任务监视计时器
            每个订阅TWDT的任务都必须周期性地调用此函数来预防TWDT计时超时
            如果单个或多个订阅了TWDT的任务没有在任务函数中重置TWDT 那TWDT将会发生超时
            如果空闲任务订阅了TWDT，它将会在空闲钩子任务中自动地重置TWDT(空闲任务一般在使用Freertos时会涉及)
            在从未订阅TWDT或者TWDT没有初始化的任务中调用此函数将会返回错误信息。
            */
            esp_task_wdt_reset();
            vTaskDelay(pdMS_TO_TICKS(1000)); // 1s的任务阻塞延时
            count_tp_wackup_timeleft();  // 开启从运行模式进入低功耗模式的倒计时
            printf("wait enter sleep mode run... %d\n",get_tp_wackup_timeleft());
            if(get_is_ap_sta_open() == false && ds_ui_get_now_show_page() == PAGE_TYPE_MEMU && get_tp_wackup_timeleft() == 0){
                break;
            }
        } while (1);

        ds_touch_gpio_isr_remove();  // 调用了gpio_isr_handler_remove(GPIO_INPUT_IO_0)函数移除GPIO0输入模式的中断处理器
        /*
        第一个参数是要唤醒的GPIO管脚号，宏定义BUTTON_GPIO_NUM_DEFAULT为4
        第二个参数是高电平或低电平触发，这里是低电平触发
        调用此函数开启触屏TP的中断引脚(GPIO4)作为低电平唤醒功能，即只要外界触摸屏幕就进入唤醒模式。
        */
        gpio_wakeup_enable(BUTTON_GPIO_NUM_DEFAULT,GPIO_INTR_LOW_LEVEL);
        /* Wake up in 60 seconds, or when button is pressed */
        esp_sleep_enable_timer_wakeup(60000000);
        /*
        开启唤醒功能需要先调用gpio_wakeup_enable函数指定GPIO号和开启唤醒模式的电平触发方式
        然后调用esp_sleep_enable_gpio_wakeup()函数开启唤醒功能
        每个GPIO管脚都支持唤醒功能并且可以通过选择电平触发的方式来开启唤醒功能
        与EXT0和EXT1唤醒源不同的是，此方法适用于所有IO口：包括RTC IO口和普通的数字IO口 不过它们只能用来唤醒轻睡眠模式。
        */
        esp_sleep_enable_gpio_wakeup();

        printf("Entering light sleep\n");

        /* Get timestamp before entering sleep */
        int64_t t_before_us = esp_timer_get_time();

        /* Enter sleep mode */
        esp_light_sleep_start();
        /* Execution continues here after wakeup */

        /* Get timestamp after waking up from sleep */
        int64_t t_after_us = esp_timer_get_time();
        /* Determine wake up reason */
        const char* wakeup_reason;
        uint32_t wackup_timeleft = 60; // 设置TP唤醒剩余时间，用于进入休眠的倒计时。
        switch (esp_sleep_get_wakeup_cause()) {  // 获取唤醒源从睡眠中唤醒的原因
            case ESP_SLEEP_WAKEUP_TIMER:         // 唤醒原因为定时器唤醒
                wakeup_reason = "timer";
                wackup_timeleft = 1;
                update_system_time_minute();
                break;
            case ESP_SLEEP_WAKEUP_GPIO:         // 唤醒原因为GPIO引脚触发
                wakeup_reason = "pin";
                gpio_wakeup_disable(BUTTON_GPIO_NUM_DEFAULT);  // 关闭TP中断引脚的唤醒功能
                //关闭所有唤醒源 此函数用于禁用所有作为参数传递给函数的触发源 但不会修改RTC中的唤醒配置 它将在esp_sleep_start函数中执行
                esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
                ds_touch_gpio_init();           // 触摸屏TP的GPIO初始化
                ds_touch_gpio_isr_add();        // 保障中断能有效地被添加再次调用add函数
                reset_tp_action_manage();       // 初始化触摸屏管理相关结构体中的唤醒标志位
                break;
            default:
                wakeup_reason = "other";
                gpio_wakeup_disable(BUTTON_GPIO_NUM_DEFAULT);
                esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
                ds_touch_gpio_init();
                ds_touch_gpio_isr_add();
                reset_tp_action_manage();
                break;
        }
        printf("Returned from light sleep, reason: %s, t=%lld ms, slept for %lld ms  wackup_timeleft=%d\n",
                wakeup_reason, t_after_us / 1000, (t_after_us - t_before_us) / 1000,wackup_timeleft);

        set_tp_wackup_timeleft(wackup_timeleft);
    }
}

static void background_task(void* arg)
{
    int apsta_close_count = 0;
    for(;;){
        printf("background_task run... \n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        //首次更新
        if(has_first_time_httpdata_request() == true && get_wifi_sta_status() == WIFI_STA_MODE_CONNECT_SUCCESS){
            ds_http_request_all();      // 首次连接，更新所有http相关内容
            set_has_first_time_httpdata_request();  // 设置首次更新标志位为否
        }
        //下拉请求更新
        if(has_update_httpdata_request() == true){  // 获取刷新http数据更新请求标志位
            if(get_is_ap_sta_open()){       // 是否开启WIFI的AP和STA模式
                if(get_wifi_sta_status() == WIFI_STA_MODE_CONNECT_SUCCESS){  // 设备是否连上热点
                    ds_http_request_all();
                    set_update_httpdata_request(false);     // 设置http数据更新请求标志位为否
                }
            }else{
                // 如果没有打开WIFI的AP和STA需要调用WIFI事件发送函数对它们进行开启
                ds_wifi_send_event(AP_STA_START);  // 函数内部调用队列发送函数将项目发送到队列上。
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
        }
        //关闭wifi AP&STA 计时
        if(get_is_ap_sta_open() == true && apsta_close_count == 0){
            //重置10min计时
            apsta_close_count = (SLEEP_TIME_RESET - 10);
        }
        if(apsta_close_count > 0){
            apsta_close_count --;
            if(apsta_close_count == 0){
                ds_wifi_send_event(AP_STA_STOP);   // 倒计时计数器减到0时 调用WIFI事件发送函数关闭AP和STA模式
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                set_update_httpdata_request(false); // 设置首次更新标志位为否
            }
        }
        // 如果屏幕处于设置页面 且没有AP和STA模式没有打开 就调用WIFI事件发送函数启动它们
        if(ds_ui_get_now_show_page() == PAGE_TYPE_SETTING){
            if(get_is_ap_sta_open() == false){
                ds_wifi_send_event(AP_STA_START);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
        }
    }
}

void app_main(void)
{
    printf("----- app start! -----\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU cores, WiFi%s%s, ",
            CHIP_NAME,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("sw revision V4.0.2 \n");

    //看门狗初始化
    // esp_task_wdt_init(TWDT_TIMEOUT_S, false);
    // esp_task_wdt_add(NULL);
    //系统相关数据初始化 (时间，wifi信息，http首次数据请求标志位1，http刷新数据请求标志位0)
    ds_system_data_init();
    ESP_ERROR_CHECK(nvs_flash_init());    // 初始化默认NVS分区
    ESP_ERROR_CHECK(esp_netif_init());    // 初始化底层TCP/IP栈。在应用程序启动时，此函数必须至少在程序中调用一次。
    ESP_ERROR_CHECK(esp_event_loop_create_default());  // 创建默认事件循环

    //http请求初始化
    ds_http_request_init();
    ds_http_server_init();
    //ap&sta模式初始化
    ds_wifi_ap_sta_init();   

    // 屏幕、TP外设初始化相关
    init_ft6336();
    init_screen_interface();
    // 屏幕显示数据初始化
    ds_ui_timepage_init();
    ds_ui_page_manage_init();
    ds_ui_weather_init();
    ds_ui_wordpage_init();
    ds_ui_tomatopage_init();
    ds_ui_fans_init();
    //定时器初始化
    ds_timer_init();
    //PWM蜂鸣器初始化
    ds_pwm_init();

    //默认Wifi账号密码设置
    // char *ssid="leo";
    // char *psw="123456789";
    // ds_nvs_save_wifi_info(ssid,psw);
    if(NVS_WIFI_INFO_HAS_SAVE == ds_nvs_read_wifi_info()){
        //已经存储wifi联网信息
        ds_ui_page_manage_send_action(PAGE_TYPE_MEMU);
    }else{
        //未存储wifi联网信息
        ds_ui_page_manage_send_action(PAGE_TYPE_SETTING);
    }

    //启动AP&STA模式
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    ds_wifi_send_event(AP_STA_START);
    xTaskCreate(background_task, "background_task", 4096, NULL, 10, NULL); 
    
    //进入低功耗模式
    sleep_mode_init();

    // char pWriteBuffer[1048];
    // init_screen_interface();
    // test_SSD1681();

    for(;;){
        printf("app run \n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        //喂狗
        esp_task_wdt_reset();  //Comment this line to trigger a TWDT timeout

        //堆栈信息打印，需打开USE_TRACE_FACILITY、USE_STATS_FORMATTING_FUNCTIONS
        // vTaskList((char *)&pWriteBuffer);
        // printf("task_name   task_state  priority   stack  tasK_num\n");
        // printf("%s\n", pWriteBuffer);  
    }
    esp_task_wdt_delete(NULL);
}
