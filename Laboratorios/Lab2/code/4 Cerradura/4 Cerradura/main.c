/*
Funcionalidades detalladas:

1. Bienvenida y Menú Interactivo:
	-	Al encender el sistema, la pantalla LCD muestra un mensaje de
		bienvenida e instrucciones para ingresar la contraseña.
	-	El menú también da la opción de cambiar la contraseña si el
		usuario lo desea.

2. Ingreso de Contraseña:
	-	El usuario ingresa la contraseña mediante el teclado matricial.
	-	La contraseña es comparada con la almacenada en la EEPROM.
	-	Si es correcta, se enciende el LED verde.
	-	Si es incorrecta, se enciende el LED rojo. Después de 3 intentos
		fallidos, se activa la alarma (buzzer).

3. Almacenamiento en EEPROM:
	-	La contraseña se guarda en la EEPROM para que esté disponible
		después de un reinicio o apagado del sistema.
	-	El microcontrolador puede leer la contraseña desde la EEPROM y
		comparar los valores con lo que el usuario ingresa.

4. Cambio de Contraseña:
	-	El sistema permite al usuario cambiar la contraseña ingresando
		primero la contraseña actual.
	-	Luego, el usuario puede elegir una nueva contraseña de entre 4 y
		6 dígitos, que será almacenada en la EEPROM.
		
Requerimientos:
	-	Microcontrolador: ATmega328P
	-	Teclado Matricial: 4x4 (16 teclas)
	-	Pantalla LCD: 16x2 o similar
	-	LED Verde: Indica éxito (contraseña correcta)
	-	LED Rojo: Indica error (contraseña incorrecta)
	-	Alarma: Buzzer o LED de advertencia
	-	EEPROM: Almacena la contraseña de manera persistente
	
 */ 


#define F_CPU 16000000
#include <xc.h>
#include <util/twi.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <string.h>
#include "i2c_master.h"
#include "i2c_master.c"
#include "liquid_crystal_i2c.h"
#include "liquid_crystal_i2c.c"

#ifndef _BV
#define _BV(b) (1U << (b))
#endif

uint16_t buzzer_on_at = 0;
uint16_t buzzer_off_at = 0;

uint32_t millis_counter = 0;

inline void keypad_init(void);
uint8_t keypad_scan(void);

void buzzer_init(void);
void buzzer_task(void);
void buzzer_beep(uint16_t duration_ms);

void timer0_init(void);
uint32_t millis_now(void);

inline void leds_init(void);
inline void led_mode(uint8_t mode);



// -----------------------------------------------------
// MAIN
// -----------------------------------------------------




int main(void) {
	keypad_init();
	timer0_init();
	buzzer_init();
	sei();

	while (1) {
		buzzer_task();
		
		uint8_t key = keypad_scan();
		if (key) {
		} 
	}
}



// -----------------------------------------------------
// INITIALIZERS
// -----------------------------------------------------



inline void keypad_init(void){
	DDRD = 0b11110000;
	PORTD = 0b00001111;
}

uint8_t keypad_scan(void) {
	uint8_t row, col;
	uint8_t cols;
	static uint8_t prevKey;

	for (row = 0; row < 4; row++) {
		PORTD = (PORTD | 0xF0) & ~(1 << (row + 4));
		_delay_us(5);  
		cols = PIND & 0x0F;  

		for (col = 0; col < 4; col++) {
			if (!(cols & (1 << col)) ) {
				if ((prevKey == ((row * 4) + col + 1))) return 0;
				
				buzzer_beep(30);
				prevKey = (row * 4) + col + 1;
				return (row * 4) + col + 1;  
			} 
		}
	}
	prevKey = 0;
	return 0; 
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

ISR(TIMER0_OVF_vect){
	millis_counter++;
}





inline void buzzer_init(){
	DDRB |= (1<<PORTB5);
	PORTB &= ~(1<<PORTB5);
}

inline void buzzer_task(void){
	if (millis_now() > buzzer_off_at){
		PORTB &= ~(1<<PORTB5);
	}
}

inline void buzzer_beep(uint16_t duration_ms){
	PORTB |= (1<<PORTB5);
	buzzer_on_at = millis_now();
	buzzer_off_at = millis_now() + duration_ms;
}





inline void leds_init(void){
	DDRB |= (1<<PORTB0) | (1<<PORTB1);
	PORTB &= ~((1<<PORTB0) | (1<<PORTB1));
}

inline void led_mode(uint8_t mode){
	if (mode == 0){
		PORTB &= ~(1<<PORTB1);
		PORTB |= (1<<PORTB0);
	} else if (mode == 1){
		PORTB &= ~(1<<PORTB0);
		PORTB |= (1<<PORTB1);
	}
}






