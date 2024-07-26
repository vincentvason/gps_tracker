/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define GPS_TIMER_Pin GPIO_PIN_13
#define GPS_TIMER_GPIO_Port GPIOC
#define STM_TX_GSM_Pin GPIO_PIN_0
#define STM_TX_GSM_GPIO_Port GPIOA
#define STM_RX_GSM_Pin GPIO_PIN_1
#define STM_RX_GSM_GPIO_Port GPIOA
#define VIN_ADC_Pin GPIO_PIN_2
#define VIN_ADC_GPIO_Port GPIOA
#define BATT_ADC_Pin GPIO_PIN_3
#define BATT_ADC_GPIO_Port GPIOA
#define CHARGE_STATUS_Pin GPIO_PIN_4
#define CHARGE_STATUS_GPIO_Port GPIOA
#define NET_MODE_Pin GPIO_PIN_5
#define NET_MODE_GPIO_Port GPIOA
#define NET_STATUS_Pin GPIO_PIN_6
#define NET_STATUS_GPIO_Port GPIOA
#define GSM_PWR_KEY_Pin GPIO_PIN_7
#define GSM_PWR_KEY_GPIO_Port GPIOA
#define GPS_RESET_Pin GPIO_PIN_1
#define GPS_RESET_GPIO_Port GPIOB
#define GPS_PWR_CTRL_Pin GPIO_PIN_2
#define GPS_PWR_CTRL_GPIO_Port GPIOB
#define USART_TX_GPS_Pin GPIO_PIN_10
#define USART_TX_GPS_GPIO_Port GPIOB
#define USART_RX_GPS_Pin GPIO_PIN_11
#define USART_RX_GPS_GPIO_Port GPIOB
#define OPTIGA_RST_Pin GPIO_PIN_12
#define OPTIGA_RST_GPIO_Port GPIOB
#define I2C_SCL_Pin GPIO_PIN_13
#define I2C_SCL_GPIO_Port GPIOB
#define I2C_SDA_Pin GPIO_PIN_14
#define I2C_SDA_GPIO_Port GPIOB
#define GSM_WAKEUP_Pin GPIO_PIN_15
#define GSM_WAKEUP_GPIO_Port GPIOB
#define GSM_RST_Pin GPIO_PIN_8
#define GSM_RST_GPIO_Port GPIOA
#define USART_TX_DBG_Pin GPIO_PIN_9
#define USART_TX_DBG_GPIO_Port GPIOA
#define USART_RX_DBG_Pin GPIO_PIN_10
#define USART_RX_DBG_GPIO_Port GPIOA
#define STM_DTR_GSM_Pin GPIO_PIN_11
#define STM_DTR_GSM_GPIO_Port GPIOA
#define STM_CTS_GSM_Pin GPIO_PIN_12
#define STM_CTS_GSM_GPIO_Port GPIOA
#define HV_OUT1_Pin GPIO_PIN_15
#define HV_OUT1_GPIO_Port GPIOA
#define HV_OUT2_Pin GPIO_PIN_3
#define HV_OUT2_GPIO_Port GPIOB
#define BATT_ADC_EN_Pin GPIO_PIN_4
#define BATT_ADC_EN_GPIO_Port GPIOB
#define VIN_ADC_EN_Pin GPIO_PIN_5
#define VIN_ADC_EN_GPIO_Port GPIOB
#define TAMPER_Pin GPIO_PIN_6
#define TAMPER_GPIO_Port GPIOB
#define IO_1_WIRE_Pin GPIO_PIN_7
#define IO_1_WIRE_GPIO_Port GPIOB
#define SENSOR_PWR_CTRL_Pin GPIO_PIN_8
#define SENSOR_PWR_CTRL_GPIO_Port GPIOB
#define IO_DOORSW_Pin GPIO_PIN_9
#define IO_DOORSW_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
