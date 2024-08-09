// spi.c
//
// SPI master routines were pulled from the Atmel ATMega168 datasheet.



#include <avr/io.h>
#include "IOU_board.h"
#include "spi.h"

// Initialize the SPI as master
void SPI_Init(void)
{
	

	 	MCP2515_SS0_DDR |= (1 << MCP2515_SS0_PIN);
	 	MCP2515_SS0_PORT |= (1 << MCP2515_SS0_PIN);      //kh?i ??ng chân SS là output và xu?t 1 tr??c khi kh?i ??ng SPI
		   
	// make the MOSI, SCK, and SS pins outputs
	SPI_DDR |= ( 1 << SPI_MOSI ) | ( 1 << SPI_SCK ) ;

	// make sure the MISO pin is input
	SPI_DDR &= ~( 1 << SPI_MISO );

	// set up the SPI module: SPI enabled, MSB first, master mode,
	//  clock polarity and phase = 0, F_osc/16
	SPI_SPCR |= ( 1 << SPI_SPE ) | ( 1 << SPI_MSTR ) | ( 1 << SPI_SPR0 );
	


}

// Transfer a byte of data
uint8_t SPI_SendByte( uint8_t data )
{
	// Start transmission
	SPDR = data;
//	SPDR = 01;

	// Wait for the transmission to complete
	while (!(SPSR & (1 << SPIF)));

	// return the byte received from the slave
	return SPDR;
}


// Transfer a byte of data
uint8_t SPI_ReadByte( void )
{
	// Start transmission
	SPDR = 0xFF;

	// Wait for the transmission to complete
	spi_wait();

	// return the byte received from the slave
	return SPDR;
}

void SPI_RxBuffer(uint8_t *buffer, uint8_t length)
{
	uint8_t i;
	uint32_t timeout_counter;

	for (i = 0; i < length; i++) {
		// Send dummy data to initiate SPI clock
		SPDR = 0xFF;
		
		// Wait for reception complete with timeout
		timeout_counter = 0;
		while (!(SPSR & (1<<SPIF))) {
			_delay_us(1);
			timeout_counter++;
			if (timeout_counter > 10000) {
				// Handle timeout (e.g., set an error flag, break, etc.)
				// For now, we'll just break
				break;
			}
		}
		
		// Read received data
		buffer[i] = SPDR;
	}
}

void SPI_TxBuffer(uint8_t *buffer, uint8_t length)
{
	uint8_t i;
	uint32_t timeout_counter;

	for (i = 0; i < length; i++)
	{
		// Load data into the buffer
		SPDR = buffer[i];

		// Wait for transmission complete with timeout
		timeout_counter = 0;
		while (!(SPSR & (1<<SPIF)))
		{
			_delay_us(1);
			timeout_counter++;
			if (timeout_counter > 10000)
			{
				// Handle timeout (e.g., set an error flag, break, etc.)
				// For now, we'll just break
				break;
			}
		}
	}
}