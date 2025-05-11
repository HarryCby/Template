#include "stm32f10x.h"                  // Device header
#include "ADC.h"

int16_t ADC_SourceData[FFT_Len];
float32_t Fs = FS_DEFAULT;//采样频率
float32_t Fs_eq = FS_DEFAULT;//等效采样频率



void ADC_GPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_ADC_GPIO, ENABLE);
	GPIO_InitStructure.GPIO_Pin = ADC_INPUT_PIN;		//管脚
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;						//模拟输入模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);								//GPIO组
}
void ADC_TIM_TRIGGER_Configuration(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	RCC_APB1PeriphClockCmd(RCC_TIM_TRIGGER, ENABLE);
	// 时钟分频（预分频器值）和周期值设置为满足 256 kHz 采样频率的配置
	TIM_TimeBaseInitStructure.TIM_Period = TIM_TRIGGER_PERIOD; // 修改为满足采样频率的周期值
	TIM_TimeBaseInitStructure.TIM_Prescaler = TIM_TRIGGER_PSC; // 修改为满足采样频率的预分频器值
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM_TRIGGER, &TIM_TimeBaseInitStructure);
	TIM_SelectOutputTrigger(TIM_TRIGGER, TIM_TRGOSource_Update);//将更新事件设置为TRGO
	//    TIM_Cmd(TIM_TRIGGER, ENABLE);
}


void Adc_Init(void)
{
	ADC_GPIO_Configuration();
	ADC_TIM_TRIGGER_Configuration();
	//DMA  将ADC采集数据传输到ADC_SourceData数组 采用普通模式
	//传输DMA_BufferSize次后DMA停止，需要手动再次启动
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
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;//16位
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
	//需要手动关闭DMA_IT_HT半传输中断，使能DMA_IT_TC可能会默认使能DMA_IT_HT
	DMA_ITConfig(DMA1_Channel1, DMA_IT_HT, DISABLE);
	/************************************************************/
	//ADC_Init
	ADC_InitTypeDef  ADC_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_ADC, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);//6分频后  12mhz
	ADC_DeInit(ADC1);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T3_TRGO;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_13Cycles5);	//重要参数ADC_SampleTime_13Cycles5

	//硬件校准
	ADC_Cmd(ADC_USED, ENABLE);
	ADC_ResetCalibration(ADC_USED);
	while (ADC_GetResetCalibrationStatus(ADC_USED));
	ADC_StartCalibration(ADC_USED);
	while (ADC_GetCalibrationStatus(ADC_USED));
	ADC_ExternalTrigConvCmd(ADC_USED, ENABLE);

	ADC_DMACmd(ADC_USED, ENABLE);
	//手动修正DMAx_Channely
	DMA_Cmd(DMA1_Channel1, ENABLE);//使能DMA
	//ADC DMA准备完毕 时钟达到触发条件即可完成ADC与DMA传输
}


void ADC_Stop(void)
{
	TIM_Cmd(TIM_TRIGGER, DISABLE);
	TIM_SetCounter(TIM_TRIGGER, 0);

	DMA_Cmd(DMA1_Channel1, DISABLE);
	//手动修改*******************************************
	
	ADC_DMACmd(ADC_USED, DISABLE);
	ADC_Cmd(ADC_USED, DISABLE);
}

void ADC_Start(void)
{
	DMA_SetCurrDataCounter(DMA1_Channel1, FFT_Len);  // 重置计数器
	//手动修改*******************************************

	TIM_SetCounter(TIM_TRIGGER, 0);

	//清除标志位
	DMA_ClearITPendingBit(DMA1_IT_TC1); // 传输完成中断标志
	DMA_ClearITPendingBit(DMA1_IT_HT1);
	//手动修改*******************************************
	//手动修改*******************************************


	ADC_Cmd(ADC_USED, ENABLE);
	ADC_DMACmd(ADC_USED, ENABLE);


	DMA_Cmd(DMA1_Channel1, ENABLE);//使能DMA	
	//手动修改*******************************************


	TIM_Cmd(TIM_TRIGGER, ENABLE);

}

void ADC_Set_Freq(u32 period, u32 psc)
{
	// 配置 TIM3
	TIM_Cmd(TIM_TRIGGER, DISABLE);                          // 关闭 TIM3 以进行配置
	TIM_SetAutoreload(TIM_TRIGGER, period);                 // 设置自动重载值 (period)
	TIM_PrescalerConfig(TIM_TRIGGER, psc, TIM_PSCReloadMode_Immediate); // 设置预分频器
	Fs = 72000000 / ((psc + 1) * (period + 1));         // 计算并更新实际采样频率

}

//针对低频采样时使用普通采样，高频蔡阳时使用顺序采样所用函数
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
	// 检查无效频率
	if (freq <= 0)
	{
		return;
	}

	const u32 clock = 72000000; // 系统时钟频率 72MHz
	uint16_t psc;
	uint16_t period;

	// 计算目标值：clock / freq = (psc + 1) * (period + 1)
	u32 target = round(1.0f * clock / freq);

	// 遍历 psc，寻找合适的 period
	for (psc = 0; psc < 65535; psc++)
	{
		// 如果除数超过目标值，跳过本次循环
		if (psc + 1 > target)
		{
			return;
		}
		// 计算 period + 1 = target / divisor
		u32 period_plus_one = target / (psc + 1);
		// 检查 period 是否在 16 位范围内 (最大 65535)
		if (period_plus_one > 65536) // period_plus_one <= 65536 确保 period <= 65535
		{
			continue;
		}
		// 验证是否精确匹配
		u32 actual = (psc + 1) * period_plus_one;
		if (actual == target)
		{
			period = period_plus_one - 1;

			// 配置 TIM3
			TIM_Cmd(TIM_TRIGGER, DISABLE);                          // 关闭 TIM3 以进行配置
			TIM_SetAutoreload(TIM_TRIGGER, period);                 // 设置自动重载值 (period)
			TIM_PrescalerConfig(TIM_TRIGGER, psc, TIM_PSCReloadMode_Immediate); // 设置预分频器
			Fs = clock / ((psc + 1) * (period + 1));         // 计算并更新实际采样频率
			return;
		}
	}
	// 未找到合适的 psc 和 period，可添加错误处理
	// 示例：printf("错误：无法为频率 %u 配置合适的 psc 和 period\n", freq);
}


//ADC_DMA中断服务程序    函数名于DMA中断标志位可能需要修改**************
void DMA1_Channel1_IRQHandler(void)
{
	if (DMA_GetITStatus(DMA1_IT_TC1) != RESET)
	{
		ADC_Stop();
		adc_finish_fg = True;
		DMA_ClearITPendingBit(DMA1_IT_TC1);
	}
}