

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
#include <StringTokenizer.h>



//串口初始化
HardwareSerial serial_with_drumer(1); //获取数据
HardwareSerial Serial_debug(0);       //debug




void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);







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
            data["BT"] = BT_AvgTemp;
            // Serial_debug.printf("getBT created BT: %4.2f \n",cmd_M1.TC1);
        }
        else if (command == "getET")
        {
            root["id"] = ln_id;
            data["ET"] = ET_CurTemp;
            // Serial_debug.printf("getET created ET: %4.2f \n",cmd_M1.TC2);
        }

        else if (command == "getData")
        {
            root["id"] = ln_id;
            data["BT"] = BT_AvgTemp;
            data["ET"] = ET_CurTemp;

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










void setup() {
  // put your setup code here, to run once:
}






void loop() {
  // put your main code here, to run repeatedly:
}