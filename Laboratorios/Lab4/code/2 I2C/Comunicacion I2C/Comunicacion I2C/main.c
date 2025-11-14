#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <util/twi.h>


void Init_I2C();
uint8_t Confirmation = 0;


int main(void)
{
    while(1)
    {
        //TODO:: Please write your application code 
    }
}

// ======================================
// FUnciones I2c
// ======================================

void I2C_init(void) {
	TWSR = 0x00;            // Prescaler = 1
	TWBR = ((F_CPU / 100000UL) - 16) / 2;  // 100 kHz
	TWCR = (1<<TWEN);       // Habilitar TWI
}

void I2C_start(void) {
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN); // TWI Interrupt Flag / TWI START Condition Bit / TWI Interrupt Enable /TWEA: TWI Enable Acknowledge Bit
	while(!(TWCR & (1<<TWINT))); // Espera a que salte TWINT
}

void I2C_stop(void) {
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN); // TWSTO STOP Condition Bit
}

uint8_t I2C_write(uint8_t v) {
	TWDR = v;
	TWCR = (1<<TWINT)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
	Confirmation = TWSR & 0b11111000;
	
	return (Confirmation == 0x28);
}
