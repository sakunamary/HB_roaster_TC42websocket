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

const char* ssid = "esp_serial"; // Your WiFi SSID
const char* password = "12345678"; // Your WiFi Password

data_to_artisan_t To_artisan = {1.0,2.0,3.0,4.0};


void  get_data();

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);//Handle WebSocket event

void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){};

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


WiFi.macAddress(macAddr); 
// Serial_debug.println("WiFi.mode(AP):");
WiFi.mode(WIFI_AP);
sprintf( ap_name ,"Serial_%02X%02X%02X",macAddr[0],macAddr[1],macAddr[2]);
WiFi.softAP(ap_name, "12345678"); // defualt IP address :192.168.4.1 password min 8 digis


  //WebSerial.begin(&server);
Serial.printf("\nStart Task...\n");
    /*---------- Task Definition ---------------------*/
    // Setup tasks to run independently.
    xTaskCreatePinnedToCore(
        TaskBatCheck, "bat_check" // 测量电池电源数据，每分钟测量一次
        ,
        1024 // This stack size can be checked & adjusted by reading the Stack Highwater
        ,
        NULL, 1 // Priority, with 1 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
        ,
        NULL,  1 // Running Core decided by FreeRTOS,let core0 run wifi and BT
    );
    Serial.printf("\nbat_check...\n");

    xTaskCreatePinnedToCore(
        TaskThermalMeter, "ThermalMeter" // MAX6675 thermal task to read Bean-Temperature (BT)
        ,
        1024 // Stack size
        ,
        NULL, 3 // Priority
        ,
        NULL, 
        1 // Running Core decided by FreeRTOS,let core0 run wifi and BT
    );
    Serial.printf("\nThermalMeter...\n");


  server.begin();

}

void loop() {


}