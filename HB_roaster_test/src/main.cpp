#include <Arduino.h>
#include "config.h"
//Wifi libs 
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Hash.h>
#include <EEPROM.h>

#include <StringTokenizer.h>

#include "ArduinoJson.h"

//JSON for Artisan Websocket implementation
#include "ArduinoJson.h"

//Ticker to execute actions at defined intervals
#include "TickTwo.h" //ESP8266 compatible version of Ticker by sstaub

#include <ModbusIP_ESP8266.h>


ESP8266WebServer    server(80); //构建webserver类
//WebSocketsServer webSocket = WebSocketsServer(8080); //构建websockets类


char ap_name[30] ;
uint8_t macAddr[6];
bool cmd_chan1300 = true;

String MsgString_1300;
String MsgString_2400;

String local_IP;
String MSG_token1300[4];
String MSG_token2400[4];


user_wifi_t user_wifi = {" ", " "};

//Modbus Registers Offsets
const uint16_t BT_HREG = 3001;
const uint16_t ET_HREG = 3002;
const uint16_t AP_HREG = 3003;
const uint16_t INLET_HREG = 3004;
const uint16_t HEAT_HREG = 3005;

//int16_t  heat_from_Hreg = 0;

//Coil Pins
const int HEAT_OUT_PIN = PWM_HEAT; //GPIO26


//ModbusIP object
ModbusIP mb;


//functions declear for PlatfromIO rules
String processor(const String &var); // webpage function
void handlePortal();//处理设置网页处理模块
void task_get_data();
void Task_send_modbus();

String IpAddressToString(const IPAddress &ipAddress)
{
    return  String(ipAddress[0]) + String(".") +
            String(ipAddress[1]) + String(".") +
            String(ipAddress[2]) + String(".") +
            String(ipAddress[3]);
}

String processor(const String &var)
{
  if (var == "version")
    {
        return VERSION;
    }
    
    return String();
}


void task_get_data(){

    int i=0;

    if (cmd_chan1300 == true ) {
    //Serial.println("send chan;1300");
    Serial.write("CHAN;1300\n");
    Serial.flush();
    while (Serial.read() >=0 ) {}//clean buffer
    delay(200);
    Serial.write("READ\n");
    Serial.flush();
    delay(400);

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
            mb.Hreg(BT_HREG,int(MSG_token1300[1].toDouble() *100));
            mb.Hreg(ET_HREG,int(MSG_token1300[2].toDouble() *100));

            MsgString_1300 = "";    
            i=0;    
     while (Serial.read() >=0 ) {}//clean buffeR
     cmd_chan1300 = false ;

    } else {

    //Serial.println("send chan;1300");
    Serial.write("CHAN;2400\n");
    Serial.flush();
    while (Serial.read() >=0 ) {}//clean buffer
    delay(200);
    Serial.write("READ\n");
    Serial.flush();
    delay(400);

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

            //To_artisan.inlet = MSG_token2400[1].toDouble();
            //To_artisan.AP = MSG_token2400[2].toDouble();
            mb.Hreg(AP_HREG,int(MSG_token2400[1].toDouble() *100));
            mb.Hreg(INLET_HREG,int(MSG_token2400[2].toDouble() *100));

            MsgString_2400 = "";    
            i=0;    
     while (Serial.read() >=0 ) {}//clean buffeR
       cmd_chan1300 = true ;
    }
}


TickTwo ticker_task_1s_get_date(task_get_data, 1000, 0, MILLIS); 


void handlePortal() {

  if (server.method() == HTTP_POST) {

    strncpy(user_wifi.ssid,     server.arg("ssid").c_str(),     sizeof(user_wifi.ssid) );
    strncpy(user_wifi.password, server.arg("password").c_str(), sizeof(user_wifi.password) );

    user_wifi.ssid[server.arg("ssid").length()] = user_wifi.password[server.arg("password").length()] = '\0';

    EEPROM.put(0, user_wifi);
    EEPROM.commit();

    server.send_P(200,   "text/html",  wifi_sussce_html );
  } else {

    server.send_P(200,   "text/html", index_html );
  }
}


void setup() {

pinMode(HEAT_OUT_PIN, OUTPUT); 

Serial.begin(BAUDRATE);

analogWriteRange(PWM_RESOLUTION);
analogWriteFreq(PWM_FREQ);

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


// set up eeprom data
    EEPROM.begin(sizeof(user_wifi));
    EEPROM.get(0, user_wifi);


  server.on("/",  handlePortal);
  server.begin();


 ticker_task_1s_get_date.start();

//Init Modbus-TCP 

    Serial.printf("\nStart Modbus-TCP   service...\n");

    mb.server(502);		//Start Modbus IP //default port :502
    //mb.client();
    // Add SENSOR_IREG register - Use addIreg() for analog Inputs
    mb.addHreg(BT_HREG);
    mb.addHreg(ET_HREG);
    mb.addHreg(INLET_HREG);
    mb.addHreg(AP_HREG);
    mb.addHreg(HEAT_HREG);

    mb.Hreg(BT_HREG,0); //初始化赋值
    mb.Hreg(ET_HREG,0);  //初始化赋值
    mb.Hreg(INLET_HREG,0); //初始化赋值
    mb.Hreg(AP_HREG,0); //初始化赋值   
    mb.Hreg(HEAT_HREG,0);  //初始化赋值

}

void loop() {
    //webSocket.loop();  //处理websocketmie
    server.handleClient();//处理网页
    ticker_task_1s_get_date.update();//task_get_data 获取数据
    mb.task();//处理modbus数据
//heat_from_Hreg =  mb.Hreg(HEAT_HREG);//从寄存器读取火力数据

 //delay(100);

/*
       //HEAT 控制部分 
       if ((To_artisan.heat_level + heat_from_enc) <= 0 && To_artisan.heat_level >=0 ) { //如果输入小于0值，自动限制在0
            

            To_artisan.heat_level = 0;
            //heat_from_Artisan= To_artisan.heat_level ; 
            mb.Hreg(HEAT_HREG,To_artisan.heat_level); //反写最新火力数据到寄存器
            //encoder.clearCount();
            //encoder.setCount(100);
            heat_from_enc=0;
            

       } else if ((To_artisan.heat_level + heat_from_enc) >= 100 && To_artisan.heat_level <= 100){//如果输入大于100值，自动限制在100

            To_artisan.heat_level = 100;
            mb.Hreg(HEAT_HREG,To_artisan.heat_level);
            //heat_from_Artisan= To_artisan.heat_level ; 
            //encoder.clearCount();
            heat_from_enc=0; 


       } else { //To_artisan.heat_level 加减 encoder的增减量，再同步到

            To_artisan.heat_level = heat_from_enc + To_artisan.heat_level;
            mb.Hreg(HEAT_HREG,To_artisan.heat_level);//反写最新火力数据到寄存器
            //heat_from_Artisan= To_artisan.heat_level ; 
            //encoder.clearCount();
            heat_from_enc=0;

           }

*/   
    analogWrite(HEAT_OUT_PIN, map(mb.Hreg(HEAT_HREG) ,0,100,250,1000)); //将火力数据输出到PW

}
