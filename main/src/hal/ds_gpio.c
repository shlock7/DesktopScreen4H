
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#include "ds_gpio.h"
#include "ds_ft6336.h"
#include "ds_system_data.h"

/**
 * GPIO status:
 * GPIO5: output
 * GPIO4:  input, pulled up, interrupt from rising edge 
 */
//定义IO口，查硬件图
#define GPIO_OUTPUT_IO_0    5
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0))
/*这个宏定义表示一个GPIO输出引脚的掩码
  使用位运算符将1左移GPIO_OUTPUT_IO_0的值所表示的位数
  然后使用ULL（无符号长长整型）将结果转换为64位无符号整数
  这样就创建了一个只有GPIO_OUTPUT_IO_0引脚位置上为1，其他位都为0的掩码*/
#define GPIO_INPUT_IO_0     4
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0))
#define ESP_INTR_FLAG_DEFAULT 0

//屏幕片选 0-有效
#define SCREEN_GPIO_OUTPUT_CS 27
#define SCREEN_GPIO_OUTPUT_CS_SEL ((1ULL<<SCREEN_GPIO_OUTPUT_CS))
//屏幕数据/指令选择 1-data 0-cmd
#define SCREEN_GPIO_OUTPUT_DC 14
#define SCREEN_GPIO_OUTPUT_DC_SEL ((1ULL<<SCREEN_GPIO_OUTPUT_DC))
//屏幕复位 0-reset
#define SCREEN_GPIO_OUTPUT_RES 12
#define SCREEN_GPIO_OUTPUT_RES_SEL ((1ULL<<SCREEN_GPIO_OUTPUT_RES))
//屏幕状态 1-busy 
#define SCREEN_GPIO_INTPUT_BUSY 13
#define SCREEN_GPIO_INTPUT_BUSY_SEL ((1ULL<<SCREEN_GPIO_INTPUT_BUSY))


static xQueueHandle gpio_evt_queue = NULL;
/*
定义了一个处理GPIO中断的函数，当发生中断时
它将中断所涉及的GPIO引脚编号（通过参数传递）发送到消息队列gpio_evt_queue中
这样，其他部分的代码可以从该队列中接收并处理GPIO中断事件
*/
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
            if(io_num == GPIO_INPUT_IO_0){  // 假如读取到缓冲区的数据，即GPIO口为T_INT(IO4)
                set_tp_wackup_timeleft(60); // 设置睡眠倒计时10分钟
                if(gpio_get_level(io_num) == 0){
                    // 假如GPIO为低电平触发，则初始化触摸屏管理相关结构体中的唤醒标志位(1)
                    reset_tp_action_manage();
                }else{
                    // 如果为高电平则检查当前屏幕被触摸的方式
                    check_tp_action();
                }
            }
        }
    }
}

//触摸屏GPIO口初始化
void ds_touch_gpio_init(){
    static bool has_init_isr = false;  // 初始化中断标志位
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;  // 关闭GPIO口中断模式
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;    // 配置GPIO输出模式
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;  // 设置管脚的位掩码
    //disable pull-down mode
    io_conf.pull_down_en = 0;  // 关闭下拉模式
    //disable pull-up mode
    io_conf.pull_up_en = 0;    // 关闭上拉模式
    //configure GPIO with the given settings
    gpio_config(&io_conf);  // io_conf结构体用来对TP的IO5口，也就是T_RST引脚进行配置

    //GPIO interrupt type : both rising and falling edge
    io_conf.intr_type = GPIO_INTR_ANYEDGE; 
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode    
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);  // IO4口TP的T_INT引脚进行配置

    //change gpio intrrupt type for one pin
    // gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_NEGEDGE);

    if(has_init_isr == false){
        //create a queue to handle gpio event from isr
        gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
        //start gpio task
        xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

        //install gpio isr service
        /*
		安装了ESP32芯片的GPIO中断服务 它将初始化并启用中断处理机制，使得可以使用GPIO中断功能
		ESP_INTR_FLAG_DEFAULT是一个标志参数，表示使用默认的中断配置选项
		*/
        gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);  // 配置GPIO中断
        //hook isr handler for specific gpio pin
        /*
		将指定的中断处理函数gpio_isr_handler添加到GPIO输入引脚上的中断处理函数列表中
		当GPIO输入引脚触发中断时，将调用该中断处理函数来处理中断事件
		GPIO_INTPUT_IO_0是GPIO输入引脚的编号。gpio_isr_handler是中断处理函数的名称。
		(void*)GPIO_INTPUT_IO_0将GPIO输入引脚的编号转换为void*类型的参数
		然后传递给中断处理函数。这允许在中断处理函数中获取和使用引脚编号信息
		*/
        gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0); // 配置GPIO中断后需要调用此函数添加对应的GPIO中断服务
    }
    has_init_isr = true;
}

void ds_touch_gpio_isr_remove(){
    gpio_isr_handler_remove(GPIO_INPUT_IO_0);
}

void ds_touch_gpio_isr_add(){
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
}

void ds_screen_gpio_init(){
    gpio_config_t io_conf;  // GPIO配置结构体
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = SCREEN_GPIO_OUTPUT_CS_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf); //初始化片选

    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = SCREEN_GPIO_OUTPUT_DC_SEL;
    //configure GPIO with the given settings
    gpio_config(&io_conf); //初始化D/C

    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = SCREEN_GPIO_OUTPUT_RES_SEL;
    //configure GPIO with the given settings
    gpio_config(&io_conf); //复位

    io_conf.intr_type = GPIO_INTR_NEGEDGE; // 下降沿触发
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = SCREEN_GPIO_INTPUT_BUSY_SEL;
    //set as input mode    
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
}

void ds_gpio_init(){
    ds_touch_gpio_init();
}

void ds_gpio_set_screen_cs(uint32_t level){
    gpio_set_level(SCREEN_GPIO_OUTPUT_CS, level);
}

void ds_gpio_set_screen_dc(uint32_t level){
    gpio_set_level(SCREEN_GPIO_OUTPUT_DC, level);
}

void ds_gpio_set_screen_rst(uint32_t level){
    gpio_set_level(SCREEN_GPIO_OUTPUT_RES, level);
}

int ds_gpio_get_screen_busy(){
    return gpio_get_level(SCREEN_GPIO_INTPUT_BUSY);
}

void ds_gpio_set_touch_rst(uint32_t level){
    gpio_set_level(GPIO_OUTPUT_IO_0, level);
}

