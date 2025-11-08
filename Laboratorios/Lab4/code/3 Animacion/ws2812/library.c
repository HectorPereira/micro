

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#ifndef NUM_LEDS
#define NUM_LEDS 256
#endif

#ifndef LED_PORT
#define LED_PORT PORTD
#endif

#ifndef LED_DDR
#define LED_DDR  DDRD
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>



// ===============================================================
// INTERNAL BUFFER
// ===============================================================
uint8_t leds[NUM_LEDS * 3];  // GRB order per LED

// ===============================================================
// LOW LEVEL FUNCTIONS
// ===============================================================
static void send_bit(uint8_t bitVal);
static void send_byte(uint8_t byte);

// ===============================================================
// INITIALIZATION
// ===============================================================
void ws2812_init(void) {
	LED_DDR |= (1 << LED_PORT);  // set pin as output
}

// ===============================================================
// SEND ONE BIT (Timing ~800kHz @ 16MHz F_CPU)
// ===============================================================
static void send_bit(uint8_t bitVal) {
	if (bitVal) {
		PORTD |=  (1 << LED_PORT);
		asm volatile (
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t"
		);
		PORTD &= ~(1 << LED_PORT);
		asm volatile ("nop\n\t""nop\n\t""nop\n\t""nop\n\t");
		} else {
		PORTD |=  (1 << LED_PORT);
		asm volatile ("nop\n\t""nop\n\t""nop\n\t");
		PORTD &= ~(1 << LED_PORT);
		asm volatile (
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
		);
	}
}

// ===============================================================
// SEND ONE BYTE (8 bits MSB first)
// ===============================================================
static void send_byte(uint8_t byte) {
	for (uint8_t i = 0; i < 8; i++) {
		send_bit(byte & 0x80);
		byte <<= 1;
	}
}

// ===============================================================
// SEND ONE PIXEL (GRB order)
// ===============================================================
void ws2812_send_pixel(uint8_t r, uint8_t g, uint8_t b) {
	send_byte(g);
	send_byte(r);
	send_byte(b);
}

// ===============================================================
// SHOW: latch/reset signal (>50us)
// ===============================================================
void ws2812_show(void) {
	_delay_us(60);
}

// ===============================================================
// FILL ALL PIXELS WITH SAME COLOR
// ===============================================================
void ws2812_fill(uint8_t r, uint8_t g, uint8_t b, uint16_t n) {
	cli();
	for (uint16_t i = 0; i < n; i++) {
		ws2812_send_pixel(r, g, b);
	}
	sei();
	ws2812_show();
}

// ===============================================================
// SET A SINGLE PIXEL COLOR (store in RAM buffer)
// ===============================================================
void ws2812_set_pixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
	if (index >= NUM_LEDS) return;
	leds[index * 3 + 0] = g;
	leds[index * 3 + 1] = r;
	leds[index * 3 + 2] = b;
}

// ===============================================================
// SEND ALL PIXELS FROM BUFFER TO STRIP
// ===============================================================
void ws2812_show_all(void) {
	cli();
	for (uint16_t i = 0; i < NUM_LEDS; i++) {
		ws2812_send_pixel(leds[i * 3 + 1], leds[i * 3 + 0], leds[i * 3 + 2]);
	}
	sei();
	ws2812_show();
}

// ===============================================================
// CLEAR BUFFER AND TURN OFF ALL PIXELS
// ===============================================================
void ws2812_clear(void) {
	for (uint16_t i = 0; i < NUM_LEDS * 3; i++) leds[i] = 0;
	ws2812_show_all();
}

// ===============================================================
// TURN ON SPECIFIC LED AT COORDINATE (x, y) 8x8 matrix style
// ===============================================================
void turn_led(uint8_t led_x, uint8_t led_y) {
	uint8_t index = led_y * 8 + led_x;
	ws2812_clear();
	ws2812_set_pixel(index, 255, 0, 0);
	ws2812_show_all();
}
