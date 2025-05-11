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
//����ֵ����FS_DEFAULT
#define TIM_TRIGGER_PSC (1-1)
#define TIM_TRIGGER_PERIOD (281-1)
#define RCC_DMA RCC_AHBPeriph_DMA1

#define RCC_ADC RCC_APB2Periph_ADC1
#define ADC_USED ADC1
//������Ҫ���� ADCʱ�ӵķ�Ƶϵ��
// �޸�ADC_InitStructure.ADC_ExternalTrigConv Ϊ��Ӧ��ʱ�Ӵ�������;
// ����ADC_RegularChannelConfig�����������
//DMA��Adc_Init��DMA���ֽ����޸Ĳ���
//��߲���Ƶ��
void My_ADC_DMA_Init(void);
extern flag adc_finish_fg;//�������ж���*******
extern int16_t ADC_SourceData[FFT_Len];//��adc����ֻ��12λ  �Ҷ���
extern float32_t Fs;//����Ƶ��
extern float32_t Fs_eq;//��Ч����Ƶ��
void Adc_Init(void);
void ADC_Stop(void);
void ADC_Start(void);
void Change_freq(float32_t freq, uint16_t wantid);
void ADC_Change_freq(float32_t freq);
void ADC_Set_Freq(u32 period, u32 psc);
#endif