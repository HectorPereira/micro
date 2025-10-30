<<<<<<< HEAD
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/twi.h>
=======
/*

*/ 
>>>>>>> c7c0e8e6f3a1a91cc6868c95b11cd1bb24ae253d


#define BAUD 9600UL
#define UBRR_VALUE ((F_CPU/16/BAUD) - 1)
#define TX_BUFFER_SIZE 128
#define RX_BUFFER_SIZE 128
#define precarger 10000


void Init_pwm();
void Init_adc();


// macro para setear
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

// macro para resetear
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))

#define UART_TIMEOUT_MS 500


void spi_init(void);

uint8_t spi_transfer(uint8_t data);

void uart_init(unsigned int ubrr);

char uart_receive(void);

void uart_send(char c);

void uart_print(const char *s);

void uart_print_hex(uint16_t val);

void uart_print_hex_array(const uint8_t *arr, uint8_t len);

int main(void)
{
	Init_pwm();
	uart_init(UBRR_VALUE);
	Init_adc();
	
	
    while(1)
    {
        uint16_t valor = 1;
		//valor = adc_read_blocking_adif();
		uart_print_hex(valor);
    }
}

//PIN 6 with fast PWM
void Init_pwm(){
	// PIn 6
	DDRD |= (1 << DDD6);
	PORTD |= (0<<PORTD6);
	
	// Configuration PWM
	TCCR0A |= (1<<WGM01) |(1<<WGM00)|(1<<COM0A1)|(0<<COM0A0);
	// Modo fast pwm
	
	TCCR0B |= (0<<CS02) |(0<<CS01)|(1<<CS00);	
}

Init_adc(){
	ADMUX |= (1 << REFS0);
	ADCSRA = (1 << ADEN)| (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);   // ADPS=111 ? prescaler 128
	DIDR0  = (1 << ADC0D); // Desabilitado la entrada Digital
}


// Lectura  esperando ADIF=1 
uint16_t adc_read_blocking_adif(void) {
	ADCSRA |= (1 << ADIF);                       // Limpia flag previo
	ADCSRA |= (1 << ADSC);                       // Inicia conversión
	while (!(ADCSRA & (1 << ADIF))) { }          // Espera fin (ADIF=1)
	uint16_t v = ADC;                             // Lee resultado
	ADCSRA |= (1 << ADIF);                       // Limpia flag para la próxima
	return v;
}

void uart_init(unsigned int ubrr) {
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = (1<<TXEN0) | (1<<RXEN0);
	UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);
}

char uart_receive(void) {
	while (!(UCSR0A & (1<<RXC0)));
	return UDR0;
}

void uart_send(char c) {
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = c;
}

void uart_print(const char *s) {
	while (*s) uart_send(*s++);
}

void uart_print_hex(uint16_t val) {
	char buf[6];
	sprintf(buf, "0x%02X", val);
	uart_print(buf);
}

void uart_print_hex_array(const uint8_t *arr, uint8_t len) {
	for (uint8_t i = 0; i < len; i++) {
		uart_print_hex(arr[i]);
		uart_send(' ');
	}
	uart_print("\r\n");
}

void spi_init(void) {
	DDRB |= (1<<PORTB2)|(1<<PORTB3)|(1<<PORTB5); // SS, MOSI, SCK salidas
	DDRB &= ~(1<<PORTB4); // MISO entrada
	SPCR = (1<<SPE)|(1<<MSTR);
	SPSR = (1<<SPI2X); // fosc/8
}

uint8_t spi_transfer(uint8_t data) {
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));
	return SPDR;
}

