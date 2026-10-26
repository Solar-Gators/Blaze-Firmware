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
#include "stm32g4xx_hal.h"

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
#define FDCAN1_STB_Pin GPIO_PIN_14
#define FDCAN1_STB_GPIO_Port GPIOC
#define OK_LED_Pin GPIO_PIN_15
#define OK_LED_GPIO_Port GPIOC
#define HSS2_LHI_Pin GPIO_PIN_0
#define HSS2_LHI_GPIO_Port GPIOA
#define HSS2_nCS_Pin GPIO_PIN_1
#define HSS2_nCS_GPIO_Port GPIOA
#define MC_LED_IN_Pin GPIO_PIN_2
#define MC_LED_IN_GPIO_Port GPIOA
#define XCAN_nCS_Pin GPIO_PIN_3
#define XCAN_nCS_GPIO_Port GPIOA
#define THROTTLE_DAC_OUT_Pin GPIO_PIN_4
#define THROTTLE_DAC_OUT_GPIO_Port GPIOA
#define REGEN_DAC_OUT_Pin GPIO_PIN_5
#define REGEN_DAC_OUT_GPIO_Port GPIOA
#define XCAN_nINT_Pin GPIO_PIN_6
#define XCAN_nINT_GPIO_Port GPIOA
#define FAN_PWM_Pin GPIO_PIN_0
#define FAN_PWM_GPIO_Port GPIOB
#define MC_DIR_CTRL_Pin GPIO_PIN_1
#define MC_DIR_CTRL_GPIO_Port GPIOB
#define MC_MODE_CTRL_Pin GPIO_PIN_2
#define MC_MODE_CTRL_GPIO_Port GPIOB
#define MC_MAIN_CTRL_Pin GPIO_PIN_10
#define MC_MAIN_CTRL_GPIO_Port GPIOB
#define MC_LED_OUT_Pin GPIO_PIN_11
#define MC_LED_OUT_GPIO_Port GPIOB
#define BRAKE_SW_NC_Pin GPIO_PIN_9
#define BRAKE_SW_NC_GPIO_Port GPIOA
#define MC_ROTATION_3V3_Pin GPIO_PIN_10
#define MC_ROTATION_3V3_GPIO_Port GPIOA
#define HSS1_nCS_Pin GPIO_PIN_2
#define HSS1_nCS_GPIO_Port GPIOD
#define HSS1_LHI_Pin GPIO_PIN_4
#define HSS1_LHI_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
