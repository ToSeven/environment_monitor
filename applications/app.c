/**
 * @Author: ToSeven
 * @Date:   2017-04-06T20:52:47+08:00
 * @Email:  byone.heng@gmail.com
 * @Last modified by:   ToSeven
 * @Last modified time: 2017-04-22T15:17:21+08:00
 * @License: MIT
 */


#include "app.h"
#include "bsp_common.h"
#include "bluenrg_sdk_api.h"
#include "juma_sensor.h"
#include "oled.h"
#include "bmp.h"
#include "stm32f4xx.h"
#include <stdlib.h>
#define UPDATE_INTERVAL 100



static void read_temperature(void* arg);
static void read_humidity(void* arg);
static void read_pressure(void* arg);
static void read_magenetometer(void* arg);
static void read_accelerometer(void* arg);
static void read_gyroscopic(void* arg);
static void led_on(void* arg);
static void led_off(void* arg);
static void Show_WarnValue();
static void Monitor_Gas();
static void Monitor_People();
static void Show_Facility();

const char* board_name = "ToSeven";
static uint8_t running;
static uint8_t sendflag;
static uint16_t warnTemper;
static uint16_t rawTemper;
static uint16_t warnHumid;
static uint16_t rawHumid;
static uint32_t warnPress;
static uint32_t rawPress;
static uint8_t  show_flag;
uint8_t gas_flag;
uint8_t infrared_flag;
uint8_t facility_flag;
void jsensor_app_set_sensors(void)
{
    jsensor_app_set_sensor(JSENSOR_TYPE_HUMITY_TEMP);
    jsensor_app_set_sensor(JSENSOR_TYPE_PRESSURE);
    jsensor_app_set_sensor(JSENSOR_TYPE_MOTION_6AXIS);
    jsensor_app_set_sensor(JSENSOR_TYPE_MAGNET);
}

void on_ready(void)
{
    uint8_t tx_power_level = 7;
    uint16_t adv_interval = 100;
    uint8_t bdAddr[6];
    char name[32];
    running = 0;
    BSP_LED_On(LED0);
    HCI_get_bdAddr(bdAddr);
    sprintf(name, "%s %01x%01x", board_name, bdAddr[0], bdAddr[1]);
    /*Config Adv Parameter And Ready to Adv*/
    ble_set_adv_param(name, bdAddr, tx_power_level, adv_interval);
    ble_device_start_advertising();

    OLED_Clear();
   OLED_DrawBMP(0,0,127,7,BMP2);
   OLED_Content_clear();
   OLED_ShowString(45,2,"MEMU");
   OLED_ShowString(45,4,"SETS");
   OLED_ShowString(41,6,"ABOUT");
}
static void Show_WarnValue()
{
     if(show_flag==2)
     {
          OLED_Content_clear();
          OLED_ShowString(0,2,"WarnTemper:");
          OLED_ShowNum(88,2,warnTemper,2,16);
          OLED_ShowString(0,4,"WarnHumid:");
          OLED_ShowNum(80,4,warnHumid,2,16);
          OLED_ShowString(0,6,"warnPress:");
          OLED_ShowNum(80,6,warnPress,2,16);
     }
     run_after_delay(read_temperature,NULL,50);
}
static void Monitor_Gas()
{
     if(show_flag==1)
     {
          OLED_Line_clear(1);
          OLED_ShowString(0,2,"Monitor Gas:");
          OLED_ShowNum(96,2,gas_flag,1,16);
					gas_flag=0;
     }
     run_after_delay(Monitor_People,NULL,50);
}
static void Monitor_People()
{
     if(show_flag==1)
     {
          OLED_Line_clear(2);
          OLED_ShowString(0,4,"Monitor Peo:");
          OLED_ShowNum(96,4,infrared_flag,1,16);
          infrared_flag=0;
     }
          run_after_delay(Show_Facility,NULL,50);
}
static void Show_Facility()
{
     if(show_flag==1)
     {
          OLED_Line_clear(3);
          OLED_ShowString(0,6,"Facility On:");
          OLED_ShowNum(96,6,facility_flag,1,16);
     }
     run_after_delay(Show_WarnValue,NULL,50);
}
static void read_temperature(void* arg)
{
    int16_t humidity;
    int16_t temperature;
    JSensor_HUM_TEMP_Typedef tdef;
    	int16_t	copy_temperature=0;
    if (!running) return;
    tdef.humidity = &humidity;
    tdef.temperature = &temperature;

    if (JSENSOR_OK == jsensor_app_read_sensor(JSENSOR_TYPE_HUMITY_TEMP, (void *)&tdef)&&sendflag==0) {
        ble_device_send(0x00, 2, (uint8_t *)&temperature);

    }

		uint8_t *temp_temperature=(uint8_t *)&temperature;
          copy_temperature|=temp_temperature[0];
          copy_temperature<<=8;
          rawTemper=copy_temperature/100;
	//	printf("\n\r-tempe:%x \r\n",copy_temperature);
     if(show_flag==0)
     {
					OLED_Line_clear(1);
					OLED_ShowString(0,2,"TEM:");
          short integer=copy_temperature/100;
          short decimals=copy_temperature-integer*100;
          if(integer==0)
          {
               OLED_ShowChar(32,2,'0');
               OLED_ShowChar(40,2,'0');
          }else
               OLED_ShowNum(32,2,integer,2,16);
					OLED_ShowChar(48,2,'.');
          if(decimals==0)
          {
               OLED_ShowChar(56,2,'0');
               OLED_ShowChar(64,2,'0');
          } else
               OLED_ShowNum(56,2,decimals,2,16);
          OLED_ShowString(72,2,"'C");
	}
    run_after_delay(read_humidity, NULL, UPDATE_INTERVAL);
}

static void read_humidity(void* arg)
{
    int16_t humidity;
    int16_t temperature;
    int16_t copy_humidity=0;
    JSensor_HUM_TEMP_Typedef tdef;

    if (!running) return;

    tdef.humidity = &humidity;
    tdef.temperature = &temperature;
    if (JSENSOR_OK == jsensor_app_read_sensor(JSENSOR_TYPE_HUMITY_TEMP, (void *)&tdef)&&sendflag==0) {
       ble_device_send(0x01, 2, (uint8_t *)&humidity);
    }

          uint8_t *temp_humidity=(uint8_t *)&humidity;
          copy_humidity|=temp_humidity[0];
          copy_humidity<<=8;
          copy_humidity|=(temp_humidity[1]&0x00ff);
          rawHumid=copy_humidity/100;
     //   printf("\n\r-humidity:%x \r\n",copy_humidity);
     if(show_flag==0)
     {
          OLED_Line_clear(2);
		OLED_ShowString(0,4,"HUM:");
          short integer=copy_humidity/100;
          short decimals=copy_humidity-integer*100;
          if(integer==0)
          {
               OLED_ShowChar(32,4,'0');
               OLED_ShowChar(40,4,'0');
          }else
               OLED_ShowNum(32,4,integer,2,16);
	     OLED_ShowChar(48,4,'.');
          if(decimals==0)
          {
               OLED_ShowChar(56,4,'0');
               OLED_ShowChar(64,4,'0');
          }else
               OLED_ShowNum(56,4,decimals,2,16);
          OLED_ShowString(72,4,"RH");
     }

    run_after_delay(read_pressure, NULL, UPDATE_INTERVAL);
}

static void read_pressure(void* arg)
{
    JSensor_Press_Typedef tdef;
    int32_t pressure;
    int32_t copy_pressure=0;
    if (!running) return;

    tdef.pressure = &pressure;

    if (JSENSOR_OK == jsensor_app_read_sensor(JSENSOR_TYPE_PRESSURE, (void *)&tdef)&&sendflag==0) {
        ble_device_send(0x02, 3, (uint8_t *)&pressure);
    }

          uint8_t *temp_pressure=(uint8_t *)&pressure;
          copy_pressure|=temp_pressure[0];
          copy_pressure<<=8;
          copy_pressure|=(temp_pressure[1]&0x00ff);
          copy_pressure<<=8;
          copy_pressure|=(temp_pressure[2]&0x0000ff);
          rawPress=copy_pressure/100000;
   // printf("\n\r press:%x\r\n",copy_pressure);
          if(show_flag==0)
          {
              OLED_Line_clear(3);
			OLED_ShowString(0,6,"PRE:");
               short integer=copy_pressure/100000;
               short decimals=(copy_pressure-integer*100000)/1000;
               if(integer==0)
               {
                    OLED_ShowChar(32,6,'0');
                    OLED_ShowChar(40,6,'0');
               }else
                    OLED_ShowNum(32,6,integer,2,16);
             OLED_ShowChar(48,6,'.');
             if(decimals==0)
             {
                  OLED_ShowChar(56,6,'0');
                  OLED_ShowChar(64,6,'0');
             }   else
                    OLED_ShowNum(56,6,decimals,2,16);
               OLED_ShowString(72,6,"Pa");
          }

    run_after_delay(Monitor_Gas, NULL, UPDATE_INTERVAL);
}

static void read_magenetometer(void* arg)
{
    JSensor_MAG_Typedef tdef;
    int8_t  MAG[6];

    if (!running) return;

    tdef.MAG = MAG;

    if(JSENSOR_OK == jsensor_app_read_sensor(JSENSOR_TYPE_MAGNET, (void *)&tdef)) {
     //   ble_device_send(0x03, 6, (uint8_t*)MAG);
       // printf("%x,%x,%x,%x,%x,%x\n\r", MAG[0],MAG[1],MAG[2],MAG[3],MAG[4],MAG[5]);
    }

    run_after_delay(read_accelerometer, NULL, UPDATE_INTERVAL);
}

static void read_accelerometer(void* arg)
{
    JSensor_AXIS_Typedef tdef;
    int8_t ACC[6], GRO[6];

    if (!running) return;

    tdef.ACC = ACC;
    tdef.GRO = GRO;

    if (JSENSOR_OK == jsensor_app_read_sensor(JSENSOR_TYPE_MOTION_6AXIS, (void *)&tdef)) {
    //    ble_device_send(0x04, 6, (uint8_t*)ACC);
    }

    run_after_delay(read_gyroscopic, NULL, UPDATE_INTERVAL);
}

static void read_gyroscopic(void* arg)
{
    JSensor_AXIS_Typedef tdef;
    int8_t ACC[6], GRO[6];

    if (!running) return;

    tdef.ACC = ACC;
    tdef.GRO = GRO;

    if (JSENSOR_OK == jsensor_app_read_sensor(JSENSOR_TYPE_MOTION_6AXIS, (void *)&tdef)) {
    //    ble_device_send(0x05, 6, (uint8_t*)GRO);
    }

    run_after_delay(read_temperature, NULL, UPDATE_INTERVAL);
}

/* Device On Message receive data from ble */
void ble_device_on_message(uint8_t type, uint16_t length, uint8_t* value)
{
	switch (type) {
	     case 0x00:
               sendflag=*value;
               break;
         case 0x01:
               warnTemper=*value;
               break;
          case 0x02:
               warnHumid=*value;
               break;
          case 0x03:
               warnPress=*value;
               break;
          case 0x04:
               if(*value>0)
               {
                    Bsp_Facility_On();
                    facility_flag=1;
               }
               else{
                    Bsp_Facility_Off();
                    facility_flag=0;
               }
               break;
          case 0x05:
               show_flag=*value;
               break;

	}
}

/* Device on connect */
void ble_device_on_connect(void)
{
    running = 1;
    run_after_delay(led_off, NULL, 150);
		run_after_delay(read_temperature,NULL,100);
}

static void led_on(void* arg)
{
    BSP_LED_On(LED0);
    run_after_delay(led_off, NULL, 100);

}

static void led_off(void* arg)
{
    if (!running) return;

    BSP_LED_Off(LED0);
    run_after_delay(led_on, NULL, 150);
}

/* Device on disconnect */
void ble_device_on_disconnect(uint8_t reason)
{
    running = 0;
    /* Make the device connectable again. */
    ble_device_start_advertising();
}
void loop()
{
     if(Bsp_Gas_GetStatus()==0)
          HAL_Delay(10);
     if(Bsp_Gas_GetStatus()==0)
     {
          Buzzer_On();
          gas_flag=1;
          HAL_Delay(1000);
          Buzzer_Off();
     }
		
     if(Bsp_People_GetStatus()==1)
     {
          Buzzer_On();
          infrared_flag=1;
          HAL_Delay(1000);
          Buzzer_Off();
		 }
	
     if(rawTemper<warnTemper)
     {
          Buzzer_On();
          HAL_Delay(1000);
          Buzzer_Off();
     }
     if(rawHumid<warnHumid)
     {
          Buzzer_On();
          HAL_Delay(1000);
          Buzzer_Off();
     }
     if(rawPress<warnPress)
     {
          Buzzer_On();
          HAL_Delay(1000);
          Buzzer_Off();
     }
 }
