
/*
主要实现目标：
1:串口(serail2)获取USB来的数据 用分别赋值到结构体
2：websotcket 转换为json格式传递给artisan
3:serial作为debug接口
4:I2C方式获取 大气压、温度、湿度（二期）
5:固件OTA方式升级
6:webserial数据输出
*/

//https://www.hobbytronics.co.uk/usb-host-serial


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
//#include "TickTwo.h" //ESP8266 compatible version of Ticker by sstaub

//drumer 命令串字符处理分割
#include <StringTokenizer.h>

#include "Task_env_data.h"


String local_IP;

//串口初始化
HardwareSerial Serial_with_drumer(2); //获取数据
//HardwareSerial Serial_debug(0);       //debug

// object declare
AsyncWebServer server_OTA(80);
WebSocketsServer webSocket = WebSocketsServer(8080); //构建websockets类
user_wifi_t user_wifi = {" ", " "};


void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
String IpAddressToString(const IPAddress &ipAddress);                         //转换IP地址格式
void recvMsg(uint8_t *data, size_t len);
String processor(const String &var);
void notFound(AsyncWebServerRequest *request);  
void ReadData_from_drumer(void);//读取 锅炉的数值 指令  750ms 运行一次 。剥离数据后写入cmd_M1 和 To_artisan  

char serial_buffer[64];


// Define Artisan Websocket events to exchange data
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    // Artisan schickt Anfrage als TXT
    // TXT zu JSON lt. https://forum.arduino.cc/t/assistance-parsing-and-reading-json-array-payload-websockets-solved/667917

    ReadData_from_drumer();//send the READ and get data 

    const size_t capacity = JSON_OBJECT_SIZE(3) + 60; // Memory pool
    DynamicJsonDocument doc(capacity);
    String temp_cmd_out = ""; // from websockets recived drumer control command and send out ;
    switch (type)
    {
    case WStype_DISCONNECTED:
        webSocket.sendTXT(num, "Disonnected");
        Serial_debug.printf("[%u] Disconnected!\n", num);
        break;
    case WStype_CONNECTED:
    {
        IPAddress ip = webSocket.remoteIP(num);
        Serial_debug.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        webSocket.sendTXT(num, "Connected");
    }
    break;
    case WStype_TEXT:
    {
        // DEBUG WEBSOCKET
        Serial_debug.printf("[%u] get Text: %s\n", num, payload);

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
            
        }
        else if (command == "getInlet")
        {
            root["id"] = ln_id;
            data["Inlet"] = To_artisan.Inlet;

        }
        else if (command == "getAT")
        {
            root["id"] = ln_id;
            data["AT"] = To_artisan.AT;
            // Serial_debug.printf("getET created ET: %4.2f \n",cmd_M1.TC2);
        }
        else if (command == "getTemp")
        {
            root["id"] = ln_id;
            data["AT"] = To_artisan.temp_env;
            // Serial_debug.printf("getET created ET: %4.2f \n",cmd_M1.TC2);
        }
        else if (command == "getHumi")
        {
            root["id"] = ln_id;
            data["AT"] = To_artisan.humi_env;
            // Serial_debug.printf("getET created ET: %4.2f \n",cmd_M1.TC2);
        }
        else if (command == "getAmp")
        {
            root["id"] = ln_id;
            data["AT"] = To_artisan.amp_env;
            // Serial_debug.printf("getET created ET: %4.2f \n",cmd_M1.TC2);
        }
        else if (command == "getData")
        {
            root["id"] = ln_id;
            data["BT"] = To_artisan.bt;
            data["ET"] = To_artisan.et;
            data["Inlet"] = To_artisan.Inlet;
            data["AT"] = To_artisan.AT;
        }

        char buffer[200];                        // create temp buffer 200
        size_t len = serializeJson(doc, buffer); // serialize to buffer

        webSocket.sendTXT(num, buffer);
        WebSerial.print("websocket send back: ");
        WebSerial.println(buffer);

        Serial_debug.print("websocket send back: ");
        Serial_debug.println(buffer);

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



void ReadData_from_drumer(void) {
String msg_raw;
//String cmd = "";
//int loop_i=1; 
/*
Serial_with_drumer.print("CHAN,1300\r\n");
WebSerial.println("ReadData from drumer () sended CHAN;1300 command");
WebSerial.println(Serial_with_drumer.available());

//读取串口数据
if  (Serial_with_drumer.available()>0)
{
    msg_raw = Serial_with_drumer.readStringUntil('\n'); //读取数据


        Serial_debug.println("read from drummer raw :");
        Serial_debug.println(msg_raw); 
        WebSerial.println("read from drummer raw :");
        WebSerial.println(msg_raw); 

}
*/
Serial_with_drumer.print('READ\n');
WebSerial.println("ReadData from drumer () sended READ command");
//Serial_with_drumer.flush();

delay(150);
WebSerial.println(Serial_with_drumer.available());
//读取串口数据


if  (Serial_with_drumer.available()>0)
{
     /*
for(int i=0; i < Serial_with_drumer.available(); i++){
    cmd += char(serial_buffer[i]);
  }
*/
    msg_raw = Serial_with_drumer.readStringUntil('\n'); //读取数据
    //mag_raw = Serial_with_drumer.read();

        WebSerial.println("read from drummer raw :");
        WebSerial.println(msg_raw); 


/*
  StringTokenizer tokens_1300(msg_raw, ",");
  while(tokens_1300.hasNext()){
          if (loop_i == 1) { To_artisan.AT = tokens_1300.nextToken().toDouble();      loop_i++;}
          else if (loop_i == 2) {To_artisan.et = tokens_1300.nextToken().toDouble();  loop_i++;}
          else if (loop_i == 3) {To_artisan.bt = tokens_1300.nextToken().toDouble();  loop_i++;}
          else if (loop_i == 4) { loop_i = 1 ;}
  
*/
    }
}  //完成一次读取和处理数据

/* Message callback of WebSerial */
void recvMsg(uint8_t *data, size_t len){
  WebSerial.println("Received Data...");
  String cmd = "";
  int loop_i=1;   
  for(int i=0; i < len; i++){
    cmd += char(data[i]);
  }
cmd.toUpperCase();

  WebSerial.print("msg from webserial:");
  WebSerial.println(cmd);
  Serial_debug.print("msg from webserial:");
  Serial_debug.println(cmd);

if (cmd.indexOf("READ") == 0){
    ReadData_from_drumer();
    WebSerial.print("BT:");
    WebSerial.println(To_artisan.bt); 
    WebSerial.print("ET:");
    WebSerial.println(To_artisan.et); 
    WebSerial.print("Exhaust:");
    WebSerial.println(To_artisan.Exhaust); 
    WebSerial.print("Inlet:");
    WebSerial.println(To_artisan.Inlet); 
    WebSerial.print("AT:");
    WebSerial.println(To_artisan.AT); 

}
if (cmd.indexOf("INFO") == 0)
{
    WebSerial.print("HB Roaster :");
    WebSerial.println(VERSION); 
    WebSerial.print("websocket IP:(port:8080)");
    WebSerial.println(local_IP);
    WebSerial.print("Serial baudrate:");
    WebSerial.println(BAUDRATE); 
}

/*
  StringTokenizer tokens(d, ",");
  while(tokens.hasNext()){
          if (loop_i == 1) { To_artisan.bt = tokens.nextToken().toDouble();      loop_i++;}
          else if (loop_i == 2) {To_artisan.et = tokens.nextToken().toDouble();  loop_i++;}
          else if (loop_i == 3) {To_artisan.Exhaust = tokens.nextToken().toDouble();  loop_i++;}
          else if (loop_i == 4) {To_artisan.Inlet = tokens.nextToken().toDouble();  loop_i++;}
          else if (loop_i == 5) {To_artisan.AT = tokens.nextToken().toDouble();  loop_i++;}
          else if (loop_i == 6) {To_artisan.Null_data = tokens.nextToken().toDouble(); loop_i = 1 ; }
   }
*/  
    

}

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Opps....Not found");
}





void setup() {
  // put your setup code here, to run once:
   xTaskEnvDataMutex = xSemaphoreCreateMutex();


 Serial_debug.begin(BAUDRATE);

// ESP32 UART2  RX =GPIO16 	TX=GPIO17
 //void HardwareSerial::begin(unsigned long baud, uint32_t config=SERIAL_8N1, int8_t rxPin=-1, int8_t txPin=-1, bool invert=false, unsigned long timeout_ms = 20000UL);
  Serial_with_drumer.begin(BAUDRATE);
 
 //Serial_with_drumer.begin(BAUDRATE,SERIAL_8N1,16,17,false);
 //Serial_with_drumer.setRxTimeout(30);

 Serial_debug.printf("\nHB Roaster is  STARTING...\n");


 WebSerial.begin(&server_OTA);
 WebSerial.msgCallback(recvMsg);
 Serial_debug.printf("\nHB Roaster webserial started\n");

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
    Serial_debug.print("HB_ROASTER's IP:");


    if (WiFi.getMode() == 2) // 1:STA mode 2:AP mode
    {
        Serial_debug.println(IpAddressToString(WiFi.softAPIP()));
        local_IP = IpAddressToString(WiFi.softAPIP());

    }
    else
    {
        Serial_debug.println(IpAddressToString(WiFi.localIP()));
        local_IP = IpAddressToString(WiFi.localIP());
    }


   // init websocket
    webSocket.begin();
    // event  websocket handler
    webSocket.onEvent(webSocketEvent);

    Serial_debug.println("WebSocket started!");

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