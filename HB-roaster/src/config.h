
#ifndef __CONFIG_H__
#define __CONFIG_H__


#define BAUDRATE 115200  //serial port baudrate
#define VERSION "1.0.0"

#define DEBUG_MODE

//定义artisan 交互的数组
struct  data_to_artisan {
    double temp_env;//环境温度
    double humi_env;//环境湿度
    double  amp_env;//环境大气压强
    double bt;
    double et;
    double Exhaust;
    double Inlet;
    double Null_data;
    double AT;

 } To_artisan ={10.0,10.0,980.00,0.0,0.0,0.0,0.0,0.0};
//end of 定义artisan 交互的数组



// 网页设置的参数
 typedef struct eeprom_settings 
{
  char ssid[60]; //增加到30个字符
  char password[60]; //增加到30个字符
} user_wifi_t;

extern user_wifi_t  user_wifi ;



#endif       