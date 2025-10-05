#define F_CPU 16000000UL
#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

#define a 0b00110010   // Function set nibble
#define b 0b00101000   // 4-bit mode, 2-line, 5x8 font
#define c 0b00001100   // Display ON, cursor OFF
#define d 0b00000001   // Clear display

#define LCD_RS PB5
#define LCD_E  PB4

int LO_NIBBLE(int x) {
	return (int)(x & 0x0F);
}

int HI_NIBBLE(int x) {
	return (int)((x >> 4) & 0x0F);
}

void lcd_pulse(void) {
	PORTB |= (1 << LCD_E);
	_delay_us(1);
	PORTB &= ~(1 << LCD_E);
	_delay_us(100);
}

void setearLCD(int x) {
	PORTB &= ~(1 << LCD_RS);  // RS=0 → comando

	PORTB = (PORTB & 0xF0) | HI_NIBBLE(x);
	lcd_pulse();

	PORTB = (PORTB & 0xF0) | LO_NIBBLE(x);
	lcd_pulse();

	if (x == d) _delay_ms(2); 
}

void Escribir_LCD(int x) {
	PORTB |= (1 << LCD_RS);   // RS=1 → datos

	PORTB = (PORTB & 0xF0) | HI_NIBBLE(x);
	lcd_pulse();

	PORTB = (PORTB & 0xF0) | LO_NIBBLE(x);
	lcd_pulse();
}

char keypad[4][4] = {
	{ '1', '2', '3', 'A' },
	{ '4', '5', '6', 'B' },
	{ '7', '8', '9', 'C' },
	{ '*', '0', '#', 'D' }
};

int main(void) {
	DDRB |= 0b00111111;  // PB0–PB5 como salida
	PORTB &= ~(1 << LCD_RS);
	_delay_ms(50);       // esperar a que arranque la LCD

	setearLCD(a);
	setearLCD(b);
	setearLCD(c);
	setearLCD(d);

	Escribir_LCD('A');   // prueba con letra visible
	while (1);
}
