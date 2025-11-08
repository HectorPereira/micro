#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>

// ------------------------------------------------------------------
// Configuración general
// ------------------------------------------------------------------

#define LED_PIN PORTD6
#define LED_DDR DDRD
#define NUM_LEDS 256

uint8_t leds[NUM_LEDS * 3];  // Datos GRB para cada LED

// ------------------------------------------------------------------
// Prototipos
// ------------------------------------------------------------------

void ws2812_init(void);
void ws2812_send_pixel(uint8_t r, uint8_t g, uint8_t b);
void ws2812_show(void);
void ws2812_fill(uint8_t r, uint8_t g, uint8_t b, uint16_t n);
void ws2812_set_pixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void ws2812_show_all(void);
void ws2812_clear(void);

void send_bit(uint8_t bitVal);
void send_byte(uint8_t byte);
void turn_led(uint8_t led_x, uint8_t led_y);

// ------------------------------------------------------------------
// MAIN
// ------------------------------------------------------------------
int main(void){
	ws2812_init();
	_delay_ms(1);     // estable
	ws2812_show();    // reset largo inicial

	ws2812_clear();
	for (uint8_t i=0; i<255; i++) ws2812_set_pixel(i, 0, 255, 0);
	ws2812_show_all();

	while(1){}
}

// ------------------------------------------------------------------
// FUNCIONES WS2812
// ------------------------------------------------------------------

void ws2812_init(void) {
	LED_DDR |= (1 << LED_PIN); // Configura pin de salida
}

void send_bit(uint8_t bitVal) {
	if (bitVal) {
		PORTD |= (1 << LED_PIN);
		asm volatile (
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t");
		PORTD &= ~(1 << LED_PIN);
		asm volatile ("nop\n\t""nop\n\t""nop\n\t""nop\n\t");
		} else {
		PORTD |= (1 << LED_PIN);
		asm volatile ("nop\n\t""nop\n\t""nop\n\t");
		PORTD &= ~(1 << LED_PIN);
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

void ws2812_send_pixel(uint8_t r, uint8_t g, uint8_t b) {
	send_byte(g);
	send_byte(r);
	send_byte(b);
}

void ws2812_show(void) {
	_delay_us(60);  // tiempo de reset (>50us)
}

void ws2812_fill(uint8_t r, uint8_t g, uint8_t b, uint16_t n) {
	cli();
	for (uint16_t i = 0; i < n; i++) {
		ws2812_send_pixel(r, g, b);
	}
	sei();
	ws2812_show();
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
	ws2812_show_all();
}

// Encender LED en coordenadas (x, y)
void turn_led(uint8_t led_x, uint8_t led_y) {
	uint8_t index = led_y * 8 + led_x;
	ws2812_clear();
	ws2812_set_pixel(index, 255, 0, 0); // rojo
	ws2812_show_all();
}

