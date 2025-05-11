#ifndef __ADC_H
#define __ADC_H
#include "stm32f10x.h"                  // Device header
//#include "arm_math.h"
//#include "BlueTooth_Usart.h"
#define CLK 72000000
#define FFT_Len 256
#define ADC_HIGHEST_SAMPLE_FREQ 400000
#define FS_DEFAULT 256000
#define ADC_INPUT_PIN GPIO_Pin_0
#define RCC_ADC_GPIO RCC_APB2Periph_GPIOC
#define RCC_TIM_TRIGGER RCC_APB1Periph_TIM3
#define TIM_TRIGGER TIM3
//让数值符合FS_DEFAULT
#define TIM_TRIGGER_PSC (1-1)
#define TIM_TRIGGER_PERIOD (281-1)
#define RCC_DMA RCC_AHBPeriph_DMA1

#define RCC_ADC RCC_APB2Periph_ADC1
#define ADC_USED ADC1
//可能需要设置 ADC时钟的分频系数
// 修改ADC_InitStructure.ADC_ExternalTrigConv 为相应的时钟触发控制;
// 设置ADC_RegularChannelConfig规则组的设置
//DMA于Adc_Init的DMA部分进行修改参数
//最高采样频率
void My_ADC_DMA_Init(void);
extern flag adc_finish_fg;//主函数中定义*******
extern int16_t ADC_SourceData[FFT_Len];//用adc采样只有12位  右对齐
extern float32_t Fs;//采样频率
extern float32_t Fs_eq;//等效采样频率
void Adc_Init(void);
void ADC_Stop(void);
void ADC_Start(void);
void Change_freq(float32_t freq, uint16_t wantid);
void ADC_Change_freq(float32_t freq);
void ADC_Set_Freq(u32 period, u32 psc);
#endif