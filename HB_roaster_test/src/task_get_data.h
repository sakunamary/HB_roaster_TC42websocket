
#ifndef __TASK_GET_DATA_H__
#define __TASK_GET_DATA_H__


#include <Arduino.h>
#include <StringTokenizer.h>

String MSG_token[4];
String MsgString;

 

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

            StringTokenizer tokens(MsgString, ",");
            while(tokens.hasNext()){
                   MSG_token[i]=tokens.nextToken(); // prints the next token in the string
                  // Serial.println(MSG_token[i]);
                   i++;
                }
            if (xSemaphoreTake(xThermoDataMutex, xIntervel) == pdPASS)  //给温度数组的最后一个数值写入数据
                {//lock the  mutex       
                    To_artisan.BT = MSG_token[2] ;
                    To_artisan.ET = MSG_token[3] ;

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

             StringTokenizer tokens(MsgString, ",");
            while(tokens.hasNext()){
                   MSG_token[i]=tokens.nextToken(); // prints the next token in the string
                  // Serial.println(MSG_token[i]);
                   i++;
                }
            if (xSemaphoreTake(xThermoDataMutex, xIntervel) == pdPASS)  //给温度数组的最后一个数值写入数据
                {//lock the  mutex       
                    To_artisan.ap = MSG_token[2] ;
                    To_artisan.ET = MSG_token[3] ;

                        xSemaphoreGive(xThermoDataMutex);  //end of lock mutex
                }   
                
            MsgString = "";
            i=0;   





                vTaskDelayUntil(&xLastWakeTime, xIntervel);

    }
}//function 





#endif

