#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

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
	DDRD |= (1<<PORTD6) | (1<<PORTD7)| (1<<PORTD4) | (1<<PORTD5);
}


//PWM para el control del motor paso a paso 
void init_pwm(void)
{
		DDRB |= (1 << Pin_M);

		TCCR1A = (1<<COM1A1) | (1<<WGM11);
		TCCR1B = (1<<WGM13) | (1<<WGM12) | (1<<CS11);


		ICR1 = 39999;   // TOP


		OCR1A = 2000;  // ~1 ms ? servo en 0 grados
}


void left(){
	PORTD |= (1<<PORTD6);
	PORTD &= ~(1<<PORTD7);
	PORTD &= ~(1<<PORTD4);
	PORTD &= ~(1<<PORTD5);
}

void right(){
	PORTD |= (1<<PORTD7);
	PORTD &= ~(1<<PORTD6);
	PORTD &= ~(1<<PORTD4);
	PORTD &= ~(1<<PORTD5);
}

void forward(){
	PORTD |= (1<<PORTD7);
	PORTD |= (1<<PORTD6);
	PORTD &= ~(1<<PORTD4);
	PORTD &= ~(1<<PORTD5);
}

void BACK(){
	PORTD |= (1<<PORTD4);
	PORTD |= (1<<PORTD5);
	PORTD &= ~(1<<PORTD6);
	PORTD &= ~(1<<PORTD7);
}

void stop(){
	PORTD &= ~(1<<PORTD7);
	PORTD &= ~(1<<PORTD6);
	PORTD &= ~(1<<PORTD4);
	PORTD &= ~(1<<PORTD5);
}

void kick(){
	OCR1A = 2000; 
	_delay_ms(100); 
	OCR1A = 3000;
}

void kick1(){
	OCR1A = 4000; 
	_delay_ms(100); 
	OCR1A = 3000;
}




void uart_println(char c) {
	uart_tx(c);
	uart_tx('\r');
	uart_tx('\n');
}

void pcint_init(void)
{
    DDRD &= ~((1<<DDD2) | (1<<DDD3));  // PD2 y PD3 como ENTRADA
    PORTD |= (1<<PORTD2) | (1<<PORTD3); // Pull-up

    PCICR |= (1 << PCIE2);     // Habilitar PCINT para PORTD (PCIE2)
    PCMSK2 |= (1 << PCINT18) | (1 << PCINT19); // Habilitar PD2 y PD3
}

ISR(PCINT2_vect)
{
	char mensaje[] = "Fuera de la linea\r\n";

    // Cambio detectado en PD2
    if (PIND & (1 << PIND2)) {

        // Enviar el texto caracter por caracter
        for (uint8_t i = 0; mensaje[i] != '\0'; i++) {
            uart_println(mensaje[i]);
        }
    }

    // Cambio detectado en PD3
    if (PIND & (1 << PIND3)) {
        for (uint8_t i = 0; mensaje[i] != '\0'; i++) {
            uart_println(mensaje[i]);
        }
    }
}


int main(void) {
	uart_init();
	motor_init();
	init_pwm();
	
	while (1) {
		// If data available (RX complete flag)
		if (UCSR0A & (1 << RXC0)){
			char val = uart_rx();
			uart_println(val);
			if (val == 'F') forward();
			if (val == 'L') left();
			if (val == 'R') right();
			if (val == 'P') kick();
			if (val == 'S') kick1();
			if (val == 'B') BACK();

			if (val == '0') stop();
		}
	}
}
