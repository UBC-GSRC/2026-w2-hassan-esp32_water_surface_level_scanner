#include <stdio.h>
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "string.h"
#include "nvs_flash.h"
#include "esp_now.h"

#define SLAVE_ADDR  ((uint16_t)0x0C)

#define   TEMP_CPT_SEL_BIT          ((uint16_t)0x01)
#define   TEMP_CPT_ENABLE_BIT       ((uint16_t)0x01 << 1)
#define   MEASURE_MODE_BIT          ((uint16_t)0x01 << 2)
#define   MEASURE_TRIG_BIT          ((uint16_t)0x01 << 3)

typedef enum{ 
  ePid,
  eVid,
  eAddr,
  eComBaudrate,
  eComParityStop,
  eDistance,
  eInternalTempreture,
  eExternTempreture,
  eControl,
  eNoise
}eRegIndex_t;//Sensor register index

uint16_t readData(uint16_t addr, eRegIndex_t reg)
{

}

void app_main(void)
{


  while(1)
  {
    const uint8_t message[] = "Message sent from data acqusition";

    ESP_LOGI("esp now status : %s", "test");

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}