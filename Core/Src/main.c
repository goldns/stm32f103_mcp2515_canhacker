/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  * Author Andrei Sedakov / Kam / kam@goldns.ru
	* CanHacker STM32F103C6T6 + MCP2515(via SPI)
	* Tested on CANHacker V2.00.02
  * First commit: 08.04.2022
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//==========================================================================
//==========================================================================
//==========================================================================
#include "CANSPI.h"
#include "MCP2515.h"
#include "stdio.h"
#include "string.h"
#include "functions.h"
#define RX_UART_MAX_SIZE 32

uint8_t rx_uart_buffer[RX_UART_MAX_SIZE]= {0};

const char hex_asc_upper[] = "0123456789ABCDEF";

uint16_t rx_uart_current_size=0;
char* str_init="INIT: "__DATE__"/"__TIME__"\r\n";  // compilate date/time
// blink settings
uint32_t BlinkDelay=300;
uint32_t last_blink=0;

// can

uCAN_MSG rxMessage;
uCAN_MSG txMessage;
int need_send=0;



// canhacker
char V_version[]="V1010\r\n";
char v_version[]="v0107\r\n";
static const char CR2[] = "\r\n";
static const char CR  = '\r';

int can_init=0;
int speed = 3; //default speed 100kbit



void my_error() {
    int iiii=5;
    while(1) {
        HAL_GPIO_TogglePin(LED_GPIO_Port,LED_Pin);
        HAL_Delay(iiii);
        iiii +=5;
        if(iiii==200) {
            iiii=5;
        }
    }
}





// in loop check if need send frame

void NeedSendFrame() {
    uint8_t dlc=0;
    uint32_t frame_id=0;

    int offset = 1;

    int idChars = 3;
    for (int i=0; i<idChars; i++) {
        frame_id <<= 4;
        frame_id += hexCharToByte(rx_uart_buffer[offset++]);
    }

    dlc=hexCharToByte(rx_uart_buffer[offset++]);
    if(dlc > 8) return;
    if(dlc < 1) return;


    txMessage.frame.idType = dSTANDARD_CAN_MSG_ID_2_0B;  //dSTANDARD_CAN_MSG_ID_2_0B
    txMessage.frame.id=frame_id;
    txMessage.frame.dlc =dlc;

    for (int i=0; i<dlc; i++) {
        char hiHex = rx_uart_buffer[offset++];
        char loHex = rx_uart_buffer[offset++];
        txMessage.frame.data[i] = hexCharToByte(loHex) + (hexCharToByte(hiHex) << 4);
    }

    need_send=0;
    CANSPI_Transmit(&txMessage);


}

void ParseRxUart() {
    switch (rx_uart_buffer[0]) {

    case 'V':
        HAL_UART_Transmit_IT(&huart1,(uint8_t*)V_version,strlen(V_version));
        break;

    case 'v':
        HAL_UART_Transmit_IT(&huart1,(uint8_t*)v_version,strlen(v_version));
        break;

    case 'C':
        can_init=0;
        HAL_UART_Transmit_IT(&huart1,(uint8_t*)CR2,strlen(CR2));
        break;
    case 'Z':
        can_init=0;
        HAL_UART_Transmit_IT(&huart1,(uint8_t*)CR2,strlen(CR2));
        break;
    case 'S':
        can_init=0;
        sscanf((char*)rx_uart_buffer, "S%d", &speed);
        HAL_UART_Transmit_IT(&huart1,(uint8_t*)CR2,strlen(CR2));
        break;

    case 'O':
        HAL_UART_Transmit_IT(&huart1,(uint8_t*)CR2,strlen(CR2));
        if(CANSPI_Initialize(speed) != true ) {   // default speed 3=100kbit
            my_error();
        }
        can_init=1;
        break;
// t05180102030405060708
    case 't':
        if(need_send == 1) {
            return;
        }
        need_send=1;
        break;

    case 'T':
        HAL_UART_Transmit_IT(&huart1,(uint8_t*)CR2,strlen(CR2));
        break;

    /*		case 'D':        // test input, delay blink ;)))))))))))))))))
            int new_delay=100;
            sscanf((char*)rx_uart_buffer, "D%d", &new_delay);
            BlinkDelay=new_delay;
    		break;
    */
    default:
        return;
    }
    return;
}


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if(huart->Instance == USART1) {
        //rx_uart_current_size=Size;
        ParseRxUart();
        HAL_UARTEx_ReceiveToIdle_IT(&huart1,rx_uart_buffer,RX_UART_MAX_SIZE);
    }
}


void my_blink() {
    if(HAL_GetTick() > (last_blink+BlinkDelay)) {
        //CANSPI_Transmit(&txMessage);
        HAL_GPIO_TogglePin(LED_GPIO_Port,LED_Pin);
        last_blink=HAL_GetTick();
    }
}


void my_init() {
    if(CANSPI_Initialize(speed) != true ) {
        my_error();
    }
    can_init=1;
    HAL_Delay(10); // need to init CAN!


    HAL_UART_Transmit_IT(&huart1,(uint8_t*)str_init,strlen(str_init));
    HAL_UARTEx_ReceiveToIdle_IT(&huart1,rx_uart_buffer,RX_UART_MAX_SIZE);
}




//// calc pkg
static inline void put_hex_byte(char *buf, uint8_t byte) {
    buf[0] = hex_asc_upper_hi(byte);
    buf[1] = hex_asc_upper_lo(byte);
}
static inline void _put_id(char *buf, int end_offset, uint16_t id) {
    while (end_offset >= 0) {
        buf[end_offset--] = hex_asc_upper[id & 0xF];
        id >>= 4;
    }
}

char PkgBuff[30]= {0}; //standart pkg   len 25+\r =26
void SendPkgToUart() {
    if(rxMessage.frame.dlc == 0) return;
    if(rxMessage.frame.dlc > 8) return;
    int length=35;
    int _timestampEnabled=1;
    int len=rxMessage.frame.dlc;
    int offset=0;
    //return;
    int isRTR = (rxMessage.frame.id & CAN_RTR_FLAG) ? 1 : 0;
    if (rxMessage.frame.id & CAN_ERR_FLAG) {
        return;
    } else if (rxMessage.frame.id & CAN_EFF_FLAG) {
        PkgBuff[0] = isRTR ? 'R' : 'T';
        put_eff_id(PkgBuff+1, rxMessage.frame.id & CAN_EFF_MASK);
        offset = 9;
    } else {
        PkgBuff[0] = isRTR ? 'r' : 't';
        put_sff_id(PkgBuff+1, rxMessage.frame.id & CAN_SFF_MASK);
        offset = 4;
    }

    PkgBuff[offset++] = hex_asc_upper_lo(rxMessage.frame.dlc);
    if (!isRTR) {
        int i;
        for (i = 0; i < len; i++) {
            put_hex_byte(PkgBuff + offset, rxMessage.frame.data[i]);
            offset += 2;
        }
    }
    if (_timestampEnabled) {
        uint16_t ts = (HAL_GetTick() % 0xEA60);
        put_hex_byte(PkgBuff + offset, ts >> 8);
        offset += 2;
        put_hex_byte(PkgBuff + offset, ts);
        offset += 2;
    }
    PkgBuff[offset++] = CR;
    PkgBuff[offset] = '\0';
    if (offset >= length) return;

    HAL_UART_Transmit(&huart1,(uint8_t*)PkgBuff,offset,5000);
    return;
}



void my_loop() {
    if(can_init == 1) {
        if(CANSPI_Receive(&rxMessage)) {
            HAL_GPIO_TogglePin(LED_GPIO_Port,LED_Pin);
            SendPkgToUart();
        }

        if(need_send > 0 ) {
            NeedSendFrame();
        }
    }
    my_blink();
}



//==========================================================================
//==========================================================================
//==========================================================================
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_SPI1_Init();
    /* USER CODE BEGIN 2 */

    HAL_Delay(10);
    my_init();
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        my_loop();
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL8;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                  |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

    /* USER CODE BEGIN SPI1_Init 0 */

    /* USER CODE END SPI1_Init 0 */

    /* USER CODE BEGIN SPI1_Init 1 */

    /* USER CODE END SPI1_Init 1 */
    /* SPI1 parameter configuration*/
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN SPI1_Init 2 */

    /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

    /* USER CODE BEGIN USART1_Init 0 */

    /* USER CODE END USART1_Init 0 */

    /* USER CODE BEGIN USART1_Init 1 */

    /* USER CODE END USART1_Init 1 */
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN USART1_Init 2 */

    /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(CAN_CS_GPIO_Port, CAN_CS_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin : LED_Pin */
    GPIO_InitStruct.Pin = LED_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pins : PC14 PC15 */
    GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /*Configure GPIO pins : PA0 PA1 PA2 PA3
                             PA4 PA8 PA11 PA12
                             PA15 */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_8|GPIO_PIN_11|GPIO_PIN_12
                          |GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*Configure GPIO pins : PB0 PB1 PB2 PB10
                             PB11 PB12 PB13 PB14
                             PB15 PB3 PB4 PB5
                             PB7 PB8 PB9 */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5
                          |GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pin : CAN_CS_Pin */
    GPIO_InitStruct.Pin = CAN_CS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(CAN_CS_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    my_error();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
