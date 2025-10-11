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
	
A -> Ingresar C. 
B -> Cambiar C.


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

#define DEBOUNCE_MS 30   // time for stable reading

// --- EEPROM ---
uint8_t EEMEM password_mem[5];  // 4 dígitos + terminador '\0'

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

volatile uint32_t millis = 0;   // incremented every 1 ms in a timer ISR
uint32_t millis_now(void) {
	uint32_t m;
	cli();
	m = millis;
	sei();
	return m;
}
	
void timer0_init_millis(void) {
	TCCR0A = (1 << WGM01);
	TCCR0B = (1 << CS01) | (1 << CS00); // prescaler 64
	OCR0A = 249;
	TIMSK0 = (1 << OCIE0A); // enable compare interrupt
}

char read_keypad(void) {
	static char last_key = 0;
	static char stable_key = 0;
	static uint32_t last_change_time = 0;

	char key = 0;                 // move declaration here
	uint32_t now;                 // also declare now here
	uint8_t row, col;
	uint8_t cols;

	// --- Scan the matrix ---
	for (row = 0; row < 4; row++) {
		PORTD = (PORTD | 0xF0) & ~(1 << (row + 4));
		_delay_us(5);
		cols = PIND & 0x0F;
		for (col = 0; col < 4; col++) {
			if (!(cols & (1 << col))) {
				key = keypad[row][col];
				goto done;
			}
		}
	}
	done:                            // label must precede statements, not declarations

	now = millis_now();

	// --- Debounce logic ---
	if (key != last_key) {
		last_key = key;
		last_change_time = now;
	}

	if ((now - last_change_time) > DEBOUNCE_MS) {
		if (key != stable_key) {
			stable_key = key;
			if (stable_key != 0) {
				return stable_key; // stable press
			}
		}
	}

	return 0;
}

void keypad_init(void) {
	DDRD  &= ~0x0F;   // entradas
	PORTD |=  0x0F;   // pull-ups activadas

	DDRD  |=  0xF0;   // salidas
	PORTD |=  0xF0;   // inicializadas en 1
}



void guardar_password(void) {
	char password[5] = "1234";
	eeprom_update_block((const void*)password, (void*)password_mem, sizeof(password));
}



int main(void){
	keypad_init();
	timer0_init_millis();
	sei();
	
	LiquidCrystalDevice_t device = lq_init(0x27, 16, 2, LCD_5x8DOTS);
	lq_turnOnBacklight(&device);
	
	char welcomeText[] = "Bienvenido!";
	lq_print(&device, welcomeText);
	
	_delay_ms(1000);
	lq_clear(&device);
	
	char password_leida[5];  // buffer en RAM

	// Leer la contraseña almacenada en EEPROM
	eeprom_read_block((void*)password_leida, (const void*)password_mem, sizeof(password_leida));

	// Mostrarla en el LCD
	char eepromText[] = "EEPROM:";
	
	lq_setCursor(&device, 0, 0);
	lq_print(&device, eepromText);
	lq_setCursor(&device, 1, 0);
	lq_print(&device, password_leida);
	
	_delay_ms(5000);
	lq_clear(&device);
	
	lq_turnOnCursor(&device);
	lq_turnOnBlink(&device);
	
	char ingresarContraTexto[] = "Ingresar contra:";
	lq_print(&device, ingresarContraTexto);
	lq_setCursor(&device, 1, 0); // move to next line
	
	char key;
	char key_buffer[5] = {0};
	uint8_t index = 0;
	
	while (1) {
		char key = read_keypad();
		if (key) {
			char str[2] = { key, '\0' };
			lq_print(&device, str);
		}
	}
}

ISR(TIMER0_COMPA_vect) {
	millis++; // 1 ms tick
}
