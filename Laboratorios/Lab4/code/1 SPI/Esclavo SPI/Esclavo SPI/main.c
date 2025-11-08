#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <util/twi.h>

// ======================================
// Pines SPI
// ======================================

#define CS   PB2 //Direccion de este arduino
#define MOSI PB3
#define MISO PB4
#define SCK  PB5

// ======================================
// Protocolos SPI
// ======================================


void spi_init_slave();
uint8_t SPI_slaveReceive();
uint8_t SPI_slaveTransmit();



int main(void)
{
	DDRC |= (1 << PORTC0)| (1 << PORTC1)| (1 << PORTC2);
	PORTC &= ~(1 << PORTC0);
	PORTC &= ~(1 << PORTC1);
	
	sei();
	spi_init_slave();
	
    while(1)
    {
      uint8_t byte = SPI_slaveReceive();
	  if(byte == 0xF2){ // DHT  cambiar a pwm con ventilador
	  PORTC |= (1 << PORTC0);
	  }
	  else if(byte == 0xAA){
	  PORTC |= (1 << PORTC1);
	  }
	   else if(byte == 0xAB){
		   PORTC |= (1 << PORTC2);
	   }
    }
}

// ======================================
// Funciones SPI
// ======================================


void spi_init_slave(){
		DDRB |= (1 << MISO); // MISO salida
		DDRB &= ~(1 << MOSI); // MOSI entrada
		DDRB &= ~(1 << SCK); // Reloj de entrada, lo dicta el maestro	
		DDRB &= ~(1 << CS); // direccion habilitada por el maestro
		
		SPCR |= (1 << SPE); // Habilita SPI en modo maestro y no es necesario configurarle el prescaler, porque SCK se adapta al del maestro
}

uint8_t SPI_slaveReceive()
{
	// transmit dummy byte
	SPDR = 0xFF;

	// Wait for reception complete SPIF(SPI Interrupt Flag)
	while(!(SPSR & (1 << SPIF)));

	// return Data Register, when is read SPIF is clean
	return SPDR;
}



