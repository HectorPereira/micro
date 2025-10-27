/*

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

#define IN1 PD2    // H-bridge direction pin 1
#define IN2 PD3    // H-bridge direction pin 2

#define MIN_PWM 150
#define MAX_PWM 600
#define DEAD_ZONE 20

// ------------------------------------------------------------------
// PROGRAM VARIABLES
// ------------------------------------------------------------------



// USART
uint8_t tx_buf[TX_BUF_SZ];
uint8_t tx_head = 0, tx_tail = 0;
uint8_t rx_buf[RX_BUF_SZ];
uint8_t rx_head = 0, rx_tail = 0;

// ADC
uint16_t motor_pot = 0;
uint16_t reference_pot = 0;



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

// Leer adc
uint16_t adc_read(uint8_t channel) {
	ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);  
	ADCSRA |= (1 << ADSC);                     
	while (ADCSRA & (1 << ADSC));               // Wait for conversion to finish
	return ADC;                                 
}




void usart_task(void) {
	uint8_t direction = 0;
	motor_pot = adc_read(3);       // feedback
	reference_pot = adc_read(4);   // target

	int16_t error = (int16_t)reference_pot - (int16_t)motor_pot;
	uint16_t pwm_output = 0;

	// --- Direction + magnitude control ---
	if (error > DEAD_ZONE) {
		// Forward direction
		PORTD |=  (1 << IN1);
		PORTD &= ~(1 << IN2);
		
		direction = 1;

		pwm_output = error * 2;
		if (pwm_output < MIN_PWM) pwm_output = MIN_PWM;
		if (pwm_output > MAX_PWM) pwm_output = MAX_PWM;
		OCR1A = pwm_output;
	}
	else if (error < -DEAD_ZONE) {
		// Reverse direction
		PORTD |=  (1 << IN2);
		PORTD &= ~(1 << IN1);

		pwm_output = (-error) * 2;
		if (pwm_output < MIN_PWM) pwm_output = MIN_PWM;
		if (pwm_output > MAX_PWM) pwm_output = MAX_PWM;
		OCR1A = pwm_output;
	}
	else {
		// Stop (dead zone)
		PORTD &= ~((1 << IN1) | (1 << IN2));  // both low = brake (or coast)
		OCR1A = 0;
	}

	// --- Serial feedback ---
	char buffer[8];
	UTOA(reference_pot, buffer);
	usart_write_str(buffer);
	usart_write_str(" ");

	UTOA(motor_pot, buffer);
	usart_write_str(buffer);
	usart_write_str(" ");
	
	UTOA(pwm_output, buffer);
	usart_write_str(buffer);
	usart_write_str(" ");
	
	if (!pwm_output) usart_write_str("Stop\r\n");
	else if (!direction) usart_write_str("Izq.\r\n");
	else usart_write_str("Der.\r\n"); 
	
}



void setup_pwm_and_dir(void) {
	// Direction pins as outputs
	DDRD |= (1 << IN1) | (1 << IN2);

	// PWM pin PB1 (OC1A) as output
	DDRB |= (1 << PORTB1);

	// Fast PWM 10-bit, non-inverted, prescaler = 8
	TCCR1A = (1 << COM1A1)  | (1 << WGM10) | (1 << WGM11);
	TCCR1B = (1 << WGM12) | (1 << CS12); // clk/8
}







// ------------------------------------------------------------------
// MAIN
// ------------------------------------------------------------------

// programa principal
int main(void) {
	usart_init();
	adc_init();
	setup_pwm_and_dir();
	sei();
	
	while (1) {
		usart_task();
		_delay_ms(50);
	}
}


// ------------------------------------------------------------------
// ISRs
// ------------------------------------------------------------------


// Interrupcion de registro en enviado libre
ISR(USART_UDRE_vect) {
	if (tx_head == tx_tail) {
		UCSR0B &= (uint8_t)~(1<<UDRIE0);
		return;
	}
	UDR0 = tx_buf[tx_tail];
	tx_tail = (uint8_t)((tx_tail + 1) & TX_MASK);
}

// Interrupcion de byte recibido.
ISR(USART_RX_vect) {
	uint8_t d = UDR0;
	uint8_t next = (uint8_t)((rx_head + 1) & RX_MASK);
	if (next != rx_tail) {
		rx_buf[rx_head] = d;
		rx_head = next;
	}
}
