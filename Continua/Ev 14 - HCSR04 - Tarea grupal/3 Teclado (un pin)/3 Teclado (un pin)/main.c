
#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

#define MMIO8(addr) (*(volatile uint8_t *)(uint16_t)(addr))

#ifndef _BV
#define _BV(bit) (1U << (bit))
#endif


//RS E DB0 DB1 DB2 DB3 DB4 DB5 DB6 DB7


volatile const uint8_t LCD_PORTS[10] PROGMEM = {
	0x25, 0x25, 0x2b, 0x2b, 0x2b, 0x2b, 0x25, 0x25, 0x25, 0x25
};

volatile const uint8_t LCD_MASKS[10] PROGMEM = {
	PORTB4, PORTB5, PORTD4, PORTD5, PORTD6, PORTD7, PORTB0, PORTB1, PORTB2, PORTB3
};

static inline uint8_t reverse8(uint8_t x) {
	x = (x >> 4) | (x << 4);
	x = ((x & 0xCC) >> 2) | ((x & 0x33) << 2);
	x = ((x & 0xAA) >> 1) | ((x & 0x55) << 1);
	return x;
}

void lcd_set_outputs(void) {
	for (uint8_t i = 0; i < 10; i++) {
		uint8_t port_addr = pgm_read_byte(&LCD_PORTS[i]);
		uint8_t bit_idx   = pgm_read_byte(&LCD_MASKS[i]);
		uint8_t ddr_addr  = (uint8_t)(port_addr - 1);
		MMIO8(ddr_addr) |= _BV(bit_idx);
	}
}

void pulse_enable(void){
	_delay_ms(50);
	MMIO8(0x25) |=  0b00100000;
	_delay_ms(50);
	MMIO8(0x25) &= ~0b00100000;
	_delay_ms(50);
	
}

void lcd_apply_sequence(uint16_t sequence) {
	for (uint8_t i = 0; i < 10; i++) {
		uint8_t port_addr = pgm_read_byte(&LCD_PORTS[i]);
		uint8_t bit_idx   = pgm_read_byte(&LCD_MASKS[i]);
		uint8_t mask      = (uint8_t)_BV(bit_idx);

		if (sequence & (0b1000000000 >> i)) {     // LSB-first: i=0..9
			MMIO8(port_addr) |=  mask;    // drive high
			} else {
			MMIO8(port_addr) &= (uint8_t)~mask; // drive low
		}
	}
	pulse_enable();
}


void lcd_start_sequence(void){
	lcd_apply_sequence(0b0000001100);
	lcd_apply_sequence(0b0000011100);
	lcd_apply_sequence(0b0010000000);
	lcd_apply_sequence(0b0011110000);
}

void lcd_write(uint8_t character){
	lcd_apply_sequence((0b10 << 8) | reverse8(character));
}

void lcd_new_line(void){
	lcd_apply_sequence(0b0000000011);
}

void lcd_write_str(const char *s) {
	while (*s) {
		lcd_write((uint8_t)*s++);
	}
}


char keypad[4][4] = {
	{'1', '2', '3', 'A'},
	{'4', '5', '6', 'B'},
	{'7', '8', '9', 'C'},
	{'*', '0', '#', 'D'}
};

static const int thresholds[15] = {
	90,150,210,270,331,391,451,511,572,632,692,752,813,873,933
};

void adc_init(void){
	ADMUX  = (1<<REFS0);            
	ADCSRA = (1<<ADEN) | 0x07;      
}

uint16_t adc_read(void){
	ADCSRA |= (1<<ADSC);
	while (ADCSRA & (1<<ADSC));
	return ADC;
}

char read_key(void){
	uint16_t v = adc_read();
	int idx = 0;
	while (idx < 15 && v > thresholds[idx]) idx++;
	return keypad[idx];  
}

int main(void){
	adc_init();
	for(;;){
		char k = read_key();
		_delay_ms(20);
	}
}

