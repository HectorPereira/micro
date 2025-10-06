/*
 * main.c
 *
 * Created: 10/6/2025 7:11:20 PM
 *  Author: isacm
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define F_CPU 16000000UL    // Frecuencia del reloj del micro (16 MHz)
#define BAUD 9600           // Velocidad de transmisión (baudios)
#define BRC ((F_CPU / 16 / BAUD) - 1)   // Valor para UBRR
#define TX_BUFFER_SIZE 128

char    serialBuffer[TX_BUFFER_SIZE];
uint8_t serialReadPos  = 0;
uint8_t serialWritePos = 0;


void appendSerial(char c);

int main(void)
{
	
	UBRR0H = (BRC >> 8);
	UBRR0L = BRC;


	// Habilitar transmisor
	UCSR0B = (1 << TXEN0) | (1 << TXCIE0);

	// Modo asincrónico, 8 bits, 1 stop, sin paridad
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
	
	sei();
	
	serialWrite('Hola');
	
	_delay_ms(5);
	
    while(1)
    {
    }
}


void appendSerial(char c)
{
	serialBuffer[serialWritePos] = c;
	serialWritePos++;

	if (serialWritePos >= TX_BUFFER_SIZE) {
		serialWritePos = 0;   // wrap-around
	}
}




void serialWrite(char c[])
{
	for (uint8_t i = 0; i < strlen(c); i++)
	{
		appendSerial(c[i]);
	}

	if (UCSR0A & (1 << UDRE0))
	{
		UDR0 = 0;  // Envía el primer byte si el registro está libre
	}
}


ISR(USART_TX_vect)
{
	if (serialReadPos != serialWritePos)
	{
		UDR0 = serialBuffer[serialReadPos];
		serialReadPos++;

		if (serialReadPos >= TX_BUFFER_SIZE)
		{
			serialReadPos = 0;  // Reinicia al inicio del buffer
		}
	}
}


