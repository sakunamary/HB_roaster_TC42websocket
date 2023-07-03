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


SoftwareSerial serial_in;// D1 RX_drumer  D2 TX_drumer 

AsyncWebServer server(80);

char ap_name[30] ;
uint8_t macAddr[6];
String myString;

const char* ssid = "esp_serial"; // Your WiFi SSID
const char* password = "12345678"; // Your WiFi Password


//设置M1 3999 版输出命令的结构数据
//定义artisan 交互的数组
struct  data_to_artisan {
    double temp_env;//环境温度
    double humi_env;//环境湿度
    double  amp_env;//环境大气压强
    int    heater_powerM ;//火力 手动
    int    heater_powerA ;//火力 自动
    int    HPM;//HPM 模式选择；>0 -> A <0 ->M  //default auto mode
    int    air;//风门
    int    roll;//搅拌
    int    cooling; //冷却
    int    sv ; // pid set value manual mode
    int    sv_auto; //pid set value auto mode
    int    targetC;// 豆温预设
    int    MODE;//是否自动烘焙
 } To_artisan ={10.0,10.0,980.00,0,0,100,0,0,0,25,25,25,-100};
//end of 定义artisan 交互的数组



/* Message callback of WebSerial */
void recvMsg(uint8_t *data, size_t len){
  WebSerial.println("Received Data...");
  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }
  WebSerial.println(d);
}



void setup() {
    Serial.begin(115200);

    serial_in.begin(115200,SWSERIAL_8N1,D1,D2 );  //RX D1 TX D2

            WiFi.macAddress(macAddr); 
            // Serial_debug.println("WiFi.mode(AP):");
            WiFi.mode(WIFI_AP);
            sprintf( ap_name ,"Serial_%02X%02X%02X",macAddr[0],macAddr[1],macAddr[2]);
            WiFi.softAP(ap_name, "12345678"); // defualt IP address :192.168.4.1 password min 8 digis

  // SerailHTML is accessible at "<IP Address>/serial" in browser
  WebSerial.begin(&server);
  //SerialHTML.onMessage(receiveMessage);

  server.begin();

}

void loop() {


/*
    if (serial_port.isOpen()) :
        #print("port_ok_flag:",port_ok_flag)

        command = 'CHAN;1300\n'
        serial_port.write(str2cmd(command))
        serial_port.flush()
        RESULT_LINE = serial_port.readline().decode('utf-8', 'ignore')[:-2] # for debug using
        # if (rl.startswith('#')):
        #     print("CHAN 1300:",rl)

        command = 'READ\n'
        serial_port.write(str2cmd(command))
        time.sleep(0.1)
        serial_port.flush()
        RESULT_LINE = serial_port.readline().decode('utf-8', 'ignore')[:-2]
        if (not len(RESULT_LINE) == 0 and not RESULT_LINE.startswith('#')):
            # print("READ 1300:",RESULT_LINE)
            res1300 = RESULT_LINE.rsplit(',')
            AT = float(res1300[0])
            ET = float(res1300[1])
            BT = float(res1300[2])

        command = 'CHAN;2400\n'
        serial_port.write(str2cmd(command))
        serial_port.flush()
       # time.sleep(0.1)
        RESULT_LINE = serial_port.readline().decode('utf-8', 'ignore')[:-2] # for debug using
        # if (rl.startswith('#')):
        #     print("CHAN 2400:",rl)

        command = 'READ\n'
        serial_port.write(str2cmd(command))
        time.sleep(0.1)
        serial_port.flush()
        RESULT_LINE = serial_port.readline().decode('utf-8', 'ignore')[:-2]
        if (not len(RESULT_LINE) == 0 and not RESULT_LINE.startswith('#')):
            # print("READ 2400: ",rl)
            res2400 = RESULT_LINE.rsplit(',')
            Inlet =float(res2400[1])

        String msg = serial_with_drumer.readStringUntil(',');

        # 打包数据成json格式

*/


    serial_in.print("CHAN;1300\n");
    delay(100);
    serial_in.flush();

    serial_in.print("READ\n");
    delay(20);
       if  (serial_in.available()>0){
        myString = serial_in.readStringUntil('C');
    }   
        serial_in.println(myString);


/*
            AP = float(res1300[0])
            ET = float(res1300[1])
            BT = float(res1300[2])
            Inlet =float(res2400[1])
*/ 

    serial_in.print("CHAN;2400\n");
    delay(20);
    serial_in.flush();

    serial_in.print("READ\n");
    delay(20);
       while (serial_in.available()){
        myString = serial_in.readStringUntil('C');
    }   
        serial_in.println(myString);


}