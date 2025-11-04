// Reutilizando el codigo de USART del laboratorio 2
// y los tiempos con asm volatile para la matriz de leds


#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>


// ------------------------------------------------------------------
// Definiciones y bariables
// ------------------------------------------------------------------

// USART
#define TX_BUF_SZ 256
#define TX_MASK   (TX_BUF_SZ - 1)
#define RX_BUF_SZ 256
#define RX_MASK   (RX_BUF_SZ - 1)
#define BAUD_RATE 9600

#define LED_PIN PORTD6
#define LED_DDR DDRD

#define DEBOUNCE_MS 300

#define NUM_LEDS 64
uint8_t leds[NUM_LEDS * 3];  // GRB data

// USART
uint8_t tx_buf[TX_BUF_SZ];
uint8_t tx_head = 0, tx_tail = 0;
uint8_t rx_buf[RX_BUF_SZ];
uint8_t rx_head = 0, rx_tail = 0;

// ADC
uint16_t x_coord = 0;
uint16_t y_coord = 0;
uint8_t x_pos = 4;
uint8_t y_pos = 4;

uint8_t red = 100;
uint8_t green = 100;
uint8_t blue = 100;

uint32_t millis_counter = 0;

uint32_t debounce_on_at = 0;
uint8_t debounce_active= 0;




// ------------------------------------------------------------------
// Prototipos
// ------------------------------------------------------------------


void UTOA(uint16_t value, char *buffer);

uint32_t millis_now(void);
void timer0_init(void);

void adc_init(void);
	

// Leer adc
uint16_t adc_read(uint8_t channel);

void startDebounceTimer(void);
void handleButtonChange(void);
void init_joystick_button(void);

void debouce_task(void);

void send_bit(uint8_t bitVal);
void send_byte(uint8_t byte);
void ws2812_send_pixel(uint8_t r, uint8_t g, uint8_t b);
void ws2812_show(void);
void ws2812_fill(uint8_t r, uint8_t g, uint8_t b, uint16_t n);

void ws2812_init(void);

void turn_led(uint8_t led_x, uint8_t led_y);
void ws2812_set_pixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void ws2812_show_all(void);
void ws2812_clear(void);


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
	
	red = rand() % 256;  // random 0�255
	green = rand() % 256;  // random 0�255
	blue = rand() % 256;  // random 0�255
	
	debounce_on_at = millis_now() + DEBOUNCE_MS;
	debounce_active = 1;
}





// ------------------------------------------------------------------
// MAIN
// ------------------------------------------------------------------

int main(void) {
	adc_init();
	timer0_init();
	ws2812_init();
	DDRB |= (1<<PORTB5);
	init_joystick_button();
	sei();
		
	
	while (1) {
		debouce_task();
		x_coord = adc_read(2);

		
		y_coord = adc_read(3);

		
		if (x_coord == 0) {
			x_pos = (x_pos > 0) ? x_pos - 1 : 0;    // clamp en 0
			_delay_ms(50);
		}
		if (x_coord == 1023) {
			x_pos = (x_pos < 7) ? x_pos + 1 : 7;    // clamp en 7
			_delay_ms(50);
		}
		if (y_coord == 0) {
			y_pos = (y_pos > 0) ? y_pos - 1 : 0;    // clamp en 0
			_delay_ms(50);
		}
		if (y_coord == 1023) {
			y_pos = (y_pos < 7) ? y_pos + 1 : 7;    // clamp en 7
			_delay_ms(50);
		};
		turn_led(x_pos, y_pos);
		_delay_ms(50);
			
	}
}




void turn_led(uint8_t led_x, uint8_t led_y) {
	uint8_t index = led_y * 8 + led_x;  
	ws2812_clear();                     
	ws2812_set_pixel(index, red, green, blue);  
	ws2812_show_all();                   
}

void ws2812_set_pixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
	if (index >= NUM_LEDS) return;
	leds[index * 3 + 0] = g;  
	leds[index * 3 + 1] = r;
	leds[index * 3 + 2] = b;
}

void ws2812_show_all(void) {
	cli();
	for (uint16_t i = 0; i < NUM_LEDS; i++) {
		ws2812_send_pixel(leds[i * 3 + 1], leds[i * 3 + 0], leds[i * 3 + 2]);
	}
	sei();
	ws2812_show();   
}

void ws2812_clear(void) {
	for (uint16_t i = 0; i < NUM_LEDS * 3; i++) leds[i] = 0;
}

// ------------------------------------------------------------------
// HELPERS
// ------------------------------------------------------------------


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

void adc_init(void) {
	ADMUX  = (1 << REFS0);
	ADCSRA = (1 << ADEN)
	| (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler 128
}



// ------------------------------------------------------------------
// UTILITY
// ------------------------------------------------------------------




void send_bit(uint8_t bitVal){
	if(bitVal){
		PORTD |=  (1<<LED_PIN);
		asm volatile (
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t");
		
		PORTD &= ~(1<<LED_PIN);
		
		asm volatile (
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t");
		
		} else {
		PORTD |=  (1<<LED_PIN);
		asm volatile (
		"nop\n\t""nop\n\t""nop\n\t");
		
		PORTD &= ~(1<<LED_PIN);
		asm volatile (
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
	}
}

void send_byte(uint8_t byte) {
	cli();
	for (uint8_t i = 0; i < 8; i++) {
		send_bit(byte & 0x80);  
		byte <<= 1;             
	}
	sei();  
}

// Enviar pixel
void ws2812_send_pixel(uint8_t r, uint8_t g, uint8_t b) {
	send_byte(g);
	send_byte(r);
	send_byte(b);
}

void ws2812_show(void) {
	_delay_us(60);  // Tiempo de reset
}

// Encender n leds del mismo color
void ws2812_fill(uint8_t r, uint8_t g, uint8_t b, uint16_t n) {
	cli();
	for (uint16_t i = 0; i < n; i++) {
		ws2812_send_pixel(r, g, b);
	}
	sei();
	ws2812_show();
}

void ws2812_init(void) { // tira de leds
	LED_DDR |= (1 << LED_PIN);
}







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


void startDebounceTimer(void) {
	TCCR0A = 0x00;                 // Normal mode
	TCCR0B = (1 << CS02) | (1 << CS00); // clk/1024 prescaler
	TCNT0  = 0;                    // Reset counter
	TIMSK0 |= (1 << TOIE0);
	PCICR &= ~((1 << PCIE1));
}


