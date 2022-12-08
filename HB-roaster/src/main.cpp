
/*
主要实现目标：
1:串口(serail2)获取USB来的数据 用分别赋值到结构体
2：websotcket 转换为json格式传递给artisan
3:serial作为debug接口
4:I2C方式获取 大气压、温度、湿度（二期）
5:固件OTA方式升级
6:webserial数据输出
*/

#include <Arduino.h>

#include "config.h"
#include "index.h"

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include "WebSerial.h"
// Websockets Lib by links2004
#include <WebSocketsServer.h>
// JSON for Artisan Websocket implementation
#include "ArduinoJson.h"
#include <EEPROM.h>



#include "DFRobot_AHT20.h"
#include <Adafruit_BMP085.h>
//Ticker to execute actions at defined intervals
#include "TickTwo.h" //ESP8266 compatible version of Ticker by sstaub

//drumer 命令串字符处理分割
#include <StringTokenizer.h>



String local_IP;

//串口初始化
HardwareSerial Serial_with_drumer(1); //获取数据
HardwareSerial Serial_debug(0);       //debug

// object declare
AsyncWebServer server_OTA(80);
WebSocketsServer webSocket = WebSocketsServer(8080); //构建websockets类
user_wifi_t user_wifi = {" ", " "};

DFRobot_AHT20 aht20;//构建aht20 类
Adafruit_BMP085 bmp;//构建BMP180 类

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
String IpAddressToString(const IPAddress &ipAddress);                         //转换IP地址格式
void recvMsg(uint8_t *data, size_t len);
String processor(const String &var);
void notFound(AsyncWebServerRequest *request);  




SemaphoreHandle_t xTaskEnvDataMutex = NULL;








// Define Artisan Websocket events to exchange data
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    // Artisan schickt Anfrage als TXT
    // TXT zu JSON lt. https://forum.arduino.cc/t/assistance-parsing-and-reading-json-array-payload-websockets-solved/667917




    const size_t capacity = JSON_OBJECT_SIZE(3) + 60; // Memory pool
    DynamicJsonDocument doc(capacity);
    String temp_cmd_out = ""; // from websockets recived drumer control command and send out ;
    switch (type)
    {
    case WStype_DISCONNECTED:
        webSocket.sendTXT(num, "Disonnected");
        Serial.printf("[%u] Disconnected!\n", num);
        break;
    case WStype_CONNECTED:
    {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        webSocket.sendTXT(num, "Connected");
    }
    break;
    case WStype_TEXT:
    {
        // DEBUG WEBSOCKET
        Serial.printf("[%u] get Text: %s\n", num, payload);

        // Extract Values lt. https://arduinojson.org/v6/example/http-client/
        // Artisan Anleitung: https://artisan-scope.org/devices/websockets/

        deserializeJson(doc, (char *)payload);

        // char* entspricht String
        String command = doc["command"].as<char *>();
        WebSerial.print("websocket get msg : ");
        WebSerial.println(command);

        // Serial_debug.printf("Command received: %s \n",command);

        long ln_id = doc["id"].as<long>();

        // Send Values to Artisan over Websocket
        JsonObject root = doc.to<JsonObject>();
        JsonObject data = root.createNestedObject("data");
        if (command == "getBT")
        {
            root["id"] = ln_id;
            data["BT"] = To_artisan.bt;
        }
        else if (command == "getET")
        {
            root["id"] = ln_id;
            data["ET"] = To_artisan.et;
        }
        else if (command == "getExhaust")
        {
            root["id"] = ln_id;
            data["Exhaust"] = To_artisan.Exhaust;
            // Serial_debug.printf("getET created ET: %4.2f \n",cmd_M1.TC2);
        }
        else if (command == "getInlet")
        {
            root["id"] = ln_id;
            data["Inlet"] = To_artisan.Inlet;
            // Serial_debug.printf("getET created ET: %4.2f \n",cmd_M1.TC2);
        }
        else if (command == "getAT")
        {
            root["id"] = ln_id;
            data["AT"] = To_artisan.AT;
            // Serial_debug.printf("getET created ET: %4.2f \n",cmd_M1.TC2);
        }
        else if (command == "getData")
        {
            root["id"] = ln_id;
            data["BT"] = To_artisan.bt;
            data["ET"] = To_artisan.et;
            

            Serial.println("getData");
        }

        char buffer[200];                        // create temp buffer 200
        size_t len = serializeJson(doc, buffer); // serialize to buffer

        webSocket.sendTXT(num, buffer);
        WebSerial.print("websocket send back: ");
        WebSerial.println(buffer);

        // send message to client
        // webSocket.sendTXT(num, "message here");

        // send data to all connected clients
        // webSocket.broadcastTXT("message here");
    }
    break;
    case WStype_BIN:
        // Serial_debug.printf("[%u] get binary length: %u\n", num, length);
        // hexdump(payload, length);

        // send message to client
        webSocket.sendBIN(num, payload, length);
        break;
    }
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
    //Serial.println(var);
if (var == "version")
    {
        return VERSION;
    }
    return String();
}







/* Message callback of WebSerial */
void recvMsg(uint8_t *data, size_t len){
  WebSerial.println("Received Data...");
  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }
  WebSerial.println(d);
}

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Opps....Not found");
}





void setup() {
  // put your setup code here, to run once:
   xThermoDataMutex = xSemaphoreCreateMutex();


 Serial_debug.begin(BAUDRATE);
 Serial_with_drumer.begin(BAUDRATE);
 Serial_debug.printf("\nHB Roaster is  STARTING...\n");
 WebSerial.begin(&server_OTA);
 WebSerial.msgCallback(recvMsg);


   // 读取EEPROM 数据
    EEPROM.begin(sizeof(user_wifi));
    EEPROM.get(0, user_wifi);


// Setup tasks to run independently.
    xTaskCreatePinnedToCore(
        TaskEnvData, "env_data" // 测量电池电源数据，每分钟测量一次
        ,
        1024*4 // This stack size can be checked & adjusted by reading the Stack Highwater
        ,
        NULL, 1 // Priority, with 1 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
        ,
        NULL,  1 // Running Core decided by FreeRTOS,let core0 run wifi and BT
    );





//初始化wifi 
   WiFi.mode(WIFI_STA);
    WiFi.begin(user_wifi.ssid, user_wifi.password);

    byte tries = 0;
    while (WiFi.status() != WL_CONNECTED)
    {

        delay(1000);

        if (tries++ > 5)
        {

            // Serial_debug.println("WiFi.mode(AP):");
            WiFi.mode(WIFI_AP);
            WiFi.softAP("HB_ROASTER", "12345678"); // defualt IP address :192.168.4.1 password min 8 digis
            break;
        }
        // show AP's IP
    }
    Serial_debug.print("TC4-WB's IP:");
    WebSerial.print("TC4-WB's IP:");

    if (WiFi.getMode() == 2) // 1:STA mode 2:AP mode
    {
        Serial_debug.println(IpAddressToString(WiFi.softAPIP()));
        WebSerial.println(IpAddressToString(WiFi.softAPIP()));
        local_IP = IpAddressToString(WiFi.softAPIP());

    }
    else
    {
        Serial_debug.println(IpAddressToString(WiFi.localIP()));
        WebSerial.println(IpAddressToString(WiFi.localIP()));
        local_IP = IpAddressToString(WiFi.localIP());
    }


   // init websocket
    webSocket.begin();
    // event  websocket handler
    webSocket.onEvent(webSocketEvent);

    Serial_debug.println("WebSocket started!");
    WebSerial.println("WebSocket started!");


    // 网页处理
    server_OTA.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                  { request->send_P(200, "text/html", index_html, processor); });

    // get the value from index.html
    server_OTA.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
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
                  
    server_OTA.onNotFound(notFound); // 404 page seems not necessary...



    AsyncElegantOTA.begin(&server_OTA); // Start ElegantOTA

    server_OTA.begin();
   // WebSerial.println("HTTP server started");
    Serial_debug.println("HTTP server started");



}

  




void loop() {
  // put your main code here, to run repeatedly:

           webSocket.loop(); //处理websocketmie





}