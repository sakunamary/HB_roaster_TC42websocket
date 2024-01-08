
#ifndef __CONFIG_H__
#define __CONFIG_H__


#define VERSION "1.0.5"


#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define BAUDRATE 115200  //serial port baudrate


//#define DEBUG_MODE 

//pinout setting
#define ENC_CLK  33
#define ENC_DT   32
#define PWM_HEAT 26


//pwm setting 
#define PWM_FREQ 10000
#define PWM_RESOLUTION 10 //0-1024


//定义artisan 交互的数组
typedef struct  data_to_artisan {
    double BT;
    double ET;
    double  AP;
    double inlet;
    int heat_level;
/*
            AT = float(res1300[0])
            ET = float(res1300[1])
            BT = float(res1300[2])
            Inlet =float(res2400[1])
*/

 } data_to_artisan_t ;
 extern data_to_artisan_t To_artisan;
//end of 定义artisan 交互的数组

#endif