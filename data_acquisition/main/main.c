#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "esp_log_timestamp.h"

void esp_now_start(void);
void esp_now_tx(uint8_t *message);


void app_main(void)
{
    esp_now_start(); 
    while(1)
    {
        char message[64];
        uint32_t timestamp = esp_log_timestamp();

        snprintf(message, sizeof(message), "{'T':%lu, 'D':%d}\n",
                timestamp,
                0.0 // distance placeholder                
                );
                
        esp_now_tx((uint8_t *)message);
    }

    printf("End of ULD demo\n");
}