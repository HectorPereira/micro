#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>


#define mp 10000
#define Pin_M PORTB1
#define max_degrees 4000
#define Min_degrees 2000



double dutyCycle = 0;




void uart_init(void) {
	// 9600 baud @ 16 MHz
	uint16_t ubrr = 103;  // from formula: UBRR = (F_CPU / (16 * BAUD)) - 1
	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)(ubrr);

	// Enable transmitter and receiver
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);

	// Frame format: 8 data bits, 1 stop bit, no parity
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_tx(char c) {
	// Wait for empty transmit buffer
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = c;
}

char uart_rx(void) {
	// Wait for data to be received
	while (!(UCSR0A & (1 << RXC0)));
	return UDR0;
}

void motor_init(void){
	DDRD |= (1<<PORTD6) | (1<<PORTD7);
}


//PWM para el control del motor paso a paso 
void init_pwm(void)
{
		DDRB |= (1 << Pin_M);

		TCCR1A = (1<<COM1A1) | (1<<WGM11);
		TCCR1B = (1<<WGM13) | (1<<WGM12) | (1<<CS11);


		ICR1 = 39999;   // TOP


		OCR1A = Min_degrees;  // ~1 ms ? servo en 0 grados
}


void left(){
	PORTD |= (1<<PORTD6);
	PORTD &= ~(1<<PORTD7);
}

void right(){
	PORTD |= (1<<PORTD7);
	PORTD &= ~(1<<PORTD6);
}

void forward(){
	PORTD |= (1<<PORTD7);
	PORTD |= (1<<PORTD6);
}

void stop(){
	PORTD &= ~(1<<PORTD7);
	PORTD &= ~(1<<PORTD6);
}

void kick(){
	OCR1A = 4000; // Medio para probar
	_delay_ms(500); // desues lo cambio probando con el servo
	OCR1A = 2000;
}

void uart_println(char c) {
	uart_tx(c);
	uart_tx('\r');
	uart_tx('\n');
}



int main(void) {
	uart_init();
	//motor_init();
	init_pwm();
	
	while (1) {
		// If data available (RX complete flag)
		if (UCSR0A & (1 << RXC0)) {
			char val = uart_rx();
			uart_println(val);
			if (val == 'F') forward();
			if (val == 'L') left();
			if (val == 'R') right();
			if (val == 'P') kick();
			if (val == '0') stop();
		}
	}
}
