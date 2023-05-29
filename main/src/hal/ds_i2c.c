/*
 * @Author: Jinsc
 * @Date: 2023-05-18 15:06:56
 * @LastEditors: Jinsc
 * @LastEditTime: 2023-05-29 15:08:45
 * @FilePath: /esp/DesktopScreen4H/main/src/hal/ds_i2c.c
 * @Description: 
 * Copyright (c) 2023 by jinsc123654@gmail.com All Rights Reserved. 
 */
/* i2c - Example

   For other examples please check:
   https://github.com/espressif/esp-idf/tree/master/examples

   See README.md file to get detailed usage of this example.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "ds_i2c.h"

//设置读取地址
static esp_err_t i2c_master_set_addr(uint8_t u8Cmd){
    // 创建一个 I2C 命令句柄 cmd，用于存储 I2C 命令的链表
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);// 添加开始信号的命令到命令链表中，表示开始 I2C 通信
    // 将从设备地址和写入位组合成一个字节，并添加写入地址的命令到命令链表中
    i2c_master_write_byte(cmd, (ESP_SLAVE_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);
    // 添加写入命令的数据到命令链表中，即要写入的数据
    i2c_master_write_byte(cmd, u8Cmd, ACK_CHECK_EN);
    i2c_master_stop(cmd);// 添加停止信号的命令到命令链表中，表示结束 I2C 通信
    // 执行 I2C 命令链表中的命令，并返回执行结果的错误代码
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);// 删除命令链表，释放内存
    if (ret != ESP_OK) {
        printf("i2c_master_set_addr error !! check tp is connect ?\n");
    }
    return ret;
}

//读取数据
esp_err_t i2c_master_read_slave(uint8_t u8Cmd, uint8_t *data_rd, size_t size)
{
    if (size == 0) {
        return ESP_OK;
    }
    i2c_master_set_addr(u8Cmd);// 函数设置读取地址
    // 创建一个 I2C 命令句柄 cmd，用于存储 I2C 命令的链表
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);// 添加开始信号的命令到命令链表中，表示开始 I2C 通信
    // 将从设备地址和读取位组合成一个字节，并添加读取地址的命令到命令链表中
    i2c_master_write_byte(cmd, (ESP_SLAVE_ADDR << 1) | READ_BIT, ACK_CHECK_EN);
    // 循环读取数据，将每个字节添加到命令链表中
    for(int index=0;index<(size-1);index++)
	{	   
		i2c_master_read_byte(cmd, data_rd+index, ACK_VAL);
	}
    // 循环读取数据，将每个字节添加到命令链表中
	i2c_master_read_byte(cmd, data_rd+size-1, NACK_VAL);
    i2c_master_stop(cmd);// 添加停止信号的命令到命令链表中，表示结束 I2C 通信
    // 执行 I2C 命令链表中的命令，并返回执行结果的错误代码
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        printf("i2c_master_read_slave error !! check tp is connect ?\n");
    }
    return ret;
}

//写入数据
esp_err_t i2c_master_write_slave(uint8_t u8Cmd, uint8_t *data_wr, size_t size)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ESP_SLAVE_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, u8Cmd, ACK_CHECK_EN);
    i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        printf("i2c_master_write_slave error\n");
    }
    return ret;
}

/**
 * @brief i2c master initialization
 */
esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;       // 存储要使用的 I2C 主设备的编号
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;                // 设置 I2C 主设备的工作模式为主设备模式
    conf.sda_io_num = I2C_MASTER_SDA_IO;        // 设置 SDA 引脚的编号，用于连接 I2C 总线上的数据线
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;    // 启用 SDA 引脚的上拉电阻
    conf.scl_io_num = I2C_MASTER_SCL_IO;        // 设置 SCL 引脚的编号，用于连接 I2C 总线上的时钟线
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;    // 启用 SCL 引脚的上拉电阻
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ; // 设置 I2C 主设备的时钟频率
    i2c_param_config(i2c_master_port, &conf);
    // 安装 I2C 驱动程序并返回执行结果。该函数将指定编号的 I2C 主设备初始化为指定的模式，并禁用接收和发送缓冲区
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}


