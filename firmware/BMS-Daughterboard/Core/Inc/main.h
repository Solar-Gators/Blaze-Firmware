/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#define ALERT_Pin GPIO_PIN_0
#define ALERT_GPIO_Port GPIOA
#define TEMP5_Pin GPIO_PIN_4
#define TEMP5_GPIO_Port GPIOA
#define TEMP4_Pin GPIO_PIN_5
#define TEMP4_GPIO_Port GPIOA
#define TEMP3_Pin GPIO_PIN_6
#define TEMP3_GPIO_Port GPIOA
#define TEMP2_Pin GPIO_PIN_7
#define TEMP2_GPIO_Port GPIOA
#define TEMP1_Pin GPIO_PIN_0
#define TEMP1_GPIO_Port GPIOB
#define TEMP0_Pin GPIO_PIN_1
#define TEMP0_GPIO_Port GPIOB
#define FAULT_Pin GPIO_PIN_13
#define FAULT_GPIO_Port GPIOB
#define ERROR_Pin GPIO_PIN_14
#define ERROR_GPIO_Port GPIOB
#define OK_Pin GPIO_PIN_15
#define OK_GPIO_Port GPIOB
#define ADDR0_Pin GPIO_PIN_8
#define ADDR0_GPIO_Port GPIOA
#define ADDR1_Pin GPIO_PIN_9
#define ADDR1_GPIO_Port GPIOA
#define CS2_Pin GPIO_PIN_15
#define CS2_GPIO_Port GPIOA
#define CS1_Pin GPIO_PIN_6
#define CS1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
