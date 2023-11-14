#include <Arduino.h>
#include "config.h"
#include "EEPROM.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <AsyncUDP.h>
#include <ESPAsyncWebServer.h>
#include "Update.h"


#include <HardwareSerial.h>

#include <StringTokenizer.h>
//#include <WebSerial.h>
//

#include "ArduinoJson.h"

#include <pwmWrite.h>
#include <ESP32Encoder.h>




//SoftwareSerial Serial_in ;
//spSoftwareSerial::UART Serial_in;// D10 RX_drumer  D9 TX_drumer 
 HardwareSerial Serial_in(2);
SemaphoreHandle_t xThermoDataMutex = NULL;

AsyncWebServer server(80);

AsyncWebSocket ws("/websocket"); // access at ws://[esp ip]/

char ap_name[30] ;
uint8_t macAddr[6];

String local_IP;
String MsgString;

String MSG_token1300[4];
String MSG_token2400[4];
long timestamp;
uint16_t  heat_from_Artisan = 0;
uint16_t  heat_from_enc  = 0;

user_wifi_t user_wifi = {
                        " ", //char ssid[60]; //增加到30个字符
                        " ", //char password[60]; //增加到30个字符
                        PWM_FREQ,//int PWM_FREQ_HEAT;
                        false //bool   Init_mode ; //是否初始化模式
                        };

data_to_artisan_t To_artisan = {1.0,2.0,3.0,4.0,0};


// pwm output level 
//    PC                                        MCU-value                   ENCODER read
//Artisan-> heat_from_Artisan        >>    To_artisan.heat_level     <<     heat_from_enc
//  heat_from_Artisan == heat_from_enc  in loop（） 

const uint32_t frequency = PWM_FREQ;
const byte resolution = PWM_RESOLUTION; //pwm -0-4096

//Coil Pins
const int HEAT_OUT_PIN = PWM_HEAT; //GPIO26


int encoder_postion ;


//pwm object 
Pwm pwm = Pwm();

// rotary encoder object
ESP32Encoder encoder;

TaskHandle_t xHandle_indicator;

void notFound(AsyncWebServerRequest *request);    
void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);//Handle WebSocket event
void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){}
String IpAddressToString(const IPAddress &ipAddress);      



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

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){

     //    {"command": "getData", "id": 93609, "roasterID": 0}
    // Artisan schickt Anfrage als TXT
    // TXT zu JSON lt. https://forum.arduino.cc/t/assistance-parsing-and-reading-json-array-payload-websockets-solved/667917

    TickType_t xLastWakeTime;

    const TickType_t xIntervel = 1000/ portTICK_PERIOD_MS;

    const size_t capacity = JSON_OBJECT_SIZE(3) + 60; // Memory pool
    DynamicJsonDocument doc(capacity);

    switch (type)
    {
    case WS_EVT_DISCONNECT:
        Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
        break;
    case WS_EVT_CONNECT:
        //client connected
         Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
         client->printf("Hello Client %u :", client->id());
         client->ping();
        break;
    case WS_EVT_ERROR:
        //error was received from the other end
         Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
        break;
    case WS_EVT_PONG:
        //pong message was received (in response to a ping request maybe)
        Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");   
        break;   
    case WS_EVT_DATA:
        AwsFrameInfo * info = (AwsFrameInfo*)arg;
       if(info->final && info->index == 0 && info->len == len){

         Serial.printf("ws[%s][%u] %s-message[%llu]: ",server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

            if(info->opcode == WS_TEXT){
                    // Extract Values lt. https://arduinojson.org/v6/example/http-client/
                    // Artisan Anleitung: https://artisan-scope.org/devices/websockets/
                    deserializeJson(doc, (char *)data);
                    // char* entspricht String
                    String command = doc["command"].as<  const char *>();
                    // Serial_debug.printf("Command received: %s \n",command);
                    long ln_id = doc["id"].as<long>();

                  if(!doc["HeatVal"].isNull()) //发送Heat level
                    {
                      heat_from_Artisan = doc["HeatVal"].as<long>();

                        if (heat_from_Artisan >0 && heat_from_Artisan < 101
                           &&  (xSemaphoreTake(xThermoDataMutex, xIntervel) == pdPASS) )
                           {//给温度数组的最后一个数值写入数据   ){// 过滤TargetC -1 和 大于100 值。
              
                            To_artisan.heat_level=heat_from_Artisan;

                            xSemaphoreGive(xThermoDataMutex);  //end of lock mutex
                        }    
            }  



                    // Send Values to Artisan over Websocket
                    JsonObject root = doc.to<JsonObject>();
                    JsonObject data = root.createNestedObject("data");

                if (xSemaphoreTake(xThermoDataMutex, xIntervel) == pdPASS) {//给温度数组的最后一个数值写入数据   

                    if (command == "getBT")
                    {
                        root["id"] = ln_id;
                        data["BT"] = To_artisan.BT;
                    }
                    else if (command == "getET")
                    {
                        root["id"] = ln_id;
                        data["ET"] = To_artisan.ET;
                    }
                    else if (command == "getInlet")
                    {
                        root["id"] = ln_id;
                        data["Inlet"] = To_artisan.inlet;
                    }
                    else if (command == "getAP")
                    {
                        root["id"] = ln_id;
                        data["AP"] = To_artisan.AP;
                    }
                    else if (command == "getHEAT")
                    {
                        root["id"] = ln_id;
                        data["HEAT"] = To_artisan.heat_level;
                    }
                    else if (command == "getData")
                    {
                        root["id"] = ln_id;
                        data["BT"] = To_artisan.BT;
                        data["ET"] = To_artisan.ET;
                        data["AP"] = To_artisan.AP;
                        data["Inlet"] = To_artisan.inlet;     
                        data["HEAT"] = To_artisan.heat_level;                   
                    }

                    xSemaphoreGive(xThermoDataMutex);  //end of lock mutex
                } 


                    char buffer[200];                        // create temp buffer 200
                    size_t len = serializeJson(doc, buffer); // serialize to buffer

                    Serial.println(buffer);
                    client->text(buffer);
                }
            }   
    break;
    }
}



void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Opps....Not found");
}



void task_get_data(void *pvParameters)
{ //function 

    /* Variable Definition */
    (void)pvParameters;
    TickType_t xLastWakeTime;

    const TickType_t xIntervel = 1000/ portTICK_PERIOD_MS;


   //const TickType_t xIntervel = (2 * 1000) / portTICK_PERIOD_MS;
    /* Task Setup and Initialize */
    // Initial the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();
    int i = 0;
    for (;;) // A Task shall never return or exit.
    { //for loop
        // Wait for the next cycle (intervel 750ms).

         StringTokenizer tokens(MsgString, ",");
        //获取数据
            Serial_in.print("CHAN;1300\n");
            delay(20);
            Serial_in.flush();

            Serial_in.print("READ\n");
            delay(20);
            if(Serial_in.available()>0){
                MsgString = Serial_in.readStringUntil('C');
                MsgString.concat('C');
            } 
/*
            Serial.println("read from drummer:");
            Serial.println(MsgString);
*/


            while(tokens.hasNext()){
                   MSG_token1300[i]=tokens.nextToken(); // prints the next token in the string
                  // Serial.println(MSG_token[i]);
                   i++;
                }
            if (xSemaphoreTake(xThermoDataMutex, xIntervel) == pdPASS)  //给温度数组的最后一个数值写入数据

                {//lock the  mutex    
                    To_artisan.BT = MSG_token1300[1].toFloat();
                    To_artisan.ET = MSG_token1300[2].toFloat();

                        xSemaphoreGive(xThermoDataMutex);  //end of lock mutex
                }   
                
            MsgString = "";
            i=0;

            Serial_in.print("CHAN;2400\n");
            delay(20);
            Serial_in.flush();

            Serial_in.print("READ\n");
            delay(20);
            if(Serial_in.available()>0){
                MsgString = Serial_in.readStringUntil('C');
                MsgString.concat('C');
            }   


            while(tokens.hasNext()){
                   MSG_token2400[i]=tokens.nextToken(); // prints the next token in the string
                  // Serial.println(MSG_token[i]);
                   i++;
                }
            if (xSemaphoreTake(xThermoDataMutex, xIntervel) == pdPASS)  //给温度数组的最后一个数值写入数据
                {//lock the  mutex       
                    To_artisan.inlet = MSG_token2400[1].toFloat() ;


                        xSemaphoreGive(xThermoDataMutex);  //end of lock mutex
                }   
                
            MsgString = "";
            i=0;   





                vTaskDelayUntil(&xLastWakeTime, xIntervel);

    }
}//function 






void setup() {

    xThermoDataMutex = xSemaphoreCreateMutex();

    pinMode(HEAT_OUT_PIN, OUTPUT); 

  //初始化网络服务
    WiFi.mode(WIFI_STA);
    WiFi.begin(user_wifi.ssid, user_wifi.password);

    byte tries = 0;
    while (WiFi.status() != WL_CONNECTED)
    {

        delay(1000);
        Serial.println("wifi not ready");

        if (tries++ > 7)
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


    Serial.begin(BAUDRATE);
    //Serial_in.begin(BAUDRATE,EspSoftwareSerial::SWSERIAL_8N1,10,9); //RX  TX
    Serial_in.begin(BAUDRATE, SERIAL_8N1, RXD, TXD);



    while (!Serial)
    {
        ; // wait for serial port ready
    }

    Serial.printf("\nHB_WIFI  STARTING...\n");
    Serial.printf("\nSerial_in setup OK\n");
    Serial.printf("\nRead data from EEPROM...\n");
    // set up eeprom data
    EEPROM.begin(sizeof(user_wifi));
    EEPROM.get(0, user_wifi);

 //user_wifi.Init_mode = true ;

if (user_wifi.Init_mode) 
{
    strcat(user_wifi.ssid,"HB_WIFI");
    strcat(user_wifi.password,"12345678");
    user_wifi.PWM_FREQ_HEAT = PWM_FREQ;
    user_wifi.Init_mode = false ;
    EEPROM.put(0, user_wifi);
    EEPROM.commit();
}

    Serial.print("HB_WIFI's IP:");

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

Serial.printf("\nStart Task...\n");
    /*---------- Task Definition ---------------------*/
    // Setup tasks to run independently.
    xTaskCreatePinnedToCore(
        task_get_data, "get_data" // 获取HB数据
        ,
        1024 // This stack size can be checked & adjusted by reading the Stack Highwater
        ,
        NULL, 1 // Priority, with 1 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
        ,
        NULL,  1 // Running Core decided by FreeRTOS,let core0 run wifi and BT
    );
    Serial.printf("\nTASK1:get_data...\n");








    // init websocket
    Serial.println("WebSocket started!");
    // attach AsyncWebSocket
    ws.onEvent(onEvent);
    server.addHandler(&ws);



    // for index.html
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                  { request->send_P(200, "text/html", index_html, processor); });

    // get the value from index.html
    server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
//get value form webpage      
    strncpy(user_wifi.ssid,request->getParam("ssid")->value().c_str(), sizeof(user_wifi.ssid) );
    strncpy(user_wifi.password,request->getParam("password")->value().c_str(), sizeof(user_wifi.password) );
    user_wifi.ssid[request->getParam("ssid")->value().length()] = user_wifi.password[request->getParam("password")->value().length()] = '\0';  
//Svae EEPROM 
    EEPROM.put(0, user_wifi);
    EEPROM.commit();
//output wifi_sussce html;
    request->send_P(200, "text/html", wifi_sussce_html); });


  // upload a file to /upload
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200);
  }, onUpload);
       // Simple Firmware Update Form
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
                    {
                    request->send(200, "text/html", update_html);
                    });

  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
                        //shouldReboot = !Update.hasError();
                        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (Update.hasError())?update_fail_html:update_OK_html);
                        response->addHeader("Connection", "close");
                        request->send(response);
                        },[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
                        if(!index){
                        vTaskSuspend(xHandle_indicator); //停止显示
                        Serial.printf("Update Start: %s\n", filename.c_str());

                        if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)){
                            Update.printError(Serial);
                        }
                        }
                        if(!Update.hasError()){
                        if(Update.write(data, len) != len){
                            Update.printError(Serial);
                        }
                        }
                        if(final){
                        if(Update.end(true)){
                            Serial.printf("Update Success: %uB\n", index+len);
                            Serial.printf("ESP32 will reboot after 3s \n");
                            vTaskDelay(3000);
                            ESP.restart();

                        } else {
                            Update.printError(Serial);
                            Serial.printf("ESP32 will reboot after 3s \n");
                            vTaskDelay(3000);
                            ESP.restart();
                        }
                        }
  });         

    server.onNotFound(notFound); // 404 page seems not necessary...
    server.onFileUpload(onUpload);

  server.begin();
  Serial.println("HTTP server started");

  //Init pwm output
    pwm.pause();
    pwm.write(HEAT_OUT_PIN, 0, user_wifi.PWM_FREQ_HEAT, resolution);
  
    pwm.resume();
    pwm.printDebug();

    Serial.println("PWM started");  
   // analogReadResolution(10); //0-1024


    Serial.printf("\nStart INPUT ENCODER  service...\n");

//init ENCODER
  encoder.attachHalfQuad( ENC_CLK,ENC_DT);
  encoder.setCount ( 0 );
  Serial.println("Encoder started");  

timestamp=millis();

}

void loop() {

   if (millis() > timestamp + 100) {
       timestamp = millis();
       //HEAT 控制部分 
       if (heat_from_Artisan <= 0 ) { //如果输入小于0值，自动限制在0
            heat_from_Artisan = 0;

       } else if (heat_from_Artisan>= 100 ){//如果输入大于100值，自动限制在100
            heat_from_Artisan = 100;

       } else {
            heat_from_Hreg = mb.Hreg(HEAT_HREG); //自动模式下，从寄存器获取heat的数值
            encoder.setCount(heat_from_Hreg); //同步encoder的步进数。
       }

       heat_from_enc = heat_from_Artisan ; //自动模式下，同步数据到encoder
   }


       pwm.write(HEAT_OUT_PIN, map(heat_from_Hreg,0,100,0,4096), user_wifi.PWM_FREQ_HEAT, resolution); //自动模式下，将heat数值转换后输出到pwm
 

}