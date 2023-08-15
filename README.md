

## 

## 电路图

![image](https://github.com/Nero-iwnl/DesktopScreen4H/blob/main/images/dianlu.png)

## 触控逻辑

![image](https://github.com/Nero-iwnl/DesktopScreen4H/blob/main/images/touch_logi.png)

## 主页

![image](https://github.com/Nero-iwnl/DesktopScreen4H/blob/main/images/homepage.jpg)

## 时钟

![image](https://github.com/Nero-iwnl/DesktopScreen4H/blob/main/images/oclock.jpg)

## 天气

![image](https://github.com/Nero-iwnl/DesktopScreen4H/blob/main/images/weather.jpg)



![image](https://github.com/Nero-iwnl/DesktopScreen4H/blob/main/images/id_pic.jpg)







## 配置环境

get_idf

## 配置 
idf.py menuconfig

## 编译
 idf.py build

## 清除
idf.py fullclean

## 下载
idf.py -p /dev/ttyUSB0 flash

## 监视器
idf.py -p /dev/ttyUSB0 monitor

## 构建、下载、监视
idf.py -p /dev/ttyUSB0 flash monitor

## 擦除
idf.py -p /dev/ttyUSB0 erase-flash

## 编译成功提示
Project build complete.

## 字体库下载
python esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 115200 write_flash -z 0x15D000 myFont1.bin