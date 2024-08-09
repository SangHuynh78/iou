/*
 * CANSPI.h
 *
 * Created: 19/06/2024 9:45:31 AM
 *  Author: HTSANG
 */ 


#ifndef CANSPI_H_
#define CANSPI_H_

#include <stdbool.h>
#include <util/delay.h>
#include "MCP2515.h"

typedef union {
	struct {
		uint8_t idType;
		uint32_t id;
		uint8_t dlc;
		uint8_t data0;
		uint8_t data1;
		uint8_t data2;
		uint8_t data3;
		uint8_t data4;
		uint8_t data5;
		uint8_t data6;
		uint8_t data7;
	} frame;
	uint8_t array[14];
} uCAN_MSG;

#define dSTANDARD_CAN_MSG_ID_2_0B 1
#define dEXTENDED_CAN_MSG_ID_2_0B 2

void CAN_Init(void);

bool CANSPI_Initialize(void);
uint8_t CANSPI_Transmit(uCAN_MSG *tempCanMsg);
uint8_t CANSPI_Receive(uCAN_MSG *tempCanMsg);

#endif /* CANSPI_H_ */