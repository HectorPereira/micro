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
uint8_t LO_NIBBLE(uint8_t x) { return x & 0x0F; }
uint8_t HI_NIBBLE(uint8_t x) { return (x >> 4) & 0x0F; }

void lcd_pulse(void) {
	PORTB |= (1 << LCD_E);
	_delay_us(1);
	PORTB &= ~(1 << LCD_E);
	_delay_us(50);
}

void setearLCD(uint8_t x) {
	PORTB &= ~(1 << LCD_RS);  // RS=0 ? comando
	PORTB = (PORTB & 0xF0) | HI_NIBBLE(x);
	lcd_pulse();
	PORTB = (PORTB & 0xF0) | LO_NIBBLE(x);
	lcd_pulse();
	if (x == d) _delay_ms(2);
}

void Escribir_LCD(uint8_t x) {
	
	PORTB |= (1 << LCD_RS);   // RS=1 ? datos
	PORTB = (PORTB & 0xF0) | HI_NIBBLE(x);
	lcd_pulse();
	PORTB = (PORTB & 0xF0) | LO_NIBBLE(x);
	lcd_pulse();
}


char keypad[4][4] = {
	{'1', '2', '3', 'A'},
	{'4', '5', '6', 'B'},
	{'7', '8', '9', 'C'},
	{'*', '0', '#', 'D'}
};

char read_keypad(void) {
	for (uint8_t row = 0; row < 4; row++) {
		PORTD = ~(1 << row);  //Pone a 0 la fila individual
		_delay_us(5);

		for (uint8_t col = 0; col < 4; col++) {
			if (!(PINC & (1 << col))) {  // Lee columna de PORTC si esta en 0
				return keypad[row][col];
			}
		}
	}
	return 0;
}


int main(void) {
	DDRB |= 0b00111111;  // PB0–PB5 salida
	DDRD = 0x0F; // PD0 Salida 
	DDRC = 0x00; //	PC0 Entrada
	PORTC = 0x0F;
	
	
	// Habilitar interrupción 
	PCICR |= (1 << PCIE2);
	PCMSK2 |= (1 << PCINT16);
	sei();

	// Inicialización LCD
	_delay_ms(50);
	setearLCD(a);
	setearLCD(b);
	setearLCD(c);
	setearLCD(d);


	char key;
	while (1) {
		Escribir_LCD('T');
		Escribir_LCD('e');
		Escribir_LCD('c');		 
		Escribir_LCD('l');
		Escribir_LCD('a');
		Escribir_LCD(':');
		Escribir_LCD(' ');
		
		
		key = read_keypad();
		if (boton_presionado) {
			Escribir_LCD(key);
			_delay_ms(500);
			setearLCD(d);
		}
	}
}

// ---- Interrupción por cambio de pin ----
ISR(PCINT2_vect) {
		boton_presionado = 1;   // Marca el evento
}
