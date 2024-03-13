
#ifndef __CONFIG_H__
#define __CONFIG_H__


#define VERSION "1.0.7"


#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define BAUDRATE 115200  //serial port baudrate


//#define DEBUG_MODE 

//pinout setting
#define ENC_BUTTON 35
#define ENC_CLK  33
#define ENC_DT   32
#define PWM_HEAT 26


//pwm setting 
#define PWM_FREQ 10000
#define PWM_RESOLUTION 10 //0-1024


#endif