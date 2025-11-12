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


void Init_pwm(void);




int main(void)
{
	DDRC |= (1 << PORTC0)| (1 << PORTC1)| (1 << PORTC2)| (1 << PORTC3)| (1 << PORTC4)| (1 << PORTC5); 
	PORTC &= ~(1 << PORTC0);
	PORTC &= ~(1 << PORTC1);
	PORTC &= ~(1 << PORTC2);
	PORTC &= ~(1 << PORTC3);
	PORTC &= ~(1 << PORTC4);
	PORTC &= ~(1 << PORTC5);
	DDRD |= (1 << PORTD2)|(1 << PORTD3)|(1 << PORTD4)|(1 << PORTD5);
	//PORTD |= (1 << PORTD3)|(1 << PORTD4); //|(1 << PORTD5);
	sei();
	spi_init_slave();
	Init_pwm();
	
	// Peueba despues va en 0x0A
	 const uint16_t TMIN = 100;  // Medido
	 const uint16_t TMAX = 620;  // Medido

	
	 
	
	 
    while(1)
    {
      uint8_t byte = SPI_slaveReceive();
	  
	  if(byte == 0x00){
		PORTD |= (1 << PORTD2);
	  }
	  else if(byte == 0x01){
		PORTD &= ~(1 << PORTD2);	
	  }
	  
	  else if(byte == 0xFA){
	 while(1){
		uint16_t deg = SPI_slaveReceive();
		uint16_t ticks = TMIN + ( (uint32_t)(TMAX - TMIN) * deg ) / 180;
		OCR1A = ticks;
		
	  }
	  }
	  
	  
	  
	  if(byte == 0xAA){
		 while(1){
		  uint8_t byte_rgb = SPI_slaveReceive();
		  switch (byte_rgb) {

			case 0x01: // ?? Rojo
			PORTD |=  (1 << PD3);   // R ON
			PORTD &= ~(1 << PD4);   // G OFF
			PORTD &= ~(1 << PD5);   // B OFF
			break;

			case 0x02: // ?? Naranja (R+G)
			PORTD |=  (1 << PD3);
			PORTD |=  (1 << PD4);
			PORTD &= ~(1 << PD5);
			break;

			case 0x03: // ?? Amarillo (igual que naranja)
			PORTD |=  (1 << PD3);
			PORTD |=  (1 << PD4);
			PORTD &= ~(1 << PD5);
			break;

			case 0x04: // ?? Verde
			PORTD &= ~(1 << PD3);
			PORTD |=  (1 << PD4);
			PORTD &= ~(1 << PD5);
			break;

			case 0x05: // ?? Celeste (G+B)
			PORTD &= ~(1 << PD3);
			PORTD |=  (1 << PD4);
			PORTD |=  (1 << PD5);
			break;

			case 0x06: // ?? Azul
			PORTD &= ~(1 << PD3);
			PORTD &= ~(1 << PD4);
			PORTD |=  (1 << PD5);
			break;

			default: // ? Apagado
			PORTD &= ~((1 << PD3) | (1 << PD4) | (1 << PD5));
			break;
			}
		 }
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


void Init_pwm(void){
	DDRD |= (1 << PORTD6);

	// Fast PWM (modo 3, TOP=255), salida no inversora en OC0A
	TCCR0A = (1 << WGM01) | (1 << WGM00) | (1 << COM0A1); // COM0A1=1, COM0A0=0
	TCCR0B = (1 << CS01) | (1 << CS00); // Prescaler = 64  (? 976 Hz @16MHz)
	
	// --- Servo PWM on PB1 (OC1A)
	// PB1 (OC1A)
	DDRB |= (1 << DDB1);

	TCCR1A = (1 << WGM11);
	TCCR1B = (1 << WGM13) | (1 << WGM12);

		
	TCCR1A |= (1 << COM1A1);
	TCCR1B |= (1 << CS11) | (1 << CS10);
	ICR1 = 4999;
	OCR1A = 90;  // 90 its 0 degrees and 650 180 , 560 ticks. in 0-255: 2,19 ticks for number
}

