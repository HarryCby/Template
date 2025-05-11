#include "usart.h"
#include "general_usart.h"
int fputc(int ch, FILE * file)
{
		USART_SendData(USART2, (uint8_t)ch);
		while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
		//USART_USED USART2 
		return ch;
}
//处理数据包
void processPacket(USART_RX_Packet *packet) {
	/*读取数据与处理
		u8 cnt_0=0,cnt_1=0,i;
		for(i=0;i<RX_DATA_LEN;i++){
			if(packet->data[i]==S_0){
				cnt_0++;
			}else if(packet->data[i]==S_1){
				cnt_1++;
			}
		}
		//start_sign_0?start_sign_1
		if(cnt_0>=3&&!(start_sign_0||start_sign_1)){
			start_sign_0=True;
		}else if(cnt_1>=3&&!(start_sign_0||start_sign_1)){
			start_sign_1=True;
		}
	*/

    packet->received = 0;
}
