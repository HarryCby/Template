//����ͨ��
#include "stm32f10x.h"                  // Device header
#include "BlueTooth_Usart.h"
USART_RX_Packet rxPacket;
uint8_t dataIndex = 0;
uint8_t packetState = 0; // 0:�ȴ�ͷ 1:�������� 2:�ȴ�β
//USART3 ��������ͨ�� PB10��TX  PB11��RX  ��������Ϊ9600 ������
void BTUSART_Init(u32 bound)
{
    //GPIO�˿�����
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    //USART3_TX   PB10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    //USART3_RX          PB11
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // USART2 NVIC ����
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3; // ��ռ���ȼ�2
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;        // �����ȼ�2
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // IRQͨ��ʹ��
    NVIC_Init(&NVIC_InitStructure);                           // ����ָ���Ĳ�����ʼ��VIC�Ĵ���

    USART_DeInit(USART1);//�����˼�ǽ���˴��ڵ���������
    USART_StructInit(&USART_InitStructure);
    USART_InitStructure.USART_BaudRate = bound; // һ������Ϊ9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &USART_InitStructure);

    USART_ClearFlag(USART3, USART_FLAG_TC | USART_FLAG_RXNE);  // ��� USART3 �ı�־//��ʼ����ʱ���������λ
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE); // �����ж�  ������ܵ�����
    USART_ITConfig(USART3, USART_IT_TC, DISABLE);
    USART_Cmd(USART3, ENABLE);                     // ʹ�ܴ���
}

void USART3_SendByte(uint8_t ch)
{

    USART_SendData(USART3, ch);
    /*������������������������������������TC*/
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);//TC ��־λ���� USART �ķ������ݼĴ�������λ�Ĵ�����Ϊ��ʱ��λ�ģ����ʾһ������������֡������ֹͣλ���Ѿ���ȫ���ͳ�ȥ
    //while(USART_GetFlagStatus(USART3,USART_FLAG_TC)==RESET);

}

//todo д���亯�� printf ʹ��ǰ��selected_USART=usart3_u;
/*�жϴ���������������
USART3_IRQHandler �� USART3 ���жϴ��������������κ����õ��жϱ�־λ������ʱ������
��� USART_IT_RXNE �жϱ����ã������յ�����ʱ��RXNE ��־λ�ᱻ���ã��Ӷ������жϴ�������
��ʹ USART_IT_TC �����ã�ֻҪ RXNE ��Ȼ��Ч���жϴ�������Ȼ�ᱻ���á�*/

void USART3_IRQHandler(void)
{
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
        // ��ȡ���յ�������
        uint8_t receivedByte;
        receivedByte = USART_ReceiveData(USART3);

        switch (packetState) {
        case 0: // �ȴ�ͷ
            if (receivedByte == HEAD) {
                packetState = 1;
                dataIndex = 0;
            }
            break;

        case 1: // ��������
            rxPacket.data[dataIndex++] = receivedByte;
            if (dataIndex >= RX_DATA_LEN) {
                packetState = 2;
            }
            break;

        case 2: // �ȴ�β
            if (receivedByte == TAIL) {
                rxPacket.received = 1;
                // ���ݰ�������ɣ����Դ���
                processPacket(&rxPacket);
            }
            packetState = 0; // �����Ƿ�ƥ�䣬������״̬
            break;
        }

        // ����жϱ�־
        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    }
}
void Serial_SendArr(uint8_t* Arr, uint8_t Length)
{
    selected_USART = usart3_u;
    for (uint8_t i = 0; i < Length; ++i)
    {
        USART3_SendByte(Arr[i]);
    }
}

void SendPacket(uint8_t* buffer)
{
    selected_USART = usart3_u;
    for (int i = 0; i < sizeof(BluetoothPacket); i++) {
        USART3_SendByte(buffer[i]);  // ���ֽڷ���
    }
}

void Send_Debug(void)
{
    selected_USART = usart3_u;
    USART3_SendByte(0xff);
    USART3_SendByte('h');
    USART3_SendByte('i');
    USART3_SendByte(0xFE);
    USART3_SendByte('\r');
    USART3_SendByte('\n');
}












