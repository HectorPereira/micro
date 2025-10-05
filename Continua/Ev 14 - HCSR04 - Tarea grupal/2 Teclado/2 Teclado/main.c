#define F_CPU 16000000UL
#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define a 0b00110010   // Function set nibble
#define b 0b00101000   // 4-bit mode, 2-line, 5x8 font
#define c 0b00001100   // Display ON, cursor OFF
#define d 0b00000001   // Clear display

#define LCD_RS PB5
#define LCD_E  PB4

// ---- Bandera global ----
volatile uint8_t boton_presionado = 0;

// ---- Funciones auxiliares LCD ----
static inline uint8_t LO_NIBBLE(uint8_t x) { return x & 0x0F; }
static inline uint8_t HI_NIBBLE(uint8_t x) { return (x >> 4) & 0x0F; }

void lcd_pulse(void) {
	PORTB |= (1 << LCD_E);
	_delay_us(1);
	PORTB &= ~(1 << LCD_E);
	_delay_us(50);
}

void setearLCD(uint8_t x) {
	PORTB &= ~(1 << LCD_RS);  // RS=0 → comando
	PORTB = (PORTB & 0xF0) | HI_NIBBLE(x);
	lcd_pulse();
	PORTB = (PORTB & 0xF0) | LO_NIBBLE(x);
	lcd_pulse();
	if (x == d) _delay_ms(2);
}

void Escribir_LCD(uint8_t x) {
	PORTB |= (1 << LCD_RS);   // RS=1 → datos
	PORTB = (PORTB & 0xF0) | HI_NIBBLE(x);
	lcd_pulse();
	PORTB = (PORTB & 0xF0) | LO_NIBBLE(x);
	lcd_pulse();
}

// ---- Interrupción por cambio de pin ----
ISR(PCINT2_vect) {
	if (!(PIND & (1 << PD0))) {
		boton_presionado = 1;   // Marca el evento
	}
}

int main(void) {
	DDRB |= 0b00111111;  // PB0–PB5 salida
	DDRD &= ~(1 << PD0); // PD0 entrada
	PORTD |= (1 << PD0); // Pull-up interno

	// Habilitar interrupción PCINT16 (PD0)
	PCICR |= (1 << PCIE2);
	PCMSK2 |= (1 << PCINT16);
	sei();

	// Inicialización LCD
	_delay_ms(50);
	setearLCD(a);
	setearLCD(b);
	setearLCD(c);
	setearLCD(d);

	Escribir_LCD('B');

	while (1) {
		if (boton_presionado) {
			boton_presionado = 0;  // Limpia la bandera
			Escribir_LCD('C');     // Acción del botón
			_delay_ms(200);        // Anti-rebote
		}
	}
}

