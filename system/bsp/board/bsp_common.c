/**
 * @Author: ToSeven
 * @Date:   2017-04-06T20:53:52+08:00
 * @Email:  byone.heng@gmail.com
 * @Last modified by:   ToSeven
 * @Last modified time: 2017-04-21T22:01:14+08:00
 * @License: MIT
 */

#include "bsp_common.h"
#include  "oled.h"

/**
  * @brief STM32F4xx NUCLEO BSP Driver version number V1.2.1
  */
#define __STM32F4xx_NUCLEO_BSP_VERSION_MAIN   (0x01) /*!< [31:24] main version */
#define __STM32F4xx_NUCLEO_BSP_VERSION_SUB1   (0x02) /*!< [23:16] sub1 version */
#define __STM32F4xx_NUCLEO_BSP_VERSION_SUB2   (0x01) /*!< [15:8]  sub2 version */
#define __STM32F4xx_NUCLEO_BSP_VERSION_RC     (0x00) /*!< [7:0]  release candidate */
#define __STM32F4xx_NUCLEO_BSP_VERSION        ((__STM32F4xx_NUCLEO_BSP_VERSION_MAIN << 24)\
                                             |(__STM32F4xx_NUCLEO_BSP_VERSION_SUB1 << 16)\
                                             |(__STM32F4xx_NUCLEO_BSP_VERSION_SUB2 << 8 )\
                                             |(__STM32F4xx_NUCLEO_BSP_VERSION_RC))

/**
  * @brief LINK SD Card
  */
#define SD_DUMMY_BYTE            0xFF
#define SD_NO_RESPONSE_EXPECTED  0x80

GPIO_TypeDef* GPIO_PORT[LEDn] = {LED0_GPIO_PORT};
const uint16_t GPIO_PIN[LEDn] = {LED0_PIN};
GPIO_TypeDef* BUTTON_PORT[BUTTONn] = {KEY_BUTTON_GPIO_PORT};
const uint16_t BUTTON_PIN[BUTTONn] = {KEY_BUTTON_PIN};
const uint8_t BUTTON_IRQn[BUTTONn] = {KEY_BUTTON_EXTI_IRQn};

uint32_t SpixTimeout = NUCLEO_SPIx_TIMEOUT_MAX; /*<! Value of Timeout when SPI communication fails */
static SPI_HandleTypeDef hnucleo_Spi;
static ADC_HandleTypeDef hnucleo_Adc;

/* ADC channel configuration structure declaration */
static ADC_ChannelConfTypeDef sConfig;

static void       SPIx_Init(void);
static void       SPIx_Write(uint8_t Value);
static uint32_t   SPIx_Read(void);
static void       SPIx_Error(void);
static void       SPIx_MspInit(SPI_HandleTypeDef *hspi);

static void       ADCx_Init(void);
static void       ADCx_MspInit(ADC_HandleTypeDef *hadc);

void              SD_IO_Init(void);
HAL_StatusTypeDef SD_IO_WriteCmd(uint8_t Cmd, uint32_t Arg, uint8_t Crc, uint8_t Response);
HAL_StatusTypeDef SD_IO_WaitResponse(uint8_t Response);
void              SD_IO_WriteDummy(void);
void              SD_IO_WriteByte(uint8_t Data);
uint8_t           SD_IO_ReadByte(void);

void              LCD_IO_Init(void);
void              LCD_IO_WriteData(uint8_t Data);
void              LCD_IO_WriteMultipleData(uint8_t *pData, uint32_t Size);
void              LCD_IO_WriteReg(uint8_t LCDReg);
void              LCD_Delay(uint32_t delay);
void              LCD12864_Init(void);
void              WriteToLcd(uint8_t ,uint8_t);
void              ClearScreen(unsigned char );
void 		   				delay(unsigned short);
void 						DisplayPic(unsigned char *dp);
void 						LCDAddr(unsigned char page,unsigned char column);


void bsp_init(void)
{
    HAL_Init();
    /* Configure the system clock */
    BSP_SystemClock_Config();
    /* Configure LED0 */
    BSP_LED_Init(LED0);
    /*UART2 init*/
    UART_Init();
    HAL_Delay(1000);
    HarmfulGas_Init();
    People_Init();
    Facility_Control_Init();
    Buzzer_Control_Init();
   	OLED_Init();
     OLED_Clear();
	OLED_ShowCHinese(40,2,0);
		OLED_ShowCHinese(70,2,1);
		OLED_ShowCHinese(0,5,2);
		OLED_ShowCHinese(16,5,3);
		OLED_ShowCHinese(32,5,4);
		OLED_ShowCHinese(48,5,5);
		OLED_ShowCHinese(64,5,6);
		OLED_ShowCHinese(80,5,7);
		OLED_ShowCHinese(96,5,8);
		OLED_ShowCHinese(112,5,9);
		HAL_Delay(4000);

		/*sensor layer init*/
		jsensor_sys_init();
    /* Initialize the BlueNRG SPI driver */
    BNRG_SPI_Init();
    /* Initialize the BlueNRG HCI */
    HCI_Init();
    /* Reset BlueNRG hardware */
    BlueNRG_RST();
    /* Enable Power Clock */
    __HAL_RCC_PWR_CLK_ENABLE();

    rtc_init();
}

uint32_t BSP_GetVersion(void)
{
    return __STM32F4xx_NUCLEO_BSP_VERSION;
}

void BSP_LED_Init(Led_TypeDef Led)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    /* Enable the GPIO_LED Clock */
    LEDx_GPIO_CLK_ENABLE(Led);

    /* Configure the GPIO_LED pin */
    GPIO_InitStruct.Pin = GPIO_PIN[Led];
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;

    HAL_GPIO_Init(GPIO_PORT[Led], &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_RESET);
}

void BSP_LED_On(Led_TypeDef Led)
{
    HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_RESET);
}

void BSP_LED_Off(Led_TypeDef Led)
{
    HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_SET);
}

void BSP_LED_Toggle(Led_TypeDef Led)
{
    HAL_GPIO_TogglePin(GPIO_PORT[Led], GPIO_PIN[Led]);
}

/**
  * @brief  Configures Button GPIO and EXTI Line.
  * @param  Button: Specifies the Button to be configured.
  *   This parameter should be: BUTTON_KEY
  * @param  ButtonMode: Specifies Button mode.
  *   This parameter can be one of following parameters:
  *     @arg BUTTON_MODE_GPIO: Button will be used as simple IO
  *     @arg BUTTON_MODE_EXTI: Button will be connected to EXTI line with interrupt
  *                            generation capability
  * @retval None
  */
void BSP_PB_Init(Button_TypeDef Button, ButtonMode_TypeDef ButtonMode)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /* Enable the BUTTON Clock */
    BUTTONx_GPIO_CLK_ENABLE(Button);

    if(ButtonMode == BUTTON_MODE_GPIO)
    {
        /* Configure Button pin as input */
        GPIO_InitStruct.Pin = BUTTON_PIN[Button];
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
        HAL_GPIO_Init(BUTTON_PORT[Button], &GPIO_InitStruct);
    }

    if(ButtonMode == BUTTON_MODE_EXTI)
    {
        /* Configure Button pin as input with External interrupt */
        GPIO_InitStruct.Pin = BUTTON_PIN[Button];
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
        HAL_GPIO_Init(BUTTON_PORT[Button], &GPIO_InitStruct);

        /* Enable and set Button EXTI Interrupt to the lowest priority */
        HAL_NVIC_SetPriority((IRQn_Type)(BUTTON_IRQn[Button]), 0x0F, 0x00);
        HAL_NVIC_EnableIRQ((IRQn_Type)(BUTTON_IRQn[Button]));
    }
}

uint32_t BSP_PB_GetState(Button_TypeDef Button)
{
    return HAL_GPIO_ReadPin(BUTTON_PORT[Button], BUTTON_PIN[Button]);
}

void BSP_SystemClock_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    //RCC_OscInitStruct.HSICalibrationValue = 6;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);

//  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK;
//  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
//  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1);
}


/******************************************************************************
                            BUS OPERATIONS
*******************************************************************************/
/**
  * @brief  Initializes SPI MSP.
  * @param  None
  * @retval None
  */
static void SPIx_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    /*** Configure the GPIOs ***/
    /* Enable GPIO clock */
    NUCLEO_SPIx_SCK_GPIO_CLK_ENABLE();
    NUCLEO_SPIx_MISO_MOSI_GPIO_CLK_ENABLE();

    /* Configure SPI SCK */
    GPIO_InitStruct.Pin = NUCLEO_SPIx_SCK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = NUCLEO_SPIx_SCK_AF;
    HAL_GPIO_Init(NUCLEO_SPIx_SCK_GPIO_PORT, &GPIO_InitStruct);

    /* Configure SPI MISO and MOSI */
    GPIO_InitStruct.Pin = NUCLEO_SPIx_MOSI_PIN;
    GPIO_InitStruct.Alternate = NUCLEO_SPIx_MISO_MOSI_AF;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    HAL_GPIO_Init(NUCLEO_SPIx_MISO_MOSI_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = NUCLEO_SPIx_MISO_PIN;
    HAL_GPIO_Init(NUCLEO_SPIx_MISO_MOSI_GPIO_PORT, &GPIO_InitStruct);

    /*** Configure the SPI peripheral ***/
    /* Enable SPI clock */
    NUCLEO_SPIx_CLK_ENABLE();
}

/**
  * @brief  Initializes SPI HAL.
  * @param  None
  * @retval None
  */
static void SPIx_Init(void)
{
    if(HAL_SPI_GetState(&hnucleo_Spi) == HAL_SPI_STATE_RESET)
    {
        /* SPI Config */
        hnucleo_Spi.Instance = NUCLEO_SPIx;
        /* SPI baudrate is set to 12,5 MHz maximum (PCLK2/SPI_BaudRatePrescaler = 100/8 = 12,5 MHz)
         to verify these constraints:
            - ST7735 LCD SPI interface max baudrate is 15MHz for write and 6.66MHz for read
              Since the provided driver doesn't use read capability from LCD, only constraint
              on write baudrate is considered.
            - SD card SPI interface max baudrate is 25MHz for write/read
            - PCLK2 max frequency is 100 MHz
         */
        hnucleo_Spi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
        hnucleo_Spi.Init.Direction = SPI_DIRECTION_2LINES;
        hnucleo_Spi.Init.CLKPhase = SPI_PHASE_2EDGE;
        hnucleo_Spi.Init.CLKPolarity = SPI_POLARITY_HIGH;
        hnucleo_Spi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
        hnucleo_Spi.Init.CRCPolynomial = 7;
        hnucleo_Spi.Init.DataSize = SPI_DATASIZE_8BIT;
        hnucleo_Spi.Init.FirstBit = SPI_FIRSTBIT_MSB;
        hnucleo_Spi.Init.NSS = SPI_NSS_SOFT;
        hnucleo_Spi.Init.TIMode = SPI_TIMODE_DISABLED;
        hnucleo_Spi.Init.Mode = SPI_MODE_MASTER;

        SPIx_MspInit(&hnucleo_Spi);
        HAL_SPI_Init(&hnucleo_Spi);
    }
}

/**
  * @brief  SPI Read 4 bytes from device.
  * @param  None
  * @retval Read data
*/
static uint32_t SPIx_Read(void)
{
    HAL_StatusTypeDef status = HAL_OK;
    uint32_t readvalue = 0;
    uint32_t writevalue = 0xFFFFFFFF;

    status = HAL_SPI_TransmitReceive(&hnucleo_Spi, (uint8_t*) &writevalue, (uint8_t*) &readvalue, 1, SpixTimeout);

    /* Check the communication status */
    if(status != HAL_OK)
    {
        /* Execute user timeout callback */
        SPIx_Error();
    }

    return readvalue;
}

/**
  * @brief  SPI Write a byte to device.
  * @param  Value: value to be written
  * @retval None
  */
static void SPIx_Write(uint8_t Value)
{
    HAL_StatusTypeDef status = HAL_OK;

    status = HAL_SPI_Transmit(&hnucleo_Spi, (uint8_t*) &Value, 1, SpixTimeout);

    /* Check the communication status */
    if(status != HAL_OK)
    {
        /* Execute user timeout callback */
        SPIx_Error();
    }
}

/**
  * @brief  SPI error treatment function.
  * @param  None
  * @retval None
  */
static void SPIx_Error (void)
{
    /* De-initialize the SPI communication BUS */
    HAL_SPI_DeInit(&hnucleo_Spi);

    /* Re-Initiaize the SPI communication BUS */
    SPIx_Init();
}

/******************************************************************************
                            LINK OPERATIONS
*******************************************************************************/

/********************************* LINK SD ************************************/
/**
  * @brief  Initializes the SD Card and put it into StandBy State (Ready for
  *         data transfer).
  * @param  None
  * @retval None
  */
void SD_IO_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct;
    uint8_t counter;

    /* SD_CS_GPIO Periph clock enable */
    SD_CS_GPIO_CLK_ENABLE();

    /* Configure SD_CS_PIN pin: SD Card CS pin */
    GPIO_InitStruct.Pin = SD_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(SD_CS_GPIO_PORT, &GPIO_InitStruct);

    /*------------Put SD in SPI mode--------------*/
    /* SD SPI Config */
    SPIx_Init();

    /* SD chip select high */
    SD_CS_HIGH();

    /* Send dummy byte 0xFF, 10 times with CS high */
    /* Rise CS and MOSI for 80 clocks cycles */
    for (counter = 0; counter <= 9; counter++)
    {
        /* Send dummy byte 0xFF */
        SD_IO_WriteByte(SD_DUMMY_BYTE);
    }
}

/**
  * @brief  Writes a byte on the SD.
  * @param  Data: byte to send.
  * @retval None
  */
void SD_IO_WriteByte(uint8_t Data)
{
    /* Send the byte */
    SPIx_Write(Data);
}

/**
  * @brief  Reads a byte from the SD.
  * @param  None
  * @retval The received byte.
  */
uint8_t SD_IO_ReadByte(void)
{
    uint8_t data = 0;

    /* Get the received data */
    data = SPIx_Read();

    /* Return the shifted data */
    return data;
}

/**
  * @brief  Sends 5 bytes command to the SD card and get response.
  * @param  Cmd: The user expected command to send to SD card.
  * @param  Arg: The command argument
  * @param  Crc: The CRC
  * @param  Response: Expected response from the SD card
  * @retval HAL_StatusTypeDef HAL Status
  */
HAL_StatusTypeDef SD_IO_WriteCmd(uint8_t Cmd, uint32_t Arg, uint8_t Crc, uint8_t Response)
{
    uint32_t counter = 0x00;
    uint8_t frame[6];

    /* Prepare Frame to send */
    frame[0] = (Cmd | 0x40);         /* Construct byte 1 */
    frame[1] = (uint8_t)(Arg >> 24); /* Construct byte 2 */
    frame[2] = (uint8_t)(Arg >> 16); /* Construct byte 3 */
    frame[3] = (uint8_t)(Arg >> 8);  /* Construct byte 4 */
    frame[4] = (uint8_t)(Arg);       /* Construct byte 5 */
    frame[5] = (Crc);                /* Construct byte 6 */

    /* SD chip select low */
    SD_CS_LOW();

    /* Send Frame */
    for (counter = 0; counter < 6; counter++)
    {
        SD_IO_WriteByte(frame[counter]); /* Send the Cmd bytes */
    }

    if(Response != SD_NO_RESPONSE_EXPECTED)
    {
        return SD_IO_WaitResponse(Response);
    }

    return HAL_OK;
}

/**
  * @brief  Waits response from the SD card
  * @param  Response: Expected response from the SD card
  * @retval HAL_StatusTypeDef HAL Status
  */
HAL_StatusTypeDef SD_IO_WaitResponse(uint8_t Response)
{
    uint32_t timeout = 0xFFFF;

    /* Check if response is got or a timeout is happen */
    while ((SD_IO_ReadByte() != Response) && timeout)
    {
        timeout--;
    }

    if (timeout == 0)
    {
        /* After time out */
        return HAL_TIMEOUT;
    }
    else
    {
        /* Right response got */
        return HAL_OK;
    }
}

/**
  * @brief  Sends dummy byte with CS High.
  * @param  None
  * @retval None
  */
void SD_IO_WriteDummy(void)
{
    /* SD chip select high */
    SD_CS_HIGH();

    /* Send Dummy byte 0xFF */
    SD_IO_WriteByte(SD_DUMMY_BYTE);
}

/**
 *   Initializes 12864lcd st7565ic
 */
void delay(unsigned short tick)
{
     for(int x=0;x<tick;x++)
          for(int j=0;j<200;j++);
}
void LCD12864_Init()
{
     GPIO_InitTypeDef GPIO_InitStruct;
     LCD12864_COMMON_GPIO_CLK_ENABLE();
     LCD12864_RESET_GPIO_CLK_ENABLE();

     /*Configure lcd12864 CS RS WR RD  pin */
     GPIO_InitStruct.Pin=LCD12864_CS_PIN;
     GPIO_InitStruct.Mode=GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull=GPIO_NOPULL;
     GPIO_InitStruct.Speed=GPIO_SPEED_HIGH;
     HAL_GPIO_Init(LCD12864_CS_GPIO_PORT,&GPIO_InitStruct);

     GPIO_InitStruct.Pin=LCD12864_RS_PIN|LCD12864_RD_PIN|LCD12864_WR_PIN;
     HAL_GPIO_Init(LCD12864_RS_GPIO_PORT,&GPIO_InitStruct);

     GPIO_InitStruct.Pin=LCD12864_RESET_PIN;
     HAL_GPIO_Init(LCD12864_RESET_GPIO_PORT,&GPIO_InitStruct);

     // LCD12864_RS_HiGH();
     // LCD12864_CS_HIGH();
     // LCD12864_RESET_HIGH();
     // LCD12864_RD_HIGH();
     // LCD12864_WR_HIGH();
     /*Configure LCD12864  DATA OR COMMAND PIN */
     LCD12864_PIN1_PIN2_PIN3_GPIO_CLK_ENABLE();
     LCD12864_PIN4_PIN6_GPIO_CLK_ENABLE();
     LCD12864_PIN5_PIN7_PIN8_GPIO_CLK_ENABLE();
     GPIO_InitStruct.Pin=lCD12864_PIN1|lCD12864_PIN2|lCD12864_PIN3;
     HAL_GPIO_Init(LCD12864_PIN1_PIN2_PIN3_GPIO_PORT,&GPIO_InitStruct);
     GPIO_InitStruct.Pin=lCD12864_PIN4|lCD12864_PIN6;
     HAL_GPIO_Init(LCD12864_PIN4_PIN6_GPIO_PORT,&GPIO_InitStruct);
     GPIO_InitStruct.Pin=lCD12864_PIN5|lCD12864_PIN7|lCD12864_PIN8;
     HAL_GPIO_Init(LCD12864_PIN5_PIN7_PIN8_GPIO_PORT,&GPIO_InitStruct);


     LCD12864_RESET_LOW();
   //  delay(10);
     for(uint8_t i=0;i<100;i++);
	LCD12864_CS_LOW();
     LCD12864_RESET_HIGH();
     // LCD12864_PIN8_HIGH();
     // LCD12864_PIN7_HIGH();
     // LCD12864_PIN6_HIGH();
     // LCD12864_PIN5_HIGH();
     // LCD12864_PIN4_HIGH();
     // LCD12864_PIN3_HIGH();
     // LCD12864_PIN2_HIGH();
     // LCD12864_PIN1_HIGH();

     WriteToLcd(0xE2,1);
     for(uint8_t i=0;i<100;i++);
     // WriteToLcd(0x2c,1);
     // delay(5);
     // WriteToLcd(0x2e,1);
     // delay(5);
     // WriteToLcd(0x2f,1);
     // delay(5);
     //WriteToLcd(0x24,1);
     //WriteToLcd(0x81,1);
     WriteToLcd(0xA1,1);
     WriteToLcd(0xc8,1);
     WriteToLcd(0xA6,1);
     WriteToLcd(0xA4,1);
     WriteToLcd(0xA2,1);
     WriteToLcd(0xf8,1);
     WriteToLcd(0x01,1);
     WriteToLcd(0x81,1);
     WriteToLcd(0x23,1);
     WriteToLcd(0x25,1);
     WriteToLcd(0x2f,1);
     for(uint8_t i=0;i<100;i++);

     WriteToLcd(0x40,1);
    //
     WriteToLcd(0xaf,1);
		 for(uint8_t i=0;i<100;i++);
		ClearScreen(0x00);
		ClearScreen(0xFF);
		ClearScreen(0x00);

}

/*
     Write a data into lcd12864
 */
void WriteToLcd(uint8_t num,uint8_t flag)
{
     LCD12864_CS_LOW();
		LCD12864_RD_HIGH();
     if(flag)
          LCD12864_RS_LOW();       //select command
     else
          LCD12864_RS_HiGH();      //select data
     LCD12864_WR_LOW();
		 __nop();
		 __nop();
		 __nop();
     for(int x=0;x<8;x++)
     {
          // if(flag)
          //      delay(1);
          if(num&0x80)
          {
               switch (x) {
                    case 0 :
                         LCD12864_PIN1_HIGH();
                         break;
                    case 1:
                         LCD12864_PIN2_HIGH();
                         break;
                    case 2:
                         LCD12864_PIN3_HIGH();
                         break;
                    case 3:
                         LCD12864_PIN4_HIGH();
                         break;
                    case 4:
                         LCD12864_PIN5_HIGH();
                         break;
                    case 5:
                         LCD12864_PIN6_HIGH();
                         break;
                    case 6:
                         LCD12864_PIN7_HIGH();
                         break;
                    case 7:
                         LCD12864_PIN8_HIGH();
                         break;
               }
          }else{
               switch (x) {
                    case 0 :
                         LCD12864_PIN1_LOW();
                         break;
                    case 1:
                         LCD12864_PIN2_LOW();
                         break;
                    case 2:
                         LCD12864_PIN3_LOW();
                         break;
                    case 3:
                         LCD12864_PIN4_LOW();
                         break;
                    case 4:
                         LCD12864_PIN5_LOW();
                         break;
                    case 5:
                         LCD12864_PIN6_LOW();
                         break;
                    case 6:
                         LCD12864_PIN7_LOW();
                         break;
                    case 7:
                         LCD12864_PIN8_LOW();
                         break;
               }

          }
          num<<=1;
     }
		  __nop();
		 __nop();
		 __nop();
     LCD12864_WR_HIGH();
}
/*
     clear lcd12864  screen
 */
void ClearScreen(unsigned char num)
{
     for(int x=0;x<8;x++)
     {
          WriteToLcd(0xb0+x,1);
          WriteToLcd(0x10,1);
          WriteToLcd(0x04,1);
          for(int i=0;i<128;i++)
          {
			WriteToLcd(num,0);
					//	delay(1);
	     }
     }
}
/*
     Locate LCD12864 Address
 */
void LCDAddr(unsigned char page,unsigned char column)
{
			LCD12864_CS_LOW();
			column--;
			page--;
			WriteToLcd(0xb0+page,1);
			WriteToLcd(((column>>4)&0x0f)+0x10,1);
			WriteToLcd(column&0x0f,1);
}
/*
     Display a Picture on Lcd12864
 */
 void DisplayPic(unsigned char *dp)
 {
 	unsigned char i,j;
     LCD12864_CS_LOW();
 	for(i=0;i<8;i++)
 	{
 		for(j=0;j<2;j++)
 		{
 			LCDAddr(i,0);
 			for (j=0;j<128;j++)
 			{
 				WriteToLcd(*dp,0);
 				dp++;
 			}
 		}
 	}
     LCD12864_CS_HIGH();
 }

/********************************* LINK LCD ***********************************/
/**
  * @brief  Initializes the LCD.
  * @param  None
  * @retval None
  */
void LCD_IO_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    /* LCD_CS_GPIO and LCD_DC_GPIO Periph clock enable */
    LCD_CS_GPIO_CLK_ENABLE();
    LCD_DC_GPIO_CLK_ENABLE();

    /* Configure LCD_CS_PIN pin: LCD Card CS pin */
    GPIO_InitStruct.Pin = LCD_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(SD_CS_GPIO_PORT, &GPIO_InitStruct);

    /* Configure LCD_DC_PIN pin: LCD Card DC pin */
    GPIO_InitStruct.Pin = LCD_DC_PIN;
    HAL_GPIO_Init(LCD_DC_GPIO_PORT, &GPIO_InitStruct);

    /* LCD chip select high */
    LCD_CS_HIGH();

    /* LCD SPI Config */
    SPIx_Init();
}

/**
  * @brief  Writes command to select the LCD register.
  * @param  LCDReg: Address of the selected register.
  * @retval None
  */
void LCD_IO_WriteReg(uint8_t LCDReg)
{
    /* Reset LCD control line CS */
    LCD_CS_LOW();

    /* Set LCD data/command line DC to Low */
    LCD_DC_LOW();

    /* Send Command */
    SPIx_Write(LCDReg);

    /* Deselect : Chip Select high */
    LCD_CS_HIGH();
}

/**
  * @brief  Writes data to select the LCD register.
  *         This function must be used after st7735_WriteReg() function
  * @param  Data: data to write to the selected register.
  * @retval None
  */
void LCD_IO_WriteData(uint8_t Data)
{
    /* Reset LCD control line CS */
    LCD_CS_LOW();

    /* Set LCD data/command line DC to High */
    LCD_DC_HIGH();

    /* Send Data */
    SPIx_Write(Data);

    /* Deselect : Chip Select high */
    LCD_CS_HIGH();
}

/**
  * @brief  Writes register value.
  * @param  pData: Pointer on the register value
  * @param  Size: Size of byte to transmit to the register
  * @retval None
  */
void LCD_IO_WriteMultipleData(uint8_t *pData, uint32_t Size)
{
    uint32_t counter = 0;

    /* Reset LCD control line CS */
    LCD_CS_LOW();

    /* Set LCD data/command line DC to High */
    LCD_DC_HIGH();

    if (Size == 1)
    {
        /* Only 1 byte to be sent to LCD - general interface can be used */
        /* Send Data */
        SPIx_Write(*pData);
    }
    else
    {
        /* Several data should be sent in a raw */
        /* Direct SPI accesses for optimization */
        for (counter = Size; counter != 0; counter--)
        {
            while(((hnucleo_Spi.Instance->SR) & SPI_FLAG_TXE) != SPI_FLAG_TXE)
            {
            }
            /* Need to invert bytes for LCD*/
            *((__IO uint8_t*)&hnucleo_Spi.Instance->DR) = *(pData+1);

            while(((hnucleo_Spi.Instance->SR) & SPI_FLAG_TXE) != SPI_FLAG_TXE)
            {
            }
            *((__IO uint8_t*)&hnucleo_Spi.Instance->DR) = *pData;
            counter--;
            pData += 2;
        }

        /* Wait until the bus is ready before releasing Chip select */
        while(((hnucleo_Spi.Instance->SR) & SPI_FLAG_BSY) != RESET)
        {
        }
    }

    /* Deselect : Chip Select high */
    LCD_CS_HIGH();
}

/**
  * @brief  Wait for loop in ms.
  * @param  Delay in ms.
  * @retval None
  */
void LCD_Delay(uint32_t Delay)
{
    HAL_Delay(Delay);
}
/*
     initializes buzzer
 */
void Buzzer_Control_Init()
{
     GPIO_InitTypeDef GPIO_InitStruct;
     __GPIOC_CLK_ENABLE();
     GPIO_InitStruct.Pin=GPIO_PIN_13 ;  //MQ-135模块 ttl输入引脚
     GPIO_InitStruct.Mode=GPIO_MODE_OUTPUT_PP;
     HAL_GPIO_Init(GPIOC,&GPIO_InitStruct);
}
/**
 *  voltage relay module initializes
 */
void Facility_Control_Init()
{
     GPIO_InitTypeDef GPIO_InitStruct;
     __GPIOA_CLK_ENABLE();
     GPIO_InitStruct.Pin=GPIO_PIN_8 ;  //MQ-135模块 ttl输入引脚
     GPIO_InitStruct.Mode=GPIO_MODE_OUTPUT_PP;
     HAL_GPIO_Init(GPIOA,&GPIO_InitStruct);
}
void Bsp_Facility_Off()
{
     HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,GPIO_PIN_SET);
}
void Bsp_Facility_On()
{
     HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,GPIO_PIN_RESET);
}
/*
     infrared sensor initializes
 */
void People_Init()
{
     GPIO_InitTypeDef GPIO_InitStruct;
     __GPIOA_CLK_ENABLE();
     GPIO_InitStruct.Pin=GPIO_PIN_0 ;  //MQ-135模块 ttl输入引脚
     GPIO_InitStruct.Mode=GPIO_MODE_INPUT;
     HAL_GPIO_Init(GPIOB,&GPIO_InitStruct);
}
uint8_t Bsp_People_GetStatus()
{
     return HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0);
}
/**
 *  Initializes HarmfulGas module
 */
void HarmfulGas_Init()
{
     GPIO_InitTypeDef GPIO_InitStruct;
     __GPIOA_CLK_ENABLE();
     GPIO_InitStruct.Pin=GPIO_PIN_1 ;  //MQ-135模块 ttl输入引脚
     GPIO_InitStruct.Mode=GPIO_MODE_INPUT;
     HAL_GPIO_Init(GPIOA,&GPIO_InitStruct);
}
//low voltage work
uint8_t Bsp_Gas_GetStatus()
{
     return HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_1);
}
/******************************* LINK JOYSTICK ********************************/
/**
  * @brief  Initializes ADC MSP.
  * @param  None
  * @retval None
  */
static void ADCx_MspInit(ADC_HandleTypeDef *hadc)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    /*** Configure the GPIOs ***/
    /* Enable GPIO clock */
    NUCLEO_ADCx_GPIO_CLK_ENABLE();

    /* Configure the selected ADC Channel as analog input */
    GPIO_InitStruct.Pin = NUCLEO_ADCx_GPIO_PIN ;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(NUCLEO_ADCx_GPIO_PORT, &GPIO_InitStruct);

    /*** Configure the ADC peripheral ***/
    /* Enable ADC clock */
    NUCLEO_ADCx_CLK_ENABLE();
}

/**
  * @brief  Initializes ADC HAL.
  * @param  None
  * @retval None
  */
static void ADCx_Init(void)
{
     ADC_ChannelConfTypeDef AdcConfig;
    if(HAL_ADC_GetState(&hnucleo_Adc) == HAL_ADC_STATE_RESET)
    {
        /* ADC Config */
        hnucleo_Adc.Instance                   = NUCLEO_ADCx;
        hnucleo_Adc.Init.ClockPrescaler        = ADC_CLOCKPRESCALER_PCLK_DIV4; /* (must not exceed 36MHz) */
        hnucleo_Adc.Init.Resolution            = ADC_RESOLUTION12b;
        hnucleo_Adc.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
        hnucleo_Adc.Init.ContinuousConvMode    = DISABLE;
        hnucleo_Adc.Init.DiscontinuousConvMode = DISABLE;
        hnucleo_Adc.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
        hnucleo_Adc.Init.EOCSelection          = EOC_SINGLE_CONV;
        hnucleo_Adc.Init.NbrOfConversion       = 1;
        hnucleo_Adc.Init.DMAContinuousRequests = DISABLE;

        ADCx_MspInit(&hnucleo_Adc);
        HAL_ADC_Init(&hnucleo_Adc);
    }
    AdcConfig.Channel=ADC_CHANNEL_8;
    AdcConfig.Rank=1;
    AdcConfig.SamplingTime=ADC_SAMPLETIME_3CYCLES;
    AdcConfig.Offset=0;
    if(HAL_ADC_ConfigChannel(&hnucleo_Adc,&AdcConfig)!=HAL_OK)
          Error_Handler();
     HAL_ADC_Start(&hnucleo_Adc);
     while(HAL_ADC_GetState(&hnucleo_Adc)!=HAL_ADC_STATE_EOC);
     uint32_t temp=HAL_ADC_GetValue(&hnucleo_Adc);
     //printf("\n\radc:%d\r\n",temp);
}

/**
 * @brief  EXTI line detection callback.
 * @param  Specifies the pins connected EXTI line
 * @retval None
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    HCI_Isr();
}

void Error_Handler(void)
{
    while (1)
    {
        /* Toggle LED2 with a period of one second */
        BSP_LED_Toggle(LED0);
        HAL_Delay(1000);
    }
}
