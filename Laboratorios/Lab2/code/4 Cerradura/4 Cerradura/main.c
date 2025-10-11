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


/*
lcd = 0x04;						//--- EN = 1 for 25us initialize Sequence
PCF8574_write(lcd);
_delay_us(25);
twi_lcd_cmd(0x03);				//--- Initialize Sequence
twi_lcd_cmd(0x03);				//--- Initialize Sequence
twi_lcd_cmd(0x03);				//--- Initialize Sequence
twi_lcd_cmd(0x02);				//--- Return to Home
twi_lcd_cmd(0x28);				//--- 4-Bit Mode 2 - Row Select
twi_lcd_cmd(0x0F);				//--- Cursor on, Blinking on
twi_lcd_cmd(0x01);				//--- Clear LCD
twi_lcd_cmd(0x06);				//--- Auto increment Cursor
twi_lcd_cmd(0x80);				//--- Row 1 Column 1 Address
twi_lcd_msg("Initializing...");	//--- String Send to LCD
_delay_ms(1000);				//--- 1s Delay
twi_lcd_clear();				//--- Clear LCD
twi_lcd_cmd(0x80);				//--- Row 1 Column 1 Address

*/

#include <xc.h>
#include <util/twi.h>
#include <avr/interrupt.h>

#define PCF8574	0x27

#include "./twi_lcd.h"
	

// --- Buzzer ---
#define BUZZER_PORT PORTB
#define BUZZER_DDR  DDRB
#define BUZZER_PIN  PB5
uint8_t buzzer_state = 0;
uint16_t buzzer_timer = 0;

// --- Red LED ---
#define RED_PORT PORTB
#define RED_DDR  DDRB
#define RED_PIN  PB0
uint8_t red_state = 0;
uint16_t red_timer = 0;

// --- Green LED ---
#define GREEN_PORT PORTB
#define GREEN_DDR  DDRB
#define GREEN_PIN  PB1
uint8_t green_state = 0;
uint16_t green_timer = 0;

const char keypad[4][4] = {
	{'1', '2', '3', 'A'},
	{'4', '5', '6', 'B'},
	{'7', '8', '9', 'C'},
	{'*', '0', '#', 'D'}
};

uint8_t keypad_state = 0;
uint8_t keypad_timer = 0;
uint8_t keypad_debounce_flag = 0;
char keypad_active_key = 0; 


void timer0_init(void) {
    TCCR0A = 0x00;
    TCCR0B = (1 << CS01) | (1 << CS00); 
    TCNT0 = 6;
    TIMSK0 |= (1 << TOIE0);
}

void timer0_disable(void) {
	TIMSK0 &= ~(1 << TOIE0);  // Clear overflow interrupt enable bit
}



char read_keypad(void) {
	for (uint8_t row = 0; row < 4; row++) {
		// Pone todas las filas (PD4–PD7) en 1 y solo la actual en 0
		PORTD = (PORTD | 0xF0) & ~(1 << (row + 4));
		_delay_us(5);

		// Lee las columnas (PD0–PD3)
		uint8_t cols = PIND & 0x0F;  // Solo bits 0–3

		for (uint8_t col = 0; col < 4; col++) {
			if (!(cols & (1 << col))) {   // Si está en 0 ? presionada
				keypad_disable();
				timer0_init();
				return keypad[row][col];
			}
		}
	}
	return 0; // ninguna tecla
}

void keypad_disable(void) {
	PCICR &= ~(1 << PCIE2);
}

void keypad_enable(void){
	PCICR  |= (1 << PCIE2);
}


void keypad_init(void) {
	// --- Columnas (PD0–PD3): entradas con pull-up ---
	DDRD  &= ~0x0F;   // entradas
	PORTD |=  0x0F;   // pull-ups activadas

	// --- Filas (PD4–PD7): salidas ---
	DDRD  |=  0xF0;   // salidas
	PORTD |=  0xF0;   // inicializadas en 1

	// (opcional) interrupciones PCINT para columnas
	keypad_enable();
	PCMSK2 |= 0x0F;   // PD0–PD3
}


void leds_init(void) {
	DDRB |= (1 << PB0) | (1 << PB1);   // Configure PB0 and PB1 as outputs
	PORTB &= ~((1 << PB0) | (1 << PB1)); // Start with LEDs off
}


void leds_pattern(uint8_t mode) {
	switch (mode) {
		// Both blink together
		case 0:
		PORTB |= (1 << PB0) | (1 << PB1);
		_delay_ms(200);
		PORTB &= ~((1 << PB0) | (1 << PB1));
		_delay_ms(200);
		break;

		// Alternate blinking
		case 1:
		PORTB |= (1 << PB0);
		PORTB &= ~(1 << PB1);
		_delay_ms(200);

		PORTB |= (1 << PB1);
		PORTB &= ~(1 << PB0);
		_delay_ms(200);
		break;

		// Flash both LEDs twice quickly
		case 2:
		for (uint8_t i = 0; i < 2; i++) {
			PORTB |= (1 << PB0) | (1 << PB1);
			_delay_ms(100);
			PORTB &= ~((1 << PB0) | (1 << PB1));
			_delay_ms(100);
		}
		break;

		// Turn both on steadily
		case 3:
		PORTB |= (1 << PB0) | (1 << PB1);
		break;

		// Turn both off
		case 4:
		PORTB &= ~((1 << PB0) | (1 << PB1));
		break;

		default:
		// Invalid mode -> make both blink fast to indicate error
		for (uint8_t i = 0; i < 3; i++) {
			PORTB ^= (1 << PB0) | (1 << PB1);
			_delay_ms(100);
		}
		break;
	}
}


void beep(uint8_t beep_type){
	switch (beep_type)
	{
		case 1: // key typing
			BUZZER_PORT |= (1<<BUZZER_PIN);
			_delay_ms(50);
			BUZZER_PORT &= ~(1<<BUZZER_PIN);
			
		break;
		case 2: // alarm
		break;
		default:
		break;
		
	}
}

void beep_task(){
	
}


void beeper_init(){
	BUZZER_DDR |= (1<<BUZZER_PIN);
	BUZZER_PORT &= ~(1<<BUZZER_PIN);
}

void led_blink_task(void) {
	if (green_timer == 0) {
		green_timer = 2;          // 30 * 16ms ? 480ms
		green_state ^= 1;	          // toggle
		if (green_state)
		RED_PORT |= (1 << RED_PORT);
		else
		RED_PORT &= ~(1 << RED_PORT);
	}
}


int main(void) {
	twi_init();
	leds_init();
	twi_lcd_init();
	keypad_init();
	beeper_init();
	timer0_init();
	sei();
	
	/*
	char buf[2];
	while (1) {
		keypad_active_key = read_keypad();
		if (keypad_active_key) {
			buf[0] = keypad_active_key;
			buf[1] = '\0';
			keypad_active_key = 0;
			lcd_write(buf);
			beep(1);
			leds_pattern(1);
		}
	}
	*/
	
	while (1){
		led_blink_task();
	}
}


ISR(PCINT2_vect) {
	if (keypad_debounce_flag){
		keypad_state = 1;   
		timer0_init();
		keypad_disable();
	}
}

ISR(TIMER0_OVF_vect) { // 1ms 
	TCNT0 = 6;

	if (green_timer > 0) green_timer--;
	if (red_timer > 0) red_timer--;
}

