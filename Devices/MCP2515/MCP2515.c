/*
 * MCP2515.c
 *
 * Created: 18/06/2024 9:55:32 AM
 *  Author:
 */ 
#include "MCP2515.h"
#include <util/delay.h>


#define MCP2515_CS_HIGH()   MCP2515_CS_PORT |= (1 << MCP2515_CS_PIN)
#define MCP2515_CS_LOW()	MCP2515_CS_PORT &= ~(1 << MCP2515_CS_PIN)


bool MCP2515_SetNormalMode(void)
{
	/* configure CANCTRL Register */
	MCP2515_write(MCP2515_CANCTRL, 0x00);
	
	uint8_t loop = 10;
	
	do {
		/* confirm mode configuration */
		if((MCP2515_read(MCP2515_CANSTAT) & 0xE0) == 0x00)
		return true;
		
		loop--;
	} while(loop > 0);
	
	return false;
}

/* change mode as configuration mode */
bool MCP2515_SetConfigMode(void)
{
	/* configure CANCTRL Register */
	MCP2515_write(MCP2515_CANCTRL, 0x80);
	
	uint8_t loop = 10;
	
	do {
		/* confirm mode configuration */
		if((MCP2515_read(MCP2515_CANSTAT) & 0xE0) == 0x80)
		return true;
		
		loop--;
	} while(loop > 0);
	
	return false;
}

void MCP2515_reset(void)
{
	MCP2515_CS_LOW();
	SPI_SendByte(MCP2515_RESET);
	MCP2515_CS_HIGH();
}

/* read single byte */
uint8_t MCP2515_read(uint8_t address)
{
	uint8_t retVal;
	
	MCP2515_CS_LOW();
	
	SPI_SendByte(MCP2515_READ);
	SPI_SendByte(address);
	retVal = SPI_SendByte(0x00);
	
	MCP2515_CS_HIGH();
	
	return retVal;
}

/* read buffer */
void MCP2515_ReadRxSequence(uint8_t instruction, uint8_t *data, uint8_t length)
{
	MCP2515_CS_LOW();
	
	SPI_SendByte(instruction);
	SPI_RxBuffer(data, length);
	
	MCP2515_CS_HIGH();
}

/* write single byte */
void MCP2515_write(volatile uint8_t address, volatile uint8_t data)
{
	MCP2515_CS_LOW();
	SPI_SendByte(MCP2515_WRITE);
	SPI_SendByte(address);
	SPI_SendByte(data);
	MCP2515_CS_HIGH();
}


void MCP2515_WriteByteSequence(uint8_t startAddress, uint8_t endAddress, uint8_t *data)
{
	MCP2515_CS_LOW();
	
	SPI_SendByte(MCP2515_WRITE);
	SPI_SendByte(startAddress);
	SPI_TxBuffer(data, (endAddress - startAddress + 1));
	
	MCP2515_CS_HIGH();
}

/* write to TxBuffer(1 byte) */
void MCP2515_LoadTxBuffer(uint8_t instruction, uint8_t data)
{
	MCP2515_CS_LOW();
	
	SPI_SendByte(instruction);
	SPI_SendByte(data);
	
	MCP2515_CS_HIGH();
}

/* request to send */
void MCP2515_RequestToSend(uint8_t instruction)
{
	MCP2515_CS_LOW();
	
	SPI_SendByte(instruction);
	
	MCP2515_CS_HIGH();
}


uint8_t MCP2515_read_status(void)
{
	uint8_t result;
	MCP2515_CS_LOW();
	SPI_SendByte(MCP2515_READ_STATUS);
	result = SPI_SendByte(0x00); 
	MCP2515_CS_HIGH();
	return result;
}

/* read RX STATUS register */
uint8_t MCP2515_GetRxStatus(void)
{
	uint8_t retVal;
	
	MCP2515_CS_LOW();
	
	SPI_SendByte(MCP2515_RX_STATUS);
	retVal = SPI_ReadByte();
	
	MCP2515_CS_HIGH();
	
	return retVal;
}

/* Use when changing register value */
void MCP2515_BitModify(uint8_t address, uint8_t mask, uint8_t data)
{
	MCP2515_CS_LOW();
	
	SPI_SendByte(MCP2515_BIT_MOD);
	SPI_SendByte(address);
	SPI_SendByte(mask);
	SPI_SendByte(data);
	
	MCP2515_CS_HIGH();
}

/* write to TxBuffer */
void MCP2515_LoadTxSequence(uint8_t instruction, uint8_t *idReg, uint8_t dlc, uint8_t *data)
{
	MCP2515_CS_LOW();
	
	SPI_SendByte(instruction);
	SPI_TxBuffer(idReg, 4);
	SPI_SendByte(dlc);
	SPI_TxBuffer(data, dlc);
	
	MCP2515_CS_HIGH();
}