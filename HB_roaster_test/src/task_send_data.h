
#ifndef __TASK_SEND_DATA_H__
#define __TASK_SEND_DATA_H__



void task_send_data(void *pvParameters)
{ //function 

    /* Variable Definition */
    (void)pvParameters;
    TickType_t xLastWakeTime;

    const TickType_t xIntervel = 1000 / portTICK_PERIOD_MS;
   //const TickType_t xIntervel = (2 * 1000) / portTICK_PERIOD_MS;
    /* Task Setup and Initialize */
    // Initial the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();

    for (;;) // A Task shall never return or exit.
    { //for loop
        // Wait for the next cycle (intervel 750ms).
        vTaskDelayUntil(&xLastWakeTime, xIntervel);
          
    }
}//function 


#endif