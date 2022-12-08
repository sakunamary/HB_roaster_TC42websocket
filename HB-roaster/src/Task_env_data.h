#ifndef __TASK_ENV_DATA_H__
#define __TASK_ENV_DATA_H__

#include <Arduino.h>
#include <Wire.h>
#include "config.h"


SemaphoreHandle_t xTaskEnvDataMutex = NULL;

DFRobot_AHT20 aht20;//构建aht20 类
Adafruit_BMP085 bmp;//构建BMP180 类


void TaskEnvData(void *pvParameters)
{

    /* Variable Definition */
    (void)pvParameters;
    TickType_t xLastWakeTime;

    const TickType_t xIntervel = ( 60*1000 * 1000) / portTICK_PERIOD_MS; //1分钟更新一次
   /* Task Setup and Initialize */
    // Initial the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();

    for (;;) // A Task shall never return or exit.
    {
        // Wait for the next cycle (intervel 750ms).
        vTaskDelayUntil(&xLastWakeTime, xIntervel);

        // Perform task actions from here
        // Read BT from MAX6675 thermal couple
        if (xSemaphoreTake(xTaskEnvDataMutex, xIntervel) == pdPASS)
        {

                if(aht20.startMeasurementReady(/* crcEn = */true))
                {
                To_artisan.temp_env= (aht20.getTemperature_C() + bmp.readTemperature())/2
                ;
                To_artisan.humi_env =aht20.getHumidity_RH();
                }
                    
                To_artisan.amp_env = float(bmp.readPressure()/100);

                //debug serial
                #if defined DEBUG_MODE
                Serial_debug.print( To_artisan.temp_env, 2);
                Serial_debug.print("\t\t");
                Serial_debug.print(To_artisan.humi_env , 2);
                Serial_debug.print("\t\t");
                Serial_debug.println(To_artisan.amp_env,2);
                #endif  
                //debug serial
        }

            xSemaphoreGive(xThermoDataMutex);
    }

}

#endif