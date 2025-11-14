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
uint8_t I2C_Receive();
uint8_t byte = 0;
uint8_t status = 0;
uint8_t respond = 0;


#define D_Slave (0x50 << 1) //Direccion del esclavo
#define SCL PORTC0
#define SDA PORTC1



// ======================================
// ISR's
// ======================================

// I2C Interruption para gestionar la respuesta I2c del esclavo

ISR(TWI_vect) 
{
	uint8_t status = TWSR & 0xF8;

	switch(status)
	{
		case 0x60:  // SLA+W recibido
		break;

		case 0x80:  // Dato recibido
		byte = TWDR;   // guardo el dato
		break;

		case 0xA8:  // SLA+R recibido (maestro quiere leer)
		// Enviar NACK
		TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE);
		return;

		case 0xA0:  // STOP
		case 0xC0:  // NACK del maestro
		default:
		break;
	}

	// Para escritura sí queremos ACK
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA)|(1<<TWIE);
}

int main(void)
{
	sei();
	DDRD |= (1 << PORTD7);
			
	I2C_init();
	
    while(1)
    {
        if (byte == 0xF1)
        {
			PORTD |= (1 << PORTD7);
        }
		else if(byte == 0xF0){
			PORTD &= ~(1 << PORTD7);
			
		}
         
    }
}

void I2C_init(void) { // Cuando un maestro mande esta dirección, respondé con ACK
	DDRC |= (0 << PORTC1)|(0 << PORTC0); // Ponemos como entradas por las dudas 
	
	TWAR = D_Slave;
	TWCR = (1 << TWEN) | (1 << TWEA) | (1 << TWIE);
	
}

