#include <Arduino.h>
#include "config.h"
//Wifi libs 
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>


#include "EEPROM.h"



#include <HardwareSerial.h>
//#include <SoftwareSerial.h>
#include <StringTokenizer.h>


#include "ArduinoJson.h"
//Websockets Lib by links2004
#include <WebSocketsServer.h>
//JSON for Artisan Websocket implementation
#include "ArduinoJson.h"

//Ticker to execute actions at defined intervals
#include "TickTwo.h" //ESP8266 compatible version of Ticker by sstaub


//串口初始化

//SoftwareSerial Serial_in;// D0 IO16 RX_drumer  D5 IO14 TX_drumer


ESP8266WebServer    server(80); //构建webserver类
WebSocketsServer webSocket = WebSocketsServer(8080); //构建websockets类

char ap_name[30] ;
uint8_t macAddr[6];

String MsgString;
String local_IP;
String MSG_token1300[4];
String MSG_token2400[4];


user_wifi_t user_wifi = {" ", " ", false};
data_to_artisan_t To_artisan = {1.0,2.0,3.0,4.0};


//functions declear for PlatfromIO rules
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) ;
void handlePortal();//处理设置网页处理模块
void get_data();//获取锅炉串口信息。


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





//Define Artisan Websocket events to exchange data
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * data, size_t len) {
//    {"command": "getData", "id": 93609, "roasterID": 0}
  //Artisan schickt Anfrage als TXT
  //TXT zu JSON lt. https://forum.arduino.cc/t/assistance-parsing-and-reading-json-array-payload-websockets-solved/667917

    const size_t capacity = JSON_OBJECT_SIZE(3) + 60; // Memory pool
    DynamicJsonDocument doc(capacity);

    switch(type) {
/*
    WStype_ERROR,
    WStype_DISCONNECTED, ok
    WStype_CONNECTED, ok
    WStype_TEXT, ok
    WStype_BIN, ok
    WStype_FRAGMENT_TEXT_START,
    WStype_FRAGMENT_BIN_START,
    WStype_FRAGMENT,
    WStype_FRAGMENT_FIN,
    WStype_PING,
    WStype_PONG,
*/

        case WStype_DISCONNECTED:
            //Serial_debug.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], data);
        
                // send message to client
                webSocket.sendTXT(num, "Connected");
            }
            break;
        case WStype_TEXT:
            { 
            //DEBUG WEBSOCKET
            //Serial.printf("[%u] get Text: %s\n", num, payload);

            //Extract Values lt. https://arduinojson.org/v6/example/http-client/
            //Artisan Anleitung: https://artisan-scope.org/devices/websockets/

            deserializeJson(doc, (char *)data);

            //char* entspricht String
            String command = doc["command"].as< const  char*>();
             //Serial_debug.printf("Command received: %s \n",command);  
            
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
                data["ET"] = To_artisan.ET;
                data["AP"] = To_artisan.AP;
                data["inlet"] = To_artisan.inlet;                         
            }

    

            char buffer[200];                        // create temp buffer 200
            size_t len = serializeJson(doc, buffer); // serialize to buffer

             webSocket.sendTXT(num, buffer);

                }
            break;
        case WStype_BIN:
           // Serial_debug.printf("[%u] get binary length: %u\n", num, length);
            hexdump(data, len);

            // send message to client
            // webSocket.sendBIN(num, payload, length);
            break;
        case WStype_PING:
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] PING from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], data);
            break;
            
    }
}




void task_get_data()
{ //function 
    int i = 0;
        // Wait for the next cycle (intervel 750ms).123
        //获取数据
            Serial.print("CHAN;1300\n");
            delay(20);
            Serial.flush();

            Serial.print("READ\n");
            delay(50);
            if(Serial.available()>0){
                MsgString = Serial.readStringUntil('C');
                MsgString.concat('C');
            } 

            //Serial.println("read from drummer:");
            //Serial.println(MsgString);


         StringTokenizer tokens(MsgString, ",");
            while(tokens.hasNext()){
                   MSG_token1300[i]=tokens.nextToken(); // prints the next token in the string
                   //Serial.println(MSG_token1300[i]);
                   i++;
                }
           
  
                    To_artisan.BT = MSG_token1300[1].toDouble();
                    To_artisan.ET = MSG_token1300[2].toDouble();

                    Serial.printf("\nBT:%f,ET:%f",To_artisan.BT,To_artisan.ET);
                
            MsgString = "";
            i=0;

            Serial.print("CHAN;2400\n");
            delay(20);
            Serial.flush();

            Serial.print("READ\n");
            delay(50);
            if(Serial.available()>0){
                MsgString = Serial.readStringUntil('C');
                MsgString.concat('C');
            }   


            while(tokens.hasNext()){
                   MSG_token2400[i]=tokens.nextToken(); // prints the next token in the string
                   Serial.println(MSG_token2400[i]);
                   i++;
                }
    
                    To_artisan.inlet = MSG_token2400[1].toDouble() ; 
                    Serial.printf("\ninlet:%f",To_artisan.inlet);
                
            MsgString = "";
            i=0;   

}//function 



void handlePortal() {

  if (server.method() == HTTP_POST) {

    strncpy(user_wifi.ssid,     server.arg("ssid").c_str(),     sizeof(user_wifi.ssid) );
    strncpy(user_wifi.password, server.arg("password").c_str(), sizeof(user_wifi.password) );

    user_wifi.ssid[server.arg("ssid").length()] = user_wifi.password[server.arg("password").length()] = '\0';

    EEPROM.put(0, user_wifi);
    EEPROM.commit();

    server.send(200,   "text/html",  "<!doctype html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title><style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1,p{text-align: center}</style> </head> <body><main class='form-signin'> <h1>Wifi Setup</h1> <br/> <p>Your settings have been saved successfully!<br />Please restart the device.</p></main></body></html>" );
  } else {

    server.send(200,   "text/html", "<!doctype html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title> <style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{cursor: pointer;border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1{text-align: center}</style> </head> <body><main class='form-signin'> <form action='/' method='post'><h1 class=''>Wifi Setup</h1><br/><div class='form-floating'><label>SSID</label><input type='text' class='form-control' name='ssid'> </div><div class='form-floating'><br/><label>Password</label><input type='password' class='form-control' name='password'></div><br/><div class='form-floating'><label>PROCODE</label><input type='text' class='form-control' name='drum_prodc'></div><br/><br/><button type='submit'>Save</button><p style='text-align: right'><a href='https://www.mrdiy.ca' style='color: #32C5FF'>mrdiy.ca</a></p></form></main> </body></html>" );
  }
}

TickTwo ticker_1s(task_get_data, 1000, 0, MILLIS); 


void setup() {

Serial.begin(BAUDRATE);
//Serial_in.begin(BAUDRATE, SWSERIAL_8N1, TX, RX, false, 256); 
   
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


    while (!Serial)
    {
        ; // wait for serial port ready
    }

    Serial.printf("\nTC4-WB  STARTING...\n");
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


  server.on("/",  handlePortal);
  server.begin();

  webSocket.begin();
  Serial.println("HTTP server started");


    // event handler
  webSocket.onEvent(webSocketEvent);

  ticker_1s.start();



}

void loop() {
    webSocket.loop();  //处理websocketmie
    server.handleClient();//处理网页
    ticker_1s.update(); 

}