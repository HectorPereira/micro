/*
Problema B
	Sistema de Selección de Colores con ATmega328P, Fotocelda, Tira de
	LEDs WS2812 y Servomotor

Descripción General del Proyecto
	El sistema consiste en un selector de colores basado en un microcontrolador
	ATmega328P, una fotocelda, una tira de LEDs WS2812 y un servomotor. La
	idea principal es detectar el color presente en una hoja de referencia utilizando
	la fotocelda, posicionar un servomotor en un ángulo determinado en función del
	color identificado y mostrar dicho color en la tira de LEDs WS2812.

Componentes Utilizados
	- Microcontrolador ATmega328P
	- Fotocelda (Sensor de Luz)
	- Tira de LEDs WS2812
	- Servomotor
	- Comunicación Serial

Comunicación Serial
	- Valor de la fotocelda
	- Color detectado
	- Valor del color establecido
	- Diferencia entre valor establecido y valor de lectura

Proceso de Funcionamiento
	El microcontrolador se encargará de leer los valores de la fotocelda (sensor de
	luz), realizar el procesamiento para identificar el color, controlar el servomotor
	para moverlo al ángulo correspondiente al color detectado y encender la tira de
	LEDs WS2812 mostrando el mismo color identificado.
	Este sistema se configurará para detectar un número limitado de colores (los
	presentes en la hoja de referencia). Cada color tendrá un valor ADC
	(Analog-to-Digital Conversion) preestablecido para asegurar la correcta
	identificación y su representación precisa en la tira de LEDs.

 */ 


// ------------------------------------------------------------------
// LIBRARIES
// ------------------------------------------------------------------

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>

// -----------------------------------------------------------------
// DEFINITIONS
// ------------------------------------------------------------------


// USART
#define TX_BUF_SZ 256
#define TX_MASK   (TX_BUF_SZ - 1)
#define RX_BUF_SZ 256
#define RX_MASK   (RX_BUF_SZ - 1)
#define BAUD_RATE 9600

#define RED   PORTB2
#define GREEN PORTB3
#define BLUE  PORTB4

#define NUM_COLORS (sizeof(color_refs)/sizeof(color_refs[0]))

// ------------------------------------------------------------------
// PROGRAM VARIABLES
// ------------------------------------------------------------------


// Color mapping
typedef struct {
	const char *name;
	uint16_t r, g, b;
} ColorRef;

const ColorRef color_refs[] = {
	{"MORADO",    309, 220, 375},
	{"ROJO",       370, 170, 372},
	{"AMARILLO",    322, 152, 164},
	{"VERDE",     414, 276, 221},
	{"AZUL CLARO", 223, 226, 181},
	{"VIOLETA",    345, 356, 434},
	{"BLANCO",    180, 180, 180},
};


// USART
uint8_t tx_buf[TX_BUF_SZ];
uint8_t tx_head = 0, tx_tail = 0;
uint8_t rx_buf[RX_BUF_SZ];
uint8_t rx_head = 0, rx_tail = 0;

// ADC
uint16_t adc_result = 0;
uint8_t  adc_done   = 0;

// RGB LED
uint8_t led_state = 0;
uint16_t adc_sample[] = {0,0,0};


// ------------------------------------------------------------------
// HELPERS
// ------------------------------------------------------------------


// Retorna la cantidad de elementos en el buffer de RX
uint8_t usart_rx_available(void) {
	return (uint8_t)((rx_head - rx_tail) & RX_MASK);
}

void UTOA(uint16_t value, char *buffer) { // <- String stored in buffer
	char temp[6];
	int i = 0, j = 0;

	if (value == 0) {
		buffer[0] = '0';
		buffer[1] = '\0';
		return;
	}

	// Convert digits to temp buffer (reversed)
	while (value > 0 && i < sizeof(temp) - 1) {
		temp[i++] = (value % 10) + '0';
		value /= 10;
	}

	// Reverse digits into final buffer
	while (i > 0) buffer[j++] = temp[--i];
	buffer[j] = '\0';
}


// ------------------------------------------------------------------
// INITIALIZERS
// ------------------------------------------------------------------


void usart_init(void) {
	const uint16_t ubrr = (16000000UL / (16UL * BAUD_RATE)) - 1;
	UBRR0H = ubrr >> 8;
	UBRR0L = ubrr;
	UCSR0A = 0;
	UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);   // <- RX interrupt
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);               // 8N1
}

void adc_init(void) {
	ADMUX  = (1 << REFS0);                        // AVcc ref, ADC0 input
	ADCSRA = (1 << ADEN)                          // Enable ADC
	| (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler 128
}

void rgb_init(void){
	DDRB |= (1<<RED) | (1<<GREEN) | (1<<BLUE);
}


void servo_init(void) {
	DDRB |= (1 << PORTB1); 

	TCCR1A = (1 << COM1A1) | (1 << WGM11);
	TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11); // 8

	ICR1 = 39999;   
}
// ------------------------------------------------------------------
// UTILITY
// ------------------------------------------------------------------


void servo_set_angle(uint8_t angle) {
	uint16_t pulse = 1000 + ((uint32_t)angle * 4000) / 180;
	OCR1A = pulse;
}

uint8_t usart_write_try(uint8_t b) {
	uint8_t next = (uint8_t)((tx_head + 1) & TX_MASK);
	if (next == tx_tail) return 0;               // full
	tx_buf[tx_head] = b;
	tx_head = next;
	UCSR0B |= (1 << UDRIE0);                       // kick the ISR
	return 1;
}

uint16_t usart_write_str(const char *s) {
	uint16_t n = 0;
	while (*s && usart_write_try((uint8_t)*s++)) n++;
	return n;
}

uint8_t usart_read_try(uint8_t *b) {
	if (rx_head == rx_tail) return 0;                 // empty
	*b = rx_buf[rx_tail];
	rx_tail = (uint8_t)((rx_tail + 1) & RX_MASK);
	return 1;
}

// Stores string in 'dest'
uint8_t usart_read_str(char *dest, uint8_t max_len) {
	uint8_t count = 0;
	while (usart_rx_available() && count < (max_len - 1)) {
		uint8_t c;
		usart_read_try(&c);
		if (c == '\n' || c == '\r') {
			break;
		}
		dest[count++] = c;
	}
	
	dest[count] = '\0';
	return count;
}


uint16_t adc_read(uint8_t channel) {
	ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);  // Select ADC channel 0–7
	ADCSRA |= (1 << ADSC);                      // Start conversion
	while (ADCSRA & (1 << ADSC));               // Wait for conversion to finish
	return ADC;                                 // Return 10-bit result
}

void rgb_set(uint8_t r, uint8_t g, uint8_t b) {
	PORTB = (PORTB & ~((1 << RED)|(1 << GREEN)|(1 << BLUE))) |
	((r<<RED) | (g<<GREEN) | (b<<BLUE));
}

const char* identify_color(uint16_t r, uint16_t g, uint16_t b) {
	uint32_t best_dist = 0xFFFFFFFF;
	const char *best_name = "UNKNOWN";

	for (uint8_t i = 0; i < NUM_COLORS; i++) {
		int32_t dr = (int32_t)r - color_refs[i].r;
		int32_t dg = (int32_t)g - color_refs[i].g;
		int32_t db = (int32_t)b - color_refs[i].b;
		uint32_t dist = dr*dr + dg*dg + db*db;

		if (dist < best_dist) {
			best_dist = dist;
			best_name = color_refs[i].name;
		}
	}
	return best_name;
}

void print_data(const char *color_name) {
	char buf[10];

	// Find by name
	uint16_t ref_r = 0, ref_g = 0, ref_b = 0;
	for (uint8_t i = 0; i < NUM_COLORS; i++) {
		if (strcmp(color_name, color_refs[i].name) == 0) {
			ref_r = color_refs[i].r;
			ref_g = color_refs[i].g;
			ref_b = color_refs[i].b;
			break;
		}
	}

	// Delta
	int16_t dR = (int16_t)adc_sample[0] - ref_r;
	int16_t dG = (int16_t)adc_sample[1] - ref_g;
	int16_t dB = (int16_t)adc_sample[2] - ref_b;

	// Printing
	usart_write_str("Fotocelda: ");
	UTOA(adc_sample[0], buf); usart_write_str(buf);
	usart_write_str(" ");
	UTOA(adc_sample[1], buf); usart_write_str(buf);
	usart_write_str(" ");
	UTOA(adc_sample[2], buf); usart_write_str(buf);

	usart_write_str("  Color detectado: ");
	usart_write_str(color_name);

	usart_write_str("  Valor establecido: [");
	UTOA(ref_r, buf); usart_write_str(buf); usart_write_str(",");
	UTOA(ref_g, buf); usart_write_str(buf); usart_write_str(",");
	UTOA(ref_b, buf); usart_write_str(buf); usart_write_str("]");

	usart_write_str("  Delta: [");
	if (dR >= 0) usart_write_str("+");
	else usart_write_str("-");
	UTOA((dR >= 0) ? dR : -dR, buf); usart_write_str(buf);
	usart_write_str(",");

	if (dG >= 0) usart_write_str("+");
	else usart_write_str("-");
	UTOA((dG >= 0) ? dG : -dG, buf); usart_write_str(buf);
	usart_write_str(",");

	if (dB >= 0) usart_write_str("+");
	else usart_write_str("-");
	UTOA((dB >= 0) ? dB : -dB, buf); usart_write_str(buf);
	usart_write_str("]\r\n");
}


void rgb_read(void){
	char buffer[8];
		
	switch(led_state){
		case 0:
		rgb_set(1,0,0);
		adc_sample[0] = adc_read(0); // Rojo

		UTOA(adc_sample[0], buffer);
		break;
		case 1:
		rgb_set(0,1,0);
		adc_sample[1] = adc_read(0); // Verde

		UTOA(adc_sample[1], buffer);
		break;
		case 2:
		rgb_set(0,0,1);
		adc_sample[2] = adc_read(0); // Azul

		UTOA(adc_sample[2], buffer);
		
		const char *color_name = identify_color(adc_sample[0], adc_sample[1], adc_sample[2]);
		print_data(color_name);
		
		if (strcmp(color_name, "ROJO") == 0) {
			servo_set_angle(0);
		}
		else if (strcmp(color_name, "AMARILLO") == 0) {
			servo_set_angle(60);
		}
		else if (strcmp(color_name, "VERDE") == 0) {
			servo_set_angle(120);
		}
		else if (strcmp(color_name, "AZUL CLARO") == 0) {
			servo_set_angle(180);
		}
	
		break;
	}

	led_state = (led_state+1) % 3;
	
}




// ------------------------------------------------------------------
// MAIN
// ------------------------------------------------------------------


int main(void) {
	usart_init();
	adc_init();
	rgb_init();
	servo_init();
	sei();
	
	while (1) {
		rgb_read();
		_delay_ms(50);

	
	}
}


// ------------------------------------------------------------------
// ISRs
// ------------------------------------------------------------------



ISR(USART_UDRE_vect) {
	if (tx_head == tx_tail) {
		UCSR0B &= (uint8_t)~(1<<UDRIE0);
		return;
	}
	UDR0 = tx_buf[tx_tail];
	tx_tail = (uint8_t)((tx_tail + 1) & TX_MASK);
}

ISR(USART_RX_vect) {
	uint8_t d = UDR0;
	uint8_t next = (uint8_t)((rx_head + 1) & RX_MASK);
	if (next != rx_tail) {
		rx_buf[rx_head] = d;
		rx_head = next;
	}
}
