#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#define LED_PIN  PD6
#define LED_PORT PORTD
#define LED_DDR  DDRD

#define NUM_LEDS 200

// ---------- Low-Level Timings ----------
static inline void sendBit(uint8_t bit) {
	if (bit) {
		LED_PORT |=  (1 << LED_PIN);
		_delay_us(0.7);   // '1' high
		LED_PORT &= ~(1 << LED_PIN);
		_delay_us(0.55);  // '1' low
		} else {
		LED_PORT |=  (1 << LED_PIN);
		_delay_us(0.35);  // '0' high
		LED_PORT &= ~(1 << LED_PIN);
		_delay_us(0.80);  // '0' low
	}
}

void sendByte(uint8_t byte) {
	for (int8_t i = 7; i >= 0; i--) {
		sendBit(byte & (1 << i));
	}
}

void sendColor(uint8_t g, uint8_t r, uint8_t b) {
	sendByte(g);
	sendByte(r);
	sendByte(b);
}

void ws2812_reset(void) {
	_delay_us(80);  // latch/reset
}

// ---------- Strip Control ----------
void fillStrip(uint8_t r, uint8_t g, uint8_t b) {
	for (uint16_t i = 0; i < NUM_LEDS; i++) {
		sendColor(g, r, b);  // GRB order
	}
	ws2812_reset();
}

// ---------- Main ----------
int main(void) {
	LED_DDR |= (1 << LED_PIN);  // Data pin as output

	while (1) {
		fillStrip(255, 0, 0);    // Red
		_delay_ms(1000);

		fillStrip(0, 255, 0);    // Green
		_delay_ms(1000);

		fillStrip(0, 0, 255);    // Blue
		_delay_ms(1000);

		fillStrip(255, 255, 255); // White
		_delay_ms(1000);

		fillStrip(0, 0, 0);      // Off
		_delay_ms(500);
	}
}
