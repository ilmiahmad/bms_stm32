/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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
#include "stm32f4xx_hal.h"

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
#define BMIC_SPI_SEN_Pin GPIO_PIN_0
#define BMIC_SPI_SEN_GPIO_Port GPIOB
#define BMIC_ALARM1_Pin GPIO_PIN_1
#define BMIC_ALARM1_GPIO_Port GPIOB
#define BMIC_SHDN_Pin GPIO_PIN_2
#define BMIC_SHDN_GPIO_Port GPIOB
#define BMIC_FETOFF_Pin GPIO_PIN_10
#define BMIC_FETOFF_GPIO_Port GPIOB
#define BMIC_GPIO1_Pin GPIO_PIN_12
#define BMIC_GPIO1_GPIO_Port GPIOB
#define BMIC_GPIO2_Pin GPIO_PIN_13
#define BMIC_GPIO2_GPIO_Port GPIOB
#define BMIC_GPIO3_Pin GPIO_PIN_14
#define BMIC_GPIO3_GPIO_Port GPIOB
#define BMIC_SPI_SDO_Pin GPIO_PIN_15
#define BMIC_SPI_SDO_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
