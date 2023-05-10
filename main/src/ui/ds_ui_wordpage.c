

#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "ds_screen.h"
#include "ds_spi.h"
#include "ds_ui_page_manage.h"
#include "ds_data_icon.h"
#include "ds_ui_wordpage.h"
#include "ds_system_data.h"
#include "ds_data_page.h"

#include "ds_paint.h"
#include "ds_data_icon.h"
#include "ds_ui_mainpage.h"
#include "ds_data_num.h"

typedef struct 
{
    uint8_t init_status;
    uint8_t updateing;
    uint8_t num;
    uint8_t counter;
}WORD_PAGE_T;

WORD_PAGE_T g_word_page;

//200*200 像素屏幕适配，偏移值
static int offset_v = 29; 
static int offset_h = 24; 


void ds_ui_wordpage_init(){

    memset(&g_word_page,0,sizeof(WORD_PAGE_T));
    g_word_page.updateing = 0;
    g_word_page.counter = 0;
    g_word_page.num = 0;

}

/* New Function for Update Counter Page */
void ds_ui_counterpage_show_counter_init(uint8_t init){
    g_word_page.updateing = 1;
    int num_size = 32;
    int num_size_y = 48;
	int vertical = 35 + offset_v; //垂直位置
    int horizontal = 8 + offset_h;  //水平位置
    int now_index;
    if (init == 0)
        g_word_page.counter = 0;
    else if (init == 1)
        g_word_page.counter ++;
    else
        g_word_page.counter --;

    ds_screen_partial_data_init();

    // 百位
    now_index = g_word_page.counter / 100;
    ds_screen_partial_data_add(horizontal+num_size,horizontal+num_size*2,vertical,vertical+num_size_y,gImage_num_size48[now_index]);
    // 十位
    if (g_word_page.counter > 100)
        g_word_page.counter %= 100;
    now_index = g_word_page.counter / 10;
    ds_screen_partial_data_add(horizontal+num_size*2+3,horizontal+num_size*3+3,vertical,vertical+num_size_y,gImage_num_size48[now_index]);
    // 个位
    now_index = g_word_page.counter % 10;
    ds_screen_partial_data_add(horizontal+num_size*3+3,horizontal+num_size*4+3,vertical,vertical+num_size_y,gImage_num_size48[now_index]);

    //返回
    //size 36*36
    num_size = 36;
    vertical = 0;
    ds_screen_partial_data_add(0,num_size,vertical,vertical+num_size,gImage_back);
    ds_screen_partial_data_copy();
    g_word_page.init_status = 1;
    g_word_page.updateing = 0;
}
