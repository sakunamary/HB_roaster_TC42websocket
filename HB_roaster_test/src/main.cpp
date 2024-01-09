#include <Arduino.h>
#include "config.h"



#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>



#include <StringTokenizer.h>


#include "ArduinoJson.h"

#include <pwmWrite.h>
#include <ESP32Encoder.h>


#include "esp_task_wdt.h"

#include <ModbusIP_ESP8266.h>

SemaphoreHandle_t xGetDataMutex = NULL;

char ap_name[30] ;
uint8_t macAddr[6];

String local_IP;
String MsgString;

String MSG_token1300[4];

int cmd_chan1300 = 0;

String MsgString_1300="";
int heat_level_to_artisan = 0;

int16_t  heat_from_Hreg = 0;
int16_t  heat_from_enc  = 0;

//data_to_artisan_t To_artisan = {0.0,0.0,0.0,0.0,0};

extern bool loopTaskWDTEnabled;
extern TaskHandle_t loopTaskHandle;

const uint32_t frequency = PWM_FREQ;
const byte resolution = PWM_RESOLUTION; //pwm -0-4096

AsyncWebServer server(80);

//Modbus Registers Offsets
const uint16_t BT_HREG = 3001;
const uint16_t ET_HREG = 3002;
//const uint16_t AP_HREG = 3003;
//const uint16_t INLET_HREG = 3004;
const uint16_t HEAT_HREG = 3005;

//Coil Pins
const int HEAT_OUT_PIN = PWM_HEAT; //GPIO26


static IRAM_ATTR void enc_cb(void* arg);
void task_send_Hreg(void *pvParameters);
void task_get_data(void *pvParameters);



//ModbusIP object
ModbusIP mb;


//pwm object 
Pwm pwm = Pwm();


ESP32Encoder encoder(true);
static IRAM_ATTR void enc_cb(void* arg) {
  ESP32Encoder* enc = (ESP32Encoder*) arg;
}



void task_get_data(void *pvParameters)
{ //function 

    /* Variable Definition */
    (void)pvParameters;
    TickType_t xLastWakeTime;

    const TickType_t xIntervel = 1000/ portTICK_PERIOD_MS;
    /* Task Setup and Initialize */
    // Initial the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();
    int i = 0;
    for (;;) // A Task shall never return or exit.
    { //for loop
        // Wait for the next cycle (intervel 1s).
         vTaskDelayUntil(&xLastWakeTime, xIntervel);

        Serial.print("CHAN;1300\n");
        Serial.flush();
        vTaskDelay(100);
        while (Serial.read() >=0 ) {}//clean buffer
        Serial.print("READ\n");
        vTaskDelay(400);

            if(Serial.available()){
                MsgString_1300 = Serial.readStringUntil('C');
                Serial.printf("\nSerial input:%s\n",MsgString_1300);
                MsgString_1300.concat('C');

            } 

            while (Serial.read() >=0 ) {}//clean buffer
            StringTokenizer tokens1300(MsgString_1300, ",");

            while(tokens1300.hasNext()){
                    MSG_token1300[i]=tokens1300.nextToken(); // prints the next token in the string
                    i++;
                }
                    if (xSemaphoreTake(xGetDataMutex, xIntervel) == pdPASS) 
                        {

                        mb.Hreg(BT_HREG,int(MSG_token1300[1].toDouble() *100)); //3001
                        mb.Hreg(ET_HREG,int(MSG_token1300[2].toDouble() *100)); //3002
                           // To_artisan.BT = MSG_token1300[1].toDouble();
                           // To_artisan.ET = MSG_token1300[2].toDouble();
                           // mb.Hreg(BT_HREG,int(To_artisan.BT *100));
                           // mb.Hreg(ET_HREG,int(To_artisan.ET *100));
                        xSemaphoreGive(xGetDataMutex);  //end of lock mutex
                    } //释放mutex
            MsgString_1300 = "";    
            i=0;    
            Serial.flush();
            while (Serial.read() >=0 ) {}//clean buffer
        }
}//function 


void setup() {

    xGetDataMutex = xSemaphoreCreateMutex();
    loopTaskWDTEnabled = true;

    pinMode(HEAT_OUT_PIN, OUTPUT); 

    Serial.begin(BAUDRATE);
#if defined(DEBUG_MODE)
    Serial.printf("\nHB_WIFI  STARTING...\n");
#endif

  //初始化网络服务

    WiFi.macAddress(macAddr); 
    // Serial_debug.println("WiFi.mode(AP):");
    WiFi.mode(WIFI_AP);
    sprintf( ap_name ,"HB_WIFI_%02X%02X%02X",macAddr[0],macAddr[1],macAddr[2]);
    WiFi.softAP(ap_name, "12345678"); // defualt IP address :192.168.4.1 password min 8 digis

#if defined(DEBUG_MODE)
Serial.printf("\nStart Task...\n");
#endif

    /*---------- Task Definition ---------------------*/
    // Setup tasks to run independently.
    xTaskCreatePinnedToCore(
        task_get_data, "get_data" // 获取HB数据
        ,
        4096 // This stack size can be checked & adjusted by reading the Stack Highwater
        ,
        NULL, 2 // Priority, with 1 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
        ,
        NULL,  1 // Running Core decided by FreeRTOS,let core0 run wifi and BT
    );

    Serial.printf("\nTASK1:get_data...\n");

/*
 xTaskCreatePinnedToCore(
        task_send_Hreg, "send_Hreg" // 获取HB数据
        ,
        2048 // This stack size can be checked & adjusted by reading the Stack Highwater
        ,
        NULL, 1 // Priority, with 1 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
        ,
        NULL,  1 // Running Core decided by FreeRTOS,let core0 run wifi and BT
    );

#if defined(DEBUG_MODE)
    Serial.printf("\nTASK2:get_dsend_Hregata...\n");
#endif
*/
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! This is a sample response.");
  });

  AsyncElegantOTA.begin(&server);    // Start AsyncElegantOTA
  server.begin();

#if defined(DEBUG_MODE)
    Serial.println("HTTP server started");
#endif
  //Init pwm output
    pwm.pause();
    pwm.write(PWM_HEAT,0, PWM_FREQ, PWM_RESOLUTION);
    pwm.resume();
    pwm.printDebug();
#if defined(DEBUG_MODE)
    Serial.println("PWM started");  
   
    Serial.printf("\nStart INPUT ENCODER  service...\n");
#endif
//init ENCODER
	// Enable the weak pull up resistors
    ESP32Encoder::useInternalWeakPullResistors=UP;
    encoder.attachSingleEdge(ENC_DT, ENC_CLK);
    encoder.clearCount();
    encoder.setFilter(1023);
    esp_task_wdt_add(loopTaskHandle); //add watchdog for encoder

#if defined(DEBUG_MODE)    
    Serial.println("Encoder started"); 
#endif


//Init Modbus-TCP 
#if defined(DEBUG_MODE)
    Serial.printf("\nStart Modbus-TCP   service...\n");
#endif
    mb.server(502);		//Start Modbus IP //default port :502
    // Add SENSOR_IREG register - Use addIreg() for analog Inputs
    mb.addHreg(BT_HREG);
    mb.addHreg(ET_HREG);
    //mb.addHreg(INLET_HREG);
    //mb.addHreg(AP_HREG);
    mb.addHreg(HEAT_HREG);

    mb.Hreg(BT_HREG,0); //初始化赋值
    mb.Hreg(ET_HREG,0);  //初始化赋值
    //mb.Hreg(INLET_HREG,0); //初始化赋值
    //mb.Hreg(AP_HREG,0);//初始化赋值
    mb.Hreg(HEAT_HREG,0);  //初始化赋值
}

void loop() {

 const TickType_t xIntervel = 500/ portTICK_PERIOD_MS;
//更新寄存器数据
  mb.task();
// pwm output level 
//    PC                                        MCU-value                   ENCODER read
//Artisan-> heat_from_Artisan        >>    To_artisan.heat_level     <<     heat_from_enc
//  heat_from_Artisan == heat_from_enc  in loop（） 

    heat_from_enc = encoder.getCount();

if (xSemaphoreTake(xGetDataMutex, xIntervel) == pdPASS) {  
        heat_level_to_artisan  =  mb.Hreg(HEAT_HREG);//读取数据

       //HEAT 控制部分 
       if ((heat_level_to_artisan  + heat_from_enc) <= 0 && heat_level_to_artisan  >=0 ) { //如果输入小于0值，自动限制在0
            

            heat_level_to_artisan  = 0;
 
            mb.Hreg(HEAT_HREG,heat_level_to_artisan );
            encoder.clearCount();
            heat_from_enc=0;
            

       } else if ((heat_level_to_artisan  + heat_from_enc) >= 100 && heat_level_to_artisan  <= 100){//如果输入大于100值，自动限制在100

            heat_level_to_artisan  = 100;
            mb.Hreg(HEAT_HREG,heat_level_to_artisan );

            encoder.clearCount();
            heat_from_enc=0; 


       } else { //To_artisan.heat_level 加减 encoder的增减量，再同步到

            heat_level_to_artisan  = heat_from_enc + heat_level_to_artisan ;
            mb.Hreg(HEAT_HREG,heat_level_to_artisan );
            encoder.clearCount();
            heat_from_enc=0;

           }
       xSemaphoreGive(xGetDataMutex);  //end of lock mutex
}

       pwm.write(HEAT_OUT_PIN, map(heat_level_to_artisan ,0,100,250,1000), PWM_FREQ, resolution); //自动模式下，将heat数值转换后输出到pwm


}