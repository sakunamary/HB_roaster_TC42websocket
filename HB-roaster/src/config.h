
#ifndef __CONFIG_H__
#define __CONFIG_H__


#define BAUDRATE 115200  //serial port baudrate

//定义artisan 交互的数组
struct  data_to_artisan {
    double temp_env;//环境温度
    double humi_env;//环境湿度
    double  amp_env;//环境大气压强

 } To_artisan ={10.0,10.0,980.00};
//end of 定义artisan 交互的数组


//定义M1 锅炉的
struct  data_HB_out {
    int stat ; //状态 0（正常运转） 4（初始状态，未发prodcode，无法加热） 1 （手动火力） 
    String AHT ; //自动加热目标温度： auto heater temp
    String HPM ; //加热模式：A 自动/M手动
    double TC1 ; // 豆温  data_to_artisan.temp_bean
    double TC2 ; // 风温  data_to_artisan.temp_dumer
    int HT1P ; //自动模式加热:数值    HPM = A -> data_to_artisan.heater_power
    int HT1PMHV ; //手动设置火力模式：数值   HPM = M -> data_to_artisan.heater_power
    int RL; //搅拌速度 data_to_artisan.roll
    int SM;//排烟速度 data_to_artisan.fan
    int F3 ; //冷却速度
} data_HB={4 ,"0","-",15.0,15.0,0,0,0,0,0};
// end of 设置输出命令的结构数据


// 网页设置的参数
 typedef struct eeprom_settings 
{
  char ssid[60]; //增加到30个字符
  char password[60]; //增加到30个字符
} user_wifi_t;

extern user_wifi_t  user_wifi ;






#endif       