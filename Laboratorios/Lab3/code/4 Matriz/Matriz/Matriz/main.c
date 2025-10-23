/*
 * main.c
 *
 * Created: 10/23/2025 11:38:04 AM
 *  Author: hecto
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>

// USART
#define TX_BUF_SZ 256
#define TX_MASK   (TX_BUF_SZ - 1)
#define RX_BUF_SZ 256
#define RX_MASK   (RX_BUF_SZ - 1)
#define BAUD_RATE 9600

// USART
uint8_t tx_buf[TX_BUF_SZ];
uint8_t tx_head = 0, tx_tail = 0;
uint8_t rx_buf[RX_BUF_SZ];
uint8_t rx_head = 0, rx_tail = 0;

// ADC
uint16_t x_coord = 0;
uint16_t y_coord = 0;

uint32_t millis_counter = 0;

#define DEBOUNCE_MS 500
uint32_t debounce_on_at = 0;
uint8_t debounce_active= 0;

uint8_t usart_rx_available(void);
void UTOA(uint16_t value, char *buffer);

uint32_t millis_now(void);
void timer0_init(void);




void usart_init(void);
void adc_init(void);
	

// Leer adc
uint16_t adc_read(uint8_t channel);

uint8_t usart_write_try(uint8_t b); 
uint16_t usart_write_str(const char *s);
uint8_t usart_read_try(uint8_t *b);
uint8_t usart_read_str(char *dest, uint8_t max_len);

void startDebounceTimer(void);
void handleButtonChange(void);
void init_joystick_button(void);

void debouce_task(void);


// ------------------------------------------------------------------
// ISRs
// ------------------------------------------------------------------




ISR(TIMER0_OVF_vect){
	millis_counter++;
}

// Piano buttons
ISR(PCINT1_vect) {
	PORTB ^= (1<<PORTB5);
	PCICR &= ~(1<<PCIE1);
	
	debounce_on_at = millis_now() + DEBOUNCE_MS;
	debounce_active = 1;
}



// ------------------------------------------------------------------
// MAIN
// ------------------------------------------------------------------

int main(void) {
	adc_init();
	usart_init();
	timer0_init();
	DDRB |= (1<<PORTB5);
	init_joystick_button();
	sei();
	
	while (1) {
		debouce_task();
		x_coord = adc_read(2);
		// usart_write_str("\nX: ");
		// UTOA(x_coord, buffer); usart_write_str(buffer);
		
		y_coord = adc_read(3);
		// usart_write_str(" | Y: ");
		// UTOA(y_coord, buffer); usart_write_str(buffer);
		
		if (x_coord == 0) {
			usart_write_str("\nLEFT");
			_delay_ms(50);
		}
		if (x_coord == 1023) {
			usart_write_str("\nRIGHT");
			_delay_ms(50);
		}
		if (y_coord == 0) {
			usart_write_str("\nUP");
			_delay_ms(50);
		}
		if (y_coord == 1023) {
			usart_write_str("\nDOWN");
			_delay_ms(50);
		};
		
		
	}
}

// ------------------------------------------------------------------
// HELPERS
// ------------------------------------------------------------------

// Retorna la cantidad de elementos en el buffer de RX
uint8_t usart_rx_available(void) {
	return (uint8_t)((rx_head - rx_tail) & RX_MASK);
}

// Convierte un valor entero sin signo en un string
void UTOA(uint16_t value, char *buffer) {
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
	UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);   // RX interrupt
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);               // 8N1
}

void adc_init(void) {
	ADMUX  = (1 << REFS0);
	ADCSRA = (1 << ADEN)
	| (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler 128
}



// ------------------------------------------------------------------
// UTILITY
// ------------------------------------------------------------------



uint32_t millis_now(void) {
	uint32_t m;
	cli();     // disable interrupts
	m = millis_counter;
	sei();     // re-enable
	return m;
}


void timer0_init(void){
	TCCR0A = 0x00;
	TCCR0B |= 0b011;
	TIMSK0 |= (1<<TOIE0);
}




void debouce_task(void){
	if (debounce_active && (millis_now() >= debounce_on_at)){
		PCIFR |= (1<<PCIF1);
		PCICR |= (1<<PCIE1);
		debounce_active = 0;
	}
}



void init_joystick_button(void){
	DDRC &= ~(1<<PORTC4);
	PORTC |= (1<<PORTC4);
	PCICR |= (1<<PCIE1);
	PCMSK1 |= (1<<PCINT12);
}

// Leer adc
uint16_t adc_read(uint8_t channel) {
	ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC)); // Wait for conversion to finish
	return ADC;
}

// Escribir un byte al buffer de envio de USART
uint8_t usart_write_try(uint8_t b) {
	uint8_t next = (uint8_t)((tx_head + 1) & TX_MASK);
	if (next == tx_tail) return 0;               // full
	tx_buf[tx_head] = b;
	tx_head = next;
	UCSR0B |= (1 << UDRIE0);                       // kick the ISR
	return 1;
}

// Escribir un string entero al buffer de env?o de USART
uint16_t usart_write_str(const char *s) {
	uint16_t n = 0;
	while (*s && usart_write_try((uint8_t)*s++)) n++;
	return n;
}

// Leer byte del buffer de recepcion de usart
uint8_t usart_read_try(uint8_t *b) {
	if (rx_head == rx_tail) return 0;                 // empty
	*b = rx_buf[rx_tail];
	rx_tail = (uint8_t)((rx_tail + 1) & RX_MASK);
	return 1;
}

// Leer string del buffer de recepcion
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

void startDebounceTimer(void) {
	TCCR0A = 0x00;                 // Normal mode
	TCCR0B = (1 << CS02) | (1 << CS00); // clk/1024 prescaler
	TCNT0  = 0;                    // Reset counter
	TIMSK0 |= (1 << TOIE0);
	PCICR &= ~((1 << PCIE1));
}


// --------------------------------------------
// USART ISRs
// --------------------------------------------

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