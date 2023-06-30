
#include <Arduino.h>
#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <AsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <WebSerialLite.h>

AsyncWebServer server(80);
char ap_name[30] ;
uint8_t macAddr[6];
char serial_msg[50];

const char* ssid = "esp_serial"; // Your WiFi SSID
const char* password = "12345678"; // Your WiFi Password

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

            WiFi.macAddress(macAddr); 
            // Serial_debug.println("WiFi.mode(AP):");
            WiFi.mode(WIFI_AP);
            sprintf( ap_name ,"TC4-WB_%02X%02X%02X",macAddr[0],macAddr[1],macAddr[2]);
            WiFi.softAP(ap_name, "12345678"); // defualt IP address :192.168.4.1 password min 8 digis

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    // WebSerial is accessible at "<IP Address>/webserial" in browser
    WebSerial.begin(&server);
    /* Attach Message Callback */
    WebSerial.onMessage(recvMsg);
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

    Serial.print("CHAN;1300\n");
    delay(50);
    serial_msg = Serial.readString();
    WebSerial.println(serial_msg);

    delay(2000);





}