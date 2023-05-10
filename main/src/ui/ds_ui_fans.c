#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "ds_screen.h"
#include "ds_spi.h"
#include "ds_ui_page_manage.h"
#include "ds_system_data.h"
#include "ds_data_num.h"
#include "ds_data_icon.h"

#include "ds_paint.h"
#include "ds_data_icon.h"
#include "ds_ui_mainpage.h"
#include "ds_data_page.h"


typedef struct 
{
    uint8_t updateing;
    uint8_t page_num;
    char show_days[10];
}FANS_PAGE_T;

FANS_PAGE_T g_fans_page;


void int_to_char(char *result,int val){
    int depth=1;
    int len = 1;
    int temp=val;
    while(temp>=10){
        temp=temp/10;
        depth = depth * 10;
        len ++;
    }
    for(int index = 0;index < len;index ++){
        temp = val/depth;
        result[index] = temp + '0';
        val = val - temp * depth;
        depth = depth / 10;
    }
    result[len] = '\0';
}

int page_count = 0;

void ds_ui_fans_show_init(uint8_t type){
    if (type == 0){
        uint16_t days = get_system_data().total_days;
        printf("**********************************************\n");
        printf("***************  days (int):%d  ***************\n", days);
        printf("**********************************************\n");
        sprintf(g_fans_page.show_days, "%d", days);
        printf("**********************************************\n");
        printf("***************  days (char):%s  ***************\n", g_fans_page.show_days);
        printf("**********************************************\n");
        g_fans_page.updateing = 1;
        uint8_t *m_custom_image;
        if ((m_custom_image = (uint8_t *)malloc(IMAGE_SIZE)) == NULL) {
            printf("Failed to apply for black memory...\r\n");
            return;
        }
        Paint_NewImage(m_custom_image, EPD_2IN9BC_WIDTH, EPD_2IN9BC_HEIGHT, 0, WHITE);
        Paint_SelectImage(m_custom_image);
        Paint_Clear(WHITE);
        Paint_DrawString_piture(0,0,gImage_back,icon_back_len,36, WHITE, BLACK);

        char *words = "糊子和橙子";
        Paint_DrawString_CN(40, 100, words, WHITE, BLACK);
        Paint_DrawString_CN(75, 130, g_fans_page.show_days, WHITE, BLACK);
        Paint_DrawString_CN(85, 160, "天", WHITE, BLACK);

        ds_screen_full_display(ds_paint_image_new);
        // ds_paint_image_copy();
        free(m_custom_image);
        // g_fans_page.updateing = 0;
    }
    else{
        ds_screen_pictures();
    }
}

void ds_ui_fans_init(){
    memset(&g_fans_page,0,sizeof(FANS_PAGE_T));
    g_fans_page.updateing = 0;
    g_fans_page.page_num = 0;
}