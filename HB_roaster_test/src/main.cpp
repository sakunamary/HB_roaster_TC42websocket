#include <Arduino.h>
#include "config.h"
#include "EEPROM.h"


  #include <WiFi.h>
  #include <WiFiClient.h>
  #include <WebServer.h>
  #include <ElegantOTA.h>

//#include <HardwareSerial.h>

#include <StringTokenizer.h>
//#include <WebSerial.h>
//

#include "ArduinoJson.h"

#include <pwmWrite.h>
#include <ESP32Encoder.h>
#include "esp_task_wdt.h"

#include <ModbusIP_ESP8266.h>



//HardwareSerial Serial(0);
//HardwareSerial Serial(2);
SemaphoreHandle_t xGetDataMutex = NULL;

unsigned long ota_progress_millis = 0;

char ap_name[30] ;
uint8_t macAddr[6];

String local_IP;
String MsgString;

String MSG_token1300[4];
String MSG_token2400[4];
bool cmd_chan1300 = true;

String MsgString_1300;
String MsgString_2400;

int16_t  heat_from_Hreg = 0;
int16_t  heat_from_enc  = 0;


user_wifi_t user_wifi = {
                        " ", //char ssid[60]; //增加到30个字符
                        " ", //char password[60]; //增加到30个字符
                        PWM_FREQ,//int PWM_FREQ_HEAT;
                        false //bool   Init_mode ; //是否初始化模式
                        };

data_to_artisan_t To_artisan = {1.0,2.0,3.0,4.0,0};

extern bool loopTaskWDTEnabled;
extern TaskHandle_t loopTaskHandle;

const uint32_t frequency = PWM_FREQ;
const byte resolution = PWM_RESOLUTION; //pwm -0-4096


//Modbus Registers Offsets
const uint16_t BT_HREG = 3001;
const uint16_t ET_HREG = 3002;
const uint16_t AP_HREG = 3003;
const uint16_t INLET_HREG = 3005;
const uint16_t HEAT_HREG = 3004;

//Coil Pins
const int HEAT_OUT_PIN = PWM_HEAT; //GPIO26



//void notFound(AsyncWebServerRequest *request);    
//void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);//Handle WebSocket event
//void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){}
String IpAddressToString(const IPAddress &ipAddress);      
static IRAM_ATTR void enc_cb(void* arg);
void task_send_Hreg(void *pvParameters);
void task_get_data(void *pvParameters);

  WebServer server(80);


//ModbusIP object
ModbusIP mb;


//pwm object 
Pwm pwm = Pwm();

// rotary encoder object
ESP32Encoder encoder(true);
static IRAM_ATTR void enc_cb(void* arg) {
  ESP32Encoder* enc = (ESP32Encoder*) arg;
            //enc->clearCount();
}

String IpAddressToString(const IPAddress &ipAddress)
{
    return String(ipAddress[0]) + String(".") +
           String(ipAddress[1]) + String(".") +
           String(ipAddress[2]) + String(".") +
           String(ipAddress[3]);
}



String processor(const String &var)
{
    Serial.println(var);
  if (var == "version")
    {
        return VERSION;
    }
    
    return String();
}




void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
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

        if (cmd_chan1300 == true ) {
            //Serial.println("send chan;1300");
            Serial.write("CHAN;1300\n");
            Serial.flush();
            while (Serial.read() >=0 ) {}//clean buffer
            vTaskDelay(200);
            Serial.write("READ\n");
            Serial.flush();
            vTaskDelay(500);

            if(Serial.available()>0){
                MsgString_1300 = Serial.readStringUntil('C');
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
                            To_artisan.BT = MSG_token1300[1].toDouble();
                            To_artisan.ET = MSG_token1300[2].toDouble();
                            xSemaphoreGive(xGetDataMutex);  //end of lock mutex
                    } //释放mutex
                    MsgString_1300 = "";    
                    i=0;    
            while (Serial.read() >=0 ) {}//clean buffeR
            cmd_chan1300 = false ;

            } else {

            //Serial.println("send chan;1300");
            Serial.write("CHAN;2400\n");
            Serial.flush();
            while (Serial.read() >=0 ) {}//clean buffer
            vTaskDelay(200);
            Serial.write("READ\n");
            Serial.flush();
            vTaskDelay(500);

            if(Serial.available()>0){
                MsgString_2400 = Serial.readStringUntil('C');
                MsgString_2400.concat('C');
            } 

            while (Serial.read() >=0 ) {}//clean buffer
            StringTokenizer tokens2400(MsgString_2400, ",");

            while(tokens2400.hasNext()){
                    MSG_token2400[i]=tokens2400.nextToken(); // prints the next token in the string
                    i++;
                }
                    if (xSemaphoreTake(xGetDataMutex, xIntervel) == pdPASS) 
                        {
                            To_artisan.AP = MSG_token2400[1].toDouble();
                            To_artisan.inlet = MSG_token2400[2].toDouble();
                            xSemaphoreGive(xGetDataMutex);  //end of lock mutex
                    } //释放mutex
                    MsgString_2400 = "";    
                    i=0;    
            while (Serial.read() >=0 ) {}//clean buffeR
            cmd_chan1300 = true ;

        }
     }
}//function 



void task_send_Hreg(void *pvParameters)
{ //function 

    /* Variable Definition */
    (void)pvParameters;
    TickType_t xLastWakeTime;

    const TickType_t xIntervel = 1000/ portTICK_PERIOD_MS;

    /* Task Setup and Initialize */
    // Initial the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();
    for (;;) // A Task shall never return or exit.
       
    { //for loop
    // Wait for the next cycle (intervel 1s).
    vTaskDelayUntil(&xLastWakeTime, xIntervel);
     if (xSemaphoreTake(xGetDataMutex, xIntervel) == pdPASS) 
     {
        mb.Hreg(BT_HREG,int(To_artisan.BT *100));
        mb.Hreg(ET_HREG,int(To_artisan.ET *100));
        mb.Hreg(AP_HREG,int(To_artisan.AP *100));
        mb.Hreg(INLET_HREG,int(To_artisan.inlet *100));
        xSemaphoreGive(xGetDataMutex);  //end of lock mutex
     } //给温度数组的最后一个数值写入数据

#if defined(DEBUG_MODE)
    //Serial.begin(BAUDRATE);
    Serial.printf("\nTo_artisan.BT:%f\n",To_artisan.BT);
#endif

    }

}


void setup() {

    xGetDataMutex = xSemaphoreCreateMutex();
    loopTaskWDTEnabled = true;

    pinMode(HEAT_OUT_PIN, OUTPUT); 

    Serial.begin(BAUDRATE, SERIAL_8N1, RXD, TXD);
#if defined(DEBUG_MODE)
    //Serial.begin(BAUDRATE);
    Serial.printf("\nHB_WIFI  STARTING...\n");
#endif

  //初始化网络服务
    WiFi.mode(WIFI_STA);
    WiFi.begin(user_wifi.ssid, user_wifi.password);

    byte tries = 0;
    while (WiFi.status() != WL_CONNECTED)
    {

        delay(1000);
       // Serial.println("wifi not ready");

        if (tries++ > 5)
        {
            WiFi.macAddress(macAddr); 
            // Serial_debug.println("WiFi.mode(AP):");
            WiFi.mode(WIFI_AP);
            sprintf( ap_name ,"HB_WIFI_%02X%02X%02X",macAddr[0],macAddr[1],macAddr[2]);
            WiFi.softAP(ap_name, "12345678"); // defualt IP address :192.168.4.1 password min 8 digis
            break;
        }
        // show AP's IP
    }
#if defined(DEBUG_MODE)
    Serial.printf("\nRead data from EEPROM...\n");
#endif
    // set up eeprom data
    EEPROM.begin(sizeof(user_wifi));
    EEPROM.get(0, user_wifi);

 //user_wifi.Init_mode = true ;

// if (user_wifi.Init_mode) 
// {
//     strcat(user_wifi.ssid,"HB_WIFI");
//     strcat(user_wifi.password,"12345678");
//     user_wifi.PWM_FREQ_HEAT = PWM_FREQ;
//     user_wifi.Init_mode = false ;
//     EEPROM.put(0, user_wifi);
//     EEPROM.commit();
// }
#if defined(DEBUG_MODE)
   Serial.print("HB_WIFI's IP:");
#endif


    if (WiFi.getMode() == 2) // 1:STA mode 2:AP mode
    {
        Serial.println(IpAddressToString(WiFi.softAPIP()));
        local_IP = IpAddressToString(WiFi.softAPIP());
    }
    else
    {
        Serial.println(IpAddressToString(WiFi.localIP()));
        local_IP = IpAddressToString(WiFi.localIP());
    }
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

  ElegantOTA.begin(&server);    // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

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
    mb.addHreg(INLET_HREG);
    mb.addHreg(AP_HREG);
    mb.addHreg(HEAT_HREG);

    mb.Hreg(BT_HREG,0); //初始化赋值
    mb.Hreg(ET_HREG,0);  //初始化赋值
    mb.Hreg(INLET_HREG,0); //初始化赋值
    mb.Hreg(HEAT_HREG,0);  //初始化赋值
    mb.Hreg(AP_HREG,0);//初始化赋值
}

void loop() {

 const TickType_t xIntervel = 1000/ portTICK_PERIOD_MS;
//更新寄存器数据
  mb.task();
  ElegantOTA.loop();

// pwm output level 
//    PC                                        MCU-value                   ENCODER read
//Artisan-> heat_from_Artisan        >>    To_artisan.heat_level     <<     heat_from_enc
//  heat_from_Artisan == heat_from_enc  in loop（） 


heat_from_enc = encoder.getCount();

 //Serial.printf("heat_from_enc: %d\n", heat_from_enc);

if (xSemaphoreTake(xGetDataMutex, xIntervel) == pdPASS) {  
        To_artisan.heat_level =  mb.Hreg(HEAT_HREG);//读取数据

       //HEAT 控制部分 
       if ((To_artisan.heat_level + heat_from_enc) <= 0 && To_artisan.heat_level >=0 ) { //如果输入小于0值，自动限制在0
            

            To_artisan.heat_level = 0;
            //heat_from_Artisan= To_artisan.heat_level ; 
            mb.Hreg(HEAT_HREG,To_artisan.heat_level);
            encoder.clearCount();
            //encoder.setCount(100);
            heat_from_enc=0;
            

       } else if ((To_artisan.heat_level + heat_from_enc) >= 100 && To_artisan.heat_level <= 100){//如果输入大于100值，自动限制在100

            To_artisan.heat_level = 100;
            mb.Hreg(HEAT_HREG,To_artisan.heat_level);
            //heat_from_Artisan= To_artisan.heat_level ; 
            encoder.clearCount();
            heat_from_enc=0; 


       } else { //To_artisan.heat_level 加减 encoder的增减量，再同步到

            To_artisan.heat_level = heat_from_enc + To_artisan.heat_level;
            mb.Hreg(HEAT_HREG,To_artisan.heat_level);
            //heat_from_Artisan= To_artisan.heat_level ; 
            encoder.clearCount();
            heat_from_enc=0;

           }
       xSemaphoreGive(xGetDataMutex);  //end of lock mutex
}
       pwm.write(HEAT_OUT_PIN, map(To_artisan.heat_level,0,100,250,1000), PWM_FREQ, resolution); //自动模式下，将heat数值转换后输出到pwm
 

delay(50);

}