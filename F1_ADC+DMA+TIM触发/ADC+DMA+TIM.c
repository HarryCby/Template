#include "stm32f10x.h"                  // Device header
#include "ADC.h"

int16_t ADC_SourceData[FFT_Len];
float32_t Fs = FS_DEFAULT;//����Ƶ��
float32_t Fs_eq = FS_DEFAULT;//��Ч����Ƶ��



void ADC_GPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_ADC_GPIO, ENABLE);
	GPIO_InitStructure.GPIO_Pin = ADC_INPUT_PIN;		//�ܽ�
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;						//ģ������ģʽ
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);								//GPIO��
}
void ADC_TIM_TRIGGER_Configuration(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	RCC_APB1PeriphClockCmd(RCC_TIM_TRIGGER, ENABLE);
	// ʱ�ӷ�Ƶ��Ԥ��Ƶ��ֵ��������ֵ����Ϊ���� 256 kHz ����Ƶ�ʵ�����
	TIM_TimeBaseInitStructure.TIM_Period = TIM_TRIGGER_PERIOD; // �޸�Ϊ�������Ƶ�ʵ�����ֵ
	TIM_TimeBaseInitStructure.TIM_Prescaler = TIM_TRIGGER_PSC; // �޸�Ϊ�������Ƶ�ʵ�Ԥ��Ƶ��ֵ
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM_TRIGGER, &TIM_TimeBaseInitStructure);
	TIM_SelectOutputTrigger(TIM_TRIGGER, TIM_TRGOSource_Update);//�������¼�����ΪTRGO
	//    TIM_Cmd(TIM_TRIGGER, ENABLE);
}


void Adc_Init(void)
{
	ADC_GPIO_Configuration();
	ADC_TIM_TRIGGER_Configuration();
	//DMA  ��ADC�ɼ����ݴ��䵽ADC_SourceData���� ������ͨģʽ
	//����DMA_BufferSize�κ�DMAֹͣ����Ҫ�ֶ��ٴ�����
		/************************************************************/
	DMA_InitTypeDef  DMA_InitStructure;
	RCC_AHBPeriphClockCmd(RCC_DMA, ENABLE);
	DMA_DeInit(DMA1_Channel1);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&ADC1->DR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)ADC_SourceData;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = FFT_Len;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;//16λ
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel1, &DMA_InitStructure);

	//DMA_NVIC
	/************************************************************/
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	DMA_ClearITPendingBit(DMA1_IT_TC1);//transmit completed
	DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);
	//��Ҫ�ֶ��ر�DMA_IT_HT�봫���жϣ�ʹ��DMA_IT_TC���ܻ�Ĭ��ʹ��DMA_IT_HT
	DMA_ITConfig(DMA1_Channel1, DMA_IT_HT, DISABLE);
	/************************************************************/
	//ADC_Init
	ADC_InitTypeDef  ADC_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_ADC, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);//6��Ƶ��  12mhz
	ADC_DeInit(ADC1);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T3_TRGO;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_13Cycles5);	//��Ҫ����ADC_SampleTime_13Cycles5

	//Ӳ��У׼
	ADC_Cmd(ADC_USED, ENABLE);
	ADC_ResetCalibration(ADC_USED);
	while (ADC_GetResetCalibrationStatus(ADC_USED));
	ADC_StartCalibration(ADC_USED);
	while (ADC_GetCalibrationStatus(ADC_USED));
	ADC_ExternalTrigConvCmd(ADC_USED, ENABLE);

	ADC_DMACmd(ADC_USED, ENABLE);
	//�ֶ�����DMAx_Channely
	DMA_Cmd(DMA1_Channel1, ENABLE);//ʹ��DMA
	//ADC DMA׼����� ʱ�Ӵﵽ���������������ADC��DMA����
}


void ADC_Stop(void)
{
	TIM_Cmd(TIM_TRIGGER, DISABLE);
	TIM_SetCounter(TIM_TRIGGER, 0);

	DMA_Cmd(DMA1_Channel1, DISABLE);
	//�ֶ��޸�*******************************************
	
	ADC_DMACmd(ADC_USED, DISABLE);
	ADC_Cmd(ADC_USED, DISABLE);
}

void ADC_Start(void)
{
	DMA_SetCurrDataCounter(DMA1_Channel1, FFT_Len);  // ���ü�����
	//�ֶ��޸�*******************************************

	TIM_SetCounter(TIM_TRIGGER, 0);

	//�����־λ
	DMA_ClearITPendingBit(DMA1_IT_TC1); // ��������жϱ�־
	DMA_ClearITPendingBit(DMA1_IT_HT1);
	//�ֶ��޸�*******************************************
	//�ֶ��޸�*******************************************


	ADC_Cmd(ADC_USED, ENABLE);
	ADC_DMACmd(ADC_USED, ENABLE);


	DMA_Cmd(DMA1_Channel1, ENABLE);//ʹ��DMA	
	//�ֶ��޸�*******************************************


	TIM_Cmd(TIM_TRIGGER, ENABLE);

}

void ADC_Set_Freq(u32 period, u32 psc)
{
	// ���� TIM3
	TIM_Cmd(TIM_TRIGGER, DISABLE);                          // �ر� TIM3 �Խ�������
	TIM_SetAutoreload(TIM_TRIGGER, period);                 // �����Զ�����ֵ (period)
	TIM_PrescalerConfig(TIM_TRIGGER, psc, TIM_PSCReloadMode_Immediate); // ����Ԥ��Ƶ��
	Fs = 72000000 / ((psc + 1) * (period + 1));         // ���㲢����ʵ�ʲ���Ƶ��

}

//��Ե�Ƶ����ʱʹ����ͨ��������Ƶ����ʱʹ��˳��������ú���
void Change_freq(float32_t freq, uint16_t wantid) {
	float32_t target;
	target = FFT_Len * freq / wantid;

	if (target <= ADC_HIGHEST_SAMPLE_FREQ) {
		ADC_Change_freq(target);
		Fs_eq = Fs;
	}
	else {
		ADC_Change_freq(FFT_Len / wantid * freq / (2 * FFT_Len / wantid + 1));//16=FFT_Len/16
		Fs_eq = target;
	}
}
void ADC_Change_freq(float32_t freq)
{
	// �����ЧƵ��
	if (freq <= 0)
	{
		return;
	}

	const u32 clock = 72000000; // ϵͳʱ��Ƶ�� 72MHz
	uint16_t psc;
	uint16_t period;

	// ����Ŀ��ֵ��clock / freq = (psc + 1) * (period + 1)
	u32 target = round(1.0f * clock / freq);

	// ���� psc��Ѱ�Һ��ʵ� period
	for (psc = 0; psc < 65535; psc++)
	{
		// �����������Ŀ��ֵ����������ѭ��
		if (psc + 1 > target)
		{
			return;
		}
		// ���� period + 1 = target / divisor
		u32 period_plus_one = target / (psc + 1);
		// ��� period �Ƿ��� 16 λ��Χ�� (��� 65535)
		if (period_plus_one > 65536) // period_plus_one <= 65536 ȷ�� period <= 65535
		{
			continue;
		}
		// ��֤�Ƿ�ȷƥ��
		u32 actual = (psc + 1) * period_plus_one;
		if (actual == target)
		{
			period = period_plus_one - 1;

			// ���� TIM3
			TIM_Cmd(TIM_TRIGGER, DISABLE);                          // �ر� TIM3 �Խ�������
			TIM_SetAutoreload(TIM_TRIGGER, period);                 // �����Զ�����ֵ (period)
			TIM_PrescalerConfig(TIM_TRIGGER, psc, TIM_PSCReloadMode_Immediate); // ����Ԥ��Ƶ��
			Fs = clock / ((psc + 1) * (period + 1));         // ���㲢����ʵ�ʲ���Ƶ��
			return;
		}
	}
	// δ�ҵ����ʵ� psc �� period������Ӵ�����
	// ʾ����printf("�����޷�ΪƵ�� %u ���ú��ʵ� psc �� period\n", freq);
}


//ADC_DMA�жϷ������    ��������DMA�жϱ�־λ������Ҫ�޸�**************
void DMA1_Channel1_IRQHandler(void)
{
	if (DMA_GetITStatus(DMA1_IT_TC1) != RESET)
	{
		ADC_Stop();
		adc_finish_fg = True;
		DMA_ClearITPendingBit(DMA1_IT_TC1);
	}
}