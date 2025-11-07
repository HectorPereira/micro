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

#define CS   PB2 //SS del Arduino Slave
#define MOSI PB3
#define MISO PB4
#define SCK  PB5


// ======================================
// Protocolos SPI
// ======================================


void spi_init(void);
uint8_t spi_transfer(uint8_t data);
void SS_HIGH(void);
void SS_LOW(void);



int main(void)
{
    while(1)
    {
        //TODO:: Please write your application code 
    }
}



// ======================================
// FUnciones SPI
// ======================================


void spi_init(void) {
	DDRB |= (1 << CS) | (1 << MOSI) | (1 << SCK); // SS, MOSI, SCK salidas
	DDRB &= ~(1 << MISO);                         // MISO entrada

	SPCR = (1 << SPE) | (1 << MSTR)| (1 << SPR0); // Habilita SPI en modo maestro
	SPSR = (1 << SPI2X);              // fosc/8
}

uint8_t spi_transfer(uint8_t data) {
	SPDR = data;
	while (!(SPSR & (1 << SPIF)));
	return SPDR;
}

void SS_HIGH(void) { PORTB |=  (1 << CS); }
void SS_LOW(void)  { PORTB &= ~(1 << CS); }
