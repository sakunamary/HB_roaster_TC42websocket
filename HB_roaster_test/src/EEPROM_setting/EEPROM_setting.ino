/*
   EEPROM Write

   Stores random values into the EEPROM.
   These values will stay in the EEPROM when the board is
   turned off and may be retrieved later by another sketch.
*/

#include "EEPROM.h"
#define BAUDRATE 115200
//pwm setting 
#define PWM_FREQ 10000
#define PWM_RESOLUTION 12 //0-4096

// 网页设置的参数
 typedef struct eeprom_settings 
{
  char ssid[60]; //增加到30个字符
  char password[60]; //增加到30个字符
  int PWM_FREQ_HEAT;
  bool   Init_mode ; //是否初始化模式
} user_wifi_t;

extern user_wifi_t  user_wifi ;



user_wifi_t user_wifi = {
                        " ", //char ssid[60]; //增加到30个字符
                        " ", //char password[60]; //增加到30个字符
                        PWM_FREQ,//int PWM_FREQ_HEAT;
                        false //bool   Init_mode ; //是否初始化模式
                        };


void setup()
{
  Serial.begin(BAUDRATE);
  Serial.println("start...");
  if (!EEPROM.begin(sizeof(user_wifi)))
  {
    Serial.println("failed to initialise EEPROM"); 
    delay(1000000);
  } else {
    Serial.println("Initialed EEPROM,data will be writen after 3s..."); 
    delay(3000);
    EEPROM.get(0, user_wifi);
    
    strcat(user_wifi.ssid,"");
    strcat(user_wifi.password,"");
    user_wifi.Init_mode = false ;
    user_wifi.PWM_FREQ_HEAT = PWM_FREQ;

    EEPROM.put(0, user_wifi);
    EEPROM.commit();
  }

  Serial.println(" bytes read from Flash . Values are:");

  for (int i = 0; i < sizeof(user_wifi); i++)
  {
    Serial.print(byte(EEPROM.read(i))); Serial.print(" ");
  }

}


void loop()
{
  
}
