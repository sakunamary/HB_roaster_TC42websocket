#include <Arduino.h>
#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <AsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>
#include "SoftwareSerial.h"
#include "ArduinoJson.h"

#define DEBUG_MODE
#define BAUD 115200


SoftwareSerial serial_in;// D1 RX_drumer  D2 TX_drumer 

AsyncWebServer server(80);

AsyncWebSocket ws("/websocket"); // access at ws://[esp ip]/

char ap_name[30] ;
uint8_t macAddr[6];
String MsgString;
String local_IP;

const char* ssid = "esp_serial"; // Your WiFi SSID
const char* password = "12345678"; // Your WiFi Password
user_wifi_t user_wifi = {" ", " ", false};
data_to_artisan_t To_artisan = {1.0,2.0,3.0,4.0};


void  get_data();
void notFound(AsyncWebServerRequest *request);    
void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);//Handle WebSocket event
void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){};
String IpAddressToString(const IPAddress &ipAddress);      



String IpAddressToString(const IPAddress &ipAddress)
{
    return String(ipAddress[0]) + String(".") +
           String(ipAddress[1]) + String(".") +
           String(ipAddress[2]) + String(".") +
           String(ipAddress[3]);
}




void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){

     //    {"command": "getData", "id": 93609, "roasterID": 0}
    // Artisan schickt Anfrage als TXT
    // TXT zu JSON lt. https://forum.arduino.cc/t/assistance-parsing-and-reading-json-array-payload-websockets-solved/667917

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
                    // Send Values to Artisan over Websocket
                    JsonObject root = doc.to<JsonObject>();
                    JsonObject data = root.createNestedObject("data");
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

                    else if (command == "getData")
                    {
                        root["id"] = ln_id;
                        data["BT"] = To_artisan.BT;
                        data["AP"] = To_artisan.AP;
                        data["inlet"] = To_artisan.inlet;                         
 

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


/* Message callback of WebSerial */
/*
void recvMsg(uint8_t *data, size_t len){
  WebSerial.println("Received Data...");
  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }
  WebSerial.println(d);
}
*/


void  get_data() {

//获取数据
    serial_in.print("CHAN;1300\n");
    delay(20);
    serial_in.flush();

    serial_in.print("READ\n");
    delay(20);
       while (serial_in.available()){
        MsgString = serial_in.readStringUntil('C');

    }   

    #if defined DEBUG_MODE
    Serial.println("read from drummer:");
    Serial.println(MsgString);
    #endif 

    serial_in.print("CHAN;2400\n");
    delay(20);
    serial_in.flush();

    serial_in.print("READ\n");
    delay(20);
       while (serial_in.available()){
        MsgString = serial_in.readStringUntil('C');
    }   
        serial_in.println(MsgString);


}







void setup() {

    xThermoDataMutex = xSemaphoreCreateMutex();
    Serial.begin(BAUDRATE);
    while (!Serial)
    {
        ; // wait for serial port ready
    }
   // serial_in.begin(115200,SWSERIAL_8N1,D1,D2 );  //RX D1 TX D2
    Serial.printf("\nTC4-WB  STARTING...\n");
    Serial.printf("\nRead data from EEPROM...\n");
    // set up eeprom data
    EEPROM.begin(sizeof(user_wifi));
    EEPROM.get(0, user_wifi);

 //user_wifi.Init_mode = true ;

if (user_wifi.Init_mode) 
{
    strcat(user_wifi.ssid,"HB_WIFI");
    strcat(user_wifi.password,"12345678");
    user_wifi.Init_mode = false ;
    EEPROM.put(0, user_wifi);
    EEPROM.commit();
}

Serial.printf("\nStart Task...\n");
    /*---------- Task Definition ---------------------*/
    // Setup tasks to run independently.
    xTaskCreatePinnedToCore(
        task_get_data, "get_data" // 测量电池电源数据，每分钟测量一次
        ,
        1024 // This stack size can be checked & adjusted by reading the Stack Highwater
        ,
        NULL, 1 // Priority, with 1 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
        ,
        NULL,  1 // Running Core decided by FreeRTOS,let core0 run wifi and BT
    );
    Serial.printf("\nTASK1:get_data...\n");

    xTaskCreatePinnedToCore(
        task_send_data,"send_data" // MAX6675 thermal task to read Bean-Temperature (BT)
        ,
        1024 // Stack size
        ,
        NULL, 3 // Priority
        ,
        NULL, 
        1 // Running Core decided by FreeRTOS,let core0 run wifi and BT
    );
    Serial.printf("\nTASK2:send_data...\n");



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

    server_OTA.onNotFound(notFound); // 404 page seems not necessary...
    server_OTA.onFileUpload(onUpload);

  server.begin();
    Serial.println("HTTP server started");
}

void loop() {


}