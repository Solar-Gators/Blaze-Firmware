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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define MC_TEMP_Pin GPIO_PIN_0
#define MC_TEMP_GPIO_Port GPIOA
#define SUPP_TEMP_Pin GPIO_PIN_1
#define SUPP_TEMP_GPIO_Port GPIOA
#define V_SUPP_Pin GPIO_PIN_4
#define V_SUPP_GPIO_Port GPIOA
#define V_MAIN_Pin GPIO_PIN_5
#define V_MAIN_GPIO_Port GPIOA
#define FAN1_PWM_Pin GPIO_PIN_6
#define FAN1_PWM_GPIO_Port GPIOA
#define FAN2_PWM_Pin GPIO_PIN_7
#define FAN2_PWM_GPIO_Port GPIOA
#define Sel_Status_Pin GPIO_PIN_0
#define Sel_Status_GPIO_Port GPIOB
#define Alert_Main_Pin GPIO_PIN_1
#define Alert_Main_GPIO_Port GPIOB
#define Alert_Supp_Pin GPIO_PIN_2
#define Alert_Supp_GPIO_Port GPIOB
#define FAULT_Pin GPIO_PIN_4
#define FAULT_GPIO_Port GPIOB
#define ERROR_Pin GPIO_PIN_5
#define ERROR_GPIO_Port GPIOB
#define OK_Pin GPIO_PIN_6
#define OK_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
