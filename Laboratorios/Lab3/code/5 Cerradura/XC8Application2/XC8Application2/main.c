#define F_CPU 16000000UL
#include <avr/io.h>

#define BAUD     9600UL
#define UBRR_VAL ((F_CPU/16/BAUD)-1)   // para modo normal (U2X0=0)

static void usart_init(void) {
	// Baudrate
	UBRR0H = (uint8_t)(UBRR_VAL >> 8);
	UBRR0L = (uint8_t)(UBRR_VAL & 0xFF);

	UCSR0A = 0;                         // modo normal (U2X0=0)
	UCSR0B = (1 << TXEN0);              // habilitar TX
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8N1: 8 bits, sin paridad, 1 stop
}

static void usart_putc(char c) {
	while (!(UCSR0A & (1 << UDRE0))) {}
	UDR0 = c;
}

int main(void) {
	usart_init();
	
	while (1)
	{
		usart_putc('A');   // envía un solo carácter
	}
	
}
