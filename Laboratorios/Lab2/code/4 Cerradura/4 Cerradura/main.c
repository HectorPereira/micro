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

uint32_t buzzer_on_at = 0;
uint32_t buzzer_off_at = 0;

uint32_t led_on_at = 0;
uint32_t led_off_at = 0;
uint8_t led_state = 0;
uint8_t led_toggle_enable = 0;

uint32_t keypad_on_at = 0;
uint8_t keypad_enable = 1;

uint32_t millis_counter = 0;

const char keypad[4][4] = {
	{'1', '2', '3', 'A'},
	{'4', '5', '6', 'B'},
	{'7', '8', '9', 'C'},
	{'*', '0', '#', 'D'}
};

inline void keypad_init(void);
char keypad_scan(void);
inline void keypad_debounce_ms(uint16_t delay_ms);
inline void keypad_task(void);

void buzzer_init(void);
void buzzer_task(void);
void buzzer_beep(uint16_t duration_ms);

void timer0_init(void);
uint32_t millis_now(void);

inline void led_init(void);
inline void led_mode(uint8_t mode, uint16_t delay);
inline void led_task(void);



// -----------------------------------------------------
// MAIN
// -----------------------------------------------------

typedef enum { UI_MENU, UI_INGRESO, UI_CAMBIO_ACTUAL, UI_CAMBIO_NUEVA, UI_ABIERTO, UI_ALARMA } ui_state_t;


int main(void) {
	keypad_init();
	timer0_init();
	buzzer_init();
	led_init();
	sei();

	LiquidCrystalDevice_t device = lq_init(0x27, 16, 2, LCD_5x8DOTS);
	lq_turnOnBacklight(&device);
	char welcomeText[] = "Bienvenido!";
	lq_print(&device, welcomeText);
	
	_delay_ms(500);
	lq_clear(&device);
	
	char integresarTexto[] = "A-> Ingr. contra";
	char cambiarTexto[] = "B-> Camb. contra";
	lq_print(&device, integresarTexto);
	lq_setCursor(&device,1,0);
	lq_print(&device, cambiarTexto);
	
	ui_state_t ui_state = UI_MENU;
	
	

	while (1) {
		buzzer_task();
		led_task();
		keypad_task();
		
		char key = keypad_scan();
		if (key) {
			buzzer_beep(20);
			led_mode(1, 200);
			
			switch (ui_state)
			{
				case UI_MENU:
				if (key == 'A'){
					lq_clear(&device);
					char text[] = "Ingresar contra:";
					lq_setCursor(&device,0,0);
					lq_print(&device, text);
					
					ui_state = UI_INGRESO;
					
				} else if (key == 'B'){
					lq_clear(&device);
					char text[] = "Contra actual:";
					lq_setCursor(&device,0,0);
					lq_print(&device, text);
					
					ui_state = UI_CAMBIO_ACTUAL;
					
				}
				break;
				case UI_INGRESO:
				break;
				case UI_CAMBIO_ACTUAL:
				break;
				case UI_CAMBIO_NUEVA:
				break;
				case UI_ALARMA:
				break;
				case UI_ABIERTO:
				break;
			}
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

char keypad_scan(void) {
	if (!keypad_enable) return 0;
	
	uint8_t row, col;
	uint8_t cols;
	static uint8_t prevKey; // store for later

	for (row = 0; row < 4; row++) {
		PORTD = (PORTD | 0xF0) & ~(1 << (row + 4));
		_delay_us(5);  
		cols = PIND & 0x0F;  

		for (col = 0; col < 4; col++) {
			if (!(cols & (1 << col)) ) {
				if ((prevKey == keypad[row][col])) return 0;
				
				keypad_debounce_ms(100);
				prevKey = keypad[row][col];
				return keypad[row][col];  
			} 
		}
	}
	prevKey = 0;
	return 0; 
}

inline void keypad_task(void){
	if (!keypad_enable && (millis_now() > keypad_on_at)){
		keypad_enable = 1;
	}
}

inline void keypad_debounce_ms(uint16_t delay_ms){
	keypad_enable = 0;
	keypad_on_at = millis_now() + delay_ms;
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







inline void led_red_on(){ // Enciende rojo
	PORTB &= ~(1<<PORTB1);
	PORTB |= (1<<PORTB0);
	led_state = 0;
}

inline void led_green_on(){  // Enciende verde
	PORTB &= ~(1<<PORTB0);
	PORTB |= (1<<PORTB1);
	led_state = 1;
}

inline void led_init(void){ // Inicia rojo prendido
	DDRB |= (1<<PORTB0) | (1<<PORTB1);
	PORTB |= (1<<PORTB0);
	PORTB &= ~(1<<PORTB1);
}

inline void led_mode(uint8_t mode, uint16_t delay){
	led_on_at = millis_now();
	led_off_at = led_on_at + delay;
	led_toggle_enable = 1;
	
	if (mode == 0){
		led_red_on();
		
	} else if (mode == 1){
		led_green_on();
	}
}

inline void led_task(void){
	if (!led_toggle_enable) return;
	
	if (millis_now() > led_off_at){
		if (led_state == 0){
			led_green_on();
		} else if (led_state == 1){
			led_red_on();
		}
		led_toggle_enable = 0;
	}
}






