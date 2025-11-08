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

// ======================================
// PWM
// ======================================


void Init_pwm_D6(void);



int main(void)
{
	DDRC |= (1 << PORTC0)| (1 << PORTC1)| (1 << PORTC2)| (1 << PORTC3)| (1 << PORTC4)| (1 << PORTC5); 
	PORTC &= ~(1 << PORTC0);
	PORTC &= ~(1 << PORTC1);
	PORTC &= ~(1 << PORTC2);
	PORTC &= ~(1 << PORTC3);
	PORTC &= ~(1 << PORTC4);
	PORTC &= ~(1 << PORTC5);
	
	
	sei();
	spi_init_slave();
	Init_pwm_D6();
	
    while(1)
    {
      uint8_t byte = SPI_slaveReceive();
	  
	  if(byte == 0x00){
		PORTC |= (1 << PORTC0);
	  }
	  if(byte == 0x01){
		PORTC &= ~(1 << PORTC0);	
	  }
// 	  if(byte > 0 && byte <= 255){ // DHT  cambiar a pwm con ventilador
// 	  PORTC |= (1 << PORTC0);
// 	  OCR0A = byte;
// 	  }
// 	  else if(byte == 0xAA){
// 	  PORTC |= (1 << PORTC1);
// 	  }
// 	  else if(byte == 0xAB){
// 	  PORTC |= (1 << PORTC3);
// 	  
// 	  }
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

// ======================================
// Funciones para pwm
// ======================================


void Init_pwm_D6(void){
	DDRD |= (1 << DDD6);

	// Fast PWM (modo 3, TOP=255), salida no inversora en OC0A
	TCCR0A = (1 << WGM01) | (1 << WGM00) | (1 << COM0A1); // COM0A1=1, COM0A0=0
	TCCR0B = (1 << CS01) | (1 << CS00); // Prescaler = 64  (? 976 Hz @16MHz)
}

