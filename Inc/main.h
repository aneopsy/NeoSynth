/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/	 
/* Definition for DACx clock resources */
#define DACx                            DAC
#define DACx_CHANNEL_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()
#define DMAx_CLK_ENABLE()               __HAL_RCC_DMA1_CLK_ENABLE()

#define DACx_CLK_ENABLE()               __HAL_RCC_DAC_CLK_ENABLE()
#define DACx_FORCE_RESET()              __HAL_RCC_DAC_FORCE_RESET()
#define DACx_RELEASE_RESET()            __HAL_RCC_DAC_RELEASE_RESET()

/* Definition for DACx Channel Pin */
#define DACx_CHANNEL_PIN                GPIO_PIN_4
#define DACx_CHANNEL_GPIO_PORT          GPIOA

/* Definition for DACx's Channel */
#define DACx_DMA_CHANNEL                DMA_CHANNEL_7

#define DACx_CHANNEL                    DAC_CHANNEL_1

/* Definition for DACx's DMA_STREAM */
#define DACx_DMA_INSTANCE               DMA1_Stream5

/* Definition for DACx's NVIC */
#define DACx_DMA_IRQn                   DMA1_Stream5_IRQn
#define DACx_DMA_IRQHandler             DMA1_Stream5_IRQHandler

/* Definition for DACy clock resources */
#define DACy                            DAC
#define DACy_CHANNEL_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()
#define DMAy_CLK_ENABLE()               __HAL_RCC_DMA1_CLK_ENABLE()

#define DACy_CLK_ENABLE()               __HAL_RCC_DAC_CLK_ENABLE()
#define DACy_FORCE_RESET()              __HAL_RCC_DAC_FORCE_RESET()
#define DACy_RELEASE_RESET()            __HAL_RCC_DAC_RELEASE_RESET()

/* Definition for DACy Channel Pin */
#define DACy_CHANNEL_PIN                GPIO_PIN_5
#define DACy_CHANNEL_GPIO_PORT          GPIOA

/* Definition for DACy's Channel */
#define DACy_DMA_CHANNEL                DMA_CHANNEL_7

#define DACy_CHANNEL                    DAC_CHANNEL_2

/* Definition for DACy's DMA_STREAM */
#define DACy_DMA_INSTANCE               DMA1_Stream6

/* Definition for DACy's NVIC */
#define DACy_DMA_IRQn                   DMA1_Stream6_IRQn
#define DACy_DMA_IRQHandler             DMA1_Stream6_IRQHandler

/* Definition for TIMx clock resources */
#define TIMx                           TIM9
#define TIMx_CLK_ENABLE()              __HAL_RCC_TIM9_CLK_ENABLE()


/* Definition for TIMx's NVIC */
#define TIMx_IRQn                      TIM1_BRK_TIM9_IRQn
#define TIMx_IRQHandler                TIM1_BRK_TIM9_IRQHandler

/* Config definition */
#define VOICE_NUM 										 	6
#define OSC_NUM 												6

#define STATE_IDLE 											0
#define STATE_ATTACK 										1
#define STATE_DECAY 										2
#define STATE_SUSTAIN 									3
#define STATE_RELEASE 									4
#define STATE_FREE 											255

#define PERIODE_VOICE										511

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

/* Private function prototypes -----------------------------------------------*/
void 		intro_music(void);
void 		DAC_Config(void);
void 		TIM2_Config(uint16_t);
void 		TIM3_Config(uint16_t);
void 		TIM4_Config(uint16_t);
void 		TIM5_Config(uint16_t);
void 		TIM6_Config(uint16_t);
void 		TIM7_Config(uint16_t);
void 		TIM8_Config(uint16_t);
void 		TIM9_Config(uint16_t);
void 		TIM10_Config(uint16_t);
void 		TIM11_Config(uint16_t);
void 		TIM12_Config(uint16_t);
void 		TIM13_Config(uint16_t);
void 		TIM14_Config(uint16_t);
void 		SystemClock_Config(void);
void 		Error_Handler(void);
void 		CPU_CACHE_Enable(void);
void 		play_note(uint8_t, uint8_t);
void 		stop_note(uint8_t);
void 		stop_all_note(void);
void 		mixer(void);
float 	KarlsenLPF(float, float, float, uint8_t);
void 		envelope(void);
void 		MX_GPIO_Init(void);
void 		MakeSound(void);
void 		SelfTest(void);
void 		PrepOscs(void);
void 		DefaultPatch(void);
void 		UpdateLFOs(void);
void 		ProcessPitch(void);
void 		ProcessADSRs(void);
void 		SynthOutput(void);

#endif /* __MAIN_H */
