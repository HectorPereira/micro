
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


volatile const uint8_t LCD_PORTS[10] PROGMEM = {
	0x25, 0x25, 0x2b, 0x2b, 0x2b, 0x2b, 0x25, 0x25, 0x25, 0x25
};

volatile const uint8_t LCD_MASKS[10] PROGMEM = {
	PORTB4, PORTB5, PORTD4, PORTD5, PORTD6, PORTD7, PORTB0, PORTB1, PORTB2, PORTB3
};


char keypad[4][4] = {
	{'1', '2', '3', 'A'},
	{'4', '5', '6', 'B'},
	{'7', '8', '9', 'C'},
	{'*', '0', '#', 'D'}
};

static const uint16_t thresholds[16] = {
	88,149,206,265,327,377,432,487,
	544,599,646,702,754,804,854,904
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
	_delay_ms(2);
	MMIO8(0x25) |=  0b00100000;
	_delay_ms(2);
	MMIO8(0x25) &= ~0b00100000;
	_delay_ms(2);
	
}

void lcd_apply_sequence(uint16_t sequence) {
	for (uint8_t i = 0; i < 10; i++) {
		uint8_t port_addr = pgm_read_byte(&LCD_PORTS[i]);
		uint8_t bit_idx   = pgm_read_byte(&LCD_MASKS[i]);
		uint8_t mask      = (uint8_t)_BV(bit_idx);

		if (sequence & (0b1000000000 >> i)) {    
			MMIO8(port_addr) |=  mask;   
			} else {
			MMIO8(port_addr) &= (uint8_t)~mask;
		}
	}
	pulse_enable();
}


void lcd_init(void){
	lcd_set_outputs();
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

static inline char key_from_index(int idx){
	return keypad[idx/4][idx%4];
}

static inline void lcd_cmd(uint8_t cmd){
	lcd_apply_sequence(reverse8(cmd));
	_delay_ms(2);
}
static inline void lcd_clear(void){ lcd_cmd(0x01); } 

void adc_init(void){
	ADMUX  = (1<<REFS0);            
	ADCSRA = (1<<ADEN) | 0x07;      
}

uint16_t adc_read(void){
	ADCSRA |= (1<<ADSC);
	while (ADCSRA & (1<<ADSC));
	return ADC;
}


char read_key(void) {
	uint16_t v = adc_read();

	if (v > thresholds[15]) 
	return 0;

	uint8_t idx = 0;
	while (idx < 15 && v > thresholds[idx])
	idx++;

	return keypad[idx / 4][idx % 4];
}

int read_key_index(void) {
	uint16_t v = adc_read();

	if (v > thresholds[15]) 
	return -1;

	uint8_t idx = 0;
	while (idx < 15 && v > thresholds[idx])
	idx++;

	return idx;  
}


int main(void){
	lcd_init();
	adc_init();

	lcd_write_str("Tecla: ");
	char last = 0; 

	for(;;){
		int idx = read_key_index();        
		char k  = key_from_index(idx);     

		if (k != last){
			last = k;
			lcd_clear();                   
			lcd_write_str("Tecla: ");
			lcd_write((uint8_t)k);         
		}

		_delay_ms(20); 
	}
}