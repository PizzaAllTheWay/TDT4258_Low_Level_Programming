/*
 * usart.h
 *
 * Created: 08/11/2024 23:20:04
 *  Author: Martynas
 */ 


#ifndef USART_H_
#define USART_H_



#define USART3_F_CPU 4000000 // Microcontroller CPU Speed, required to calculate USART baud rate
#define USART3_BAUD_RATE(BAUD_RATE) ((float)(USART3_F_CPU * 64 / (16 *(float)BAUD_RATE)) + 0.5)



#include <avr/io.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>



void USART3_Init(void);
void USART3_SendChar(char c);
void USART3_SendString(char *str);
bool USART3_IsTxReady(void);
bool USART3_IsRxReady(void);
uint8_t USART3_Read();



#endif /* USART_H_ */