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

#ifndef _BV
#define _BV(b) (1U << (b))
#endif

#define DEBOUNCE_MS 30   // time for stable reading

// --- EEPROM ---
uint8_t EEMEM password_mem[5];  // 4 dígitos + terminador '\0'

// --- Buzzer ---
#define BUZZER_PORT PORTB
#define BUZZER_DDR  DDRB
#define BUZZER_PIN  PB5
uint8_t buzzer_state = 0;
uint16_t buzzer_timer = 0;

volatile uint8_t  buzzer_on = 0;
volatile uint32_t buzzer_off_at = 0;

// --- Red LED ---
#define RED_PORT PORTB0
#define RED_DDR  PORTB0
#define RED_PIN  PB0
uint8_t red_state = 0;
uint16_t red_timer = 0;

// --- Green LED ---
#define GREEN_PORT PORTB1
#define GREEN_DDR  PORTB1
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



static inline void buzzer_init(void) {
	DDRB  |= _BV(PB5);   // PB5 como salida
	PORTB &= ~_BV(PB5);  // buzzer apagado
}

static inline void buzzer_beep(uint32_t ms) {
	PORTB |= _BV(PB5);                 // encender buzzer activo
	buzzer_on = 1;
	buzzer_off_at = millis_now() + ms; // programar apagado
}

static inline void buzzer_task(void) {
	if (buzzer_on && (int32_t)(millis_now() - buzzer_off_at) >= 0) {
		PORTB &= ~_BV(PB5);            // apagar buzzer
		buzzer_on = 0;
	}
}





static inline void leds_init(void) {
	DDRB  |= _BV(RED_DDR) | _BV(GREEN_DDR);
	// Estado por defecto: CERRADO ? rojo ON, verde OFF (asumiendo activo en alto)
	PORTB |=  _BV(RED_PORT);
	PORTB &= ~_BV(GREEN_PORT);
}

static inline void leds_set_abierto(uint8_t abierto) {
	if (abierto) {
		// ABIERTO: rojo OFF, verde ON
		PORTB &= ~_BV(RED_PORT);
		PORTB |=  _BV(GREEN_PORT);
		} else {
		// CERRADO: rojo ON, verde OFF
		PORTB |=  _BV(RED_PORT);
		PORTB &= ~_BV(GREEN_PORT);
	}
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
				if ((now - last_change_time) > DEBOUNCE_MS) {
					if (key != stable_key) {
						stable_key = key;
						if (stable_key != 0) {
							buzzer_beep(30);      // <--- BEEP de ~30 ms
							return stable_key;    // stable press
						}
					}
				}
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



static inline void guardar_password_eeprom(const char *pwd4) {
	char tmp[5];
	memcpy(tmp, pwd4, 4);
	tmp[4] = '\0';
	eeprom_update_block((const void*)tmp, (void*)password_mem, sizeof(tmp));
}

typedef enum { UI_MENU, UI_INGRESO, UI_CAMBIO_ACTUAL, UI_CAMBIO_NUEVA, UI_ABIERTO } ui_state_t;


int main(void){
	keypad_init();
	buzzer_init();
	timer0_init_millis();
	leds_init();
	leds_set_abierto(0);   
	sei();

	LiquidCrystalDevice_t device = lq_init(0x27, 16, 2, LCD_5x8DOTS);
	lq_turnOnBacklight(&device);

	// Bienvenida
	char welcomeText[] = "Bienvenido!";
	lq_print(&device, welcomeText);
	_delay_ms(1000);
	lq_clear(&device);

	// Leer password guardada (si querés usarla después)
	char contra_guardada[5];
	eeprom_read_block((void*)contra_guardada, (const void*)password_mem, sizeof(contra_guardada));

	// Mostrar menú inicial
	char ingresarContraTexto[] = "A-> Ingr. contra";
	char cambiarContraTexto[]  = "B-> Camb. contra";
	lq_print(&device, ingresarContraTexto);
	lq_setCursor(&device, 1, 0);
	lq_print(&device, cambiarContraTexto);

	// --- UI state ---
	ui_state_t ui = UI_MENU;

	char entrada[5] = {0};    // buffer para ingreso (4 + '\0')
	uint8_t idx = 0;

	while (1) {
		buzzer_task();
		
		
		char key = read_keypad();
		if (!key) continue;

		switch (ui) {
			case UI_MENU:
			if (key == 'A') {
				lq_clear(&device);
				char titulo[] = "Contra:";
				lq_setCursor(&device, 0, 0);
				lq_print(&device, titulo);
				lq_setCursor(&device, 1, 0);
				idx = 0; entrada[0] = '\0';
				ui = UI_INGRESO;
				} else if (key == 'B') {
				// (tu flujo de cambio ya implementado)
				lq_clear(&device);
				char titulo[] = "Actual:";
				lq_setCursor(&device, 0, 0);
				lq_print(&device, titulo);
				lq_setCursor(&device, 1, 0);
				idx = 0; entrada[0] = '\0';
				ui = UI_CAMBIO_ACTUAL;
			}
			break;

			case UI_INGRESO:
			if (key == '#') {
				// Cancelar y volver al menú
				lq_clear(&device);
				lq_print(&device, ingresarContraTexto);
				lq_setCursor(&device, 1, 0);
				lq_print(&device, cambiarContraTexto);
				ui = UI_MENU;
				break;
			}

			if (idx < 4) {
				entrada[idx++] = key; entrada[idx] = '\0';
				char star[2] = { '*', '\0' };
				lq_print(&device, star);
			}

			if (idx == 4) {
				if (memcmp(entrada, contra_guardada, 4) == 0) {
					// ---- CORRECTA: abrir y esperar '#'
					lq_clear(&device);
					leds_set_abierto(1);
					char abierto[] = "Abierto";
					lq_setCursor(&device, 0, 0);
					lq_print(&device, abierto);
					// Aquí podrías activar un relé/solenoide, etc.
					ui = UI_ABIERTO;
					// No reseteamos idx/entrada todavía (no hace falta)
					} else {
					// ---- INCORRECTA: error y volver al menú
					lq_clear(&device);
					char err[] = "ERROR";
					leds_set_abierto(0);
					lq_setCursor(&device, 0, 0);
					lq_print(&device, err);
					_delay_ms(1000);

					lq_clear(&device);
					lq_print(&device, ingresarContraTexto);
					lq_setCursor(&device, 1, 0);
					lq_print(&device, cambiarContraTexto);
					idx = 0; entrada[0] = '\0';
					ui = UI_MENU;
				}
			}
			break;

			case UI_ABIERTO:
			// Esperar al usuario para cerrar con '#'
			if (key == '#') {
				// Cerrar
				lq_clear(&device);
				char cerrado[] = "Cerrado";
				leds_set_abierto(0);
				lq_setCursor(&device, 0, 0);
				lq_print(&device, cerrado);
				// Aquí podrías desactivar el relé/solenoide
				_delay_ms(700);

				// Volver al menú
				lq_clear(&device);
				lq_print(&device, ingresarContraTexto);
				lq_setCursor(&device, 1, 0);
				lq_print(&device, cambiarContraTexto);
				idx = 0; entrada[0] = '\0';
				ui = UI_MENU;
			}
			break;

			case UI_CAMBIO_ACTUAL:  // Verifica contraseña actual
			if (key == '#') {
				// Cancelar
				lq_clear(&device);
				lq_print(&device, ingresarContraTexto);
				lq_setCursor(&device, 1, 0);
				lq_print(&device, cambiarContraTexto);
				ui = UI_MENU;
				break;
			}

			if (idx < 4) {
				entrada[idx++] = key; entrada[idx] = '\0';
				char star[2] = { '*', '\0' };
				lq_print(&device, star);
			}

			if (idx == 4) {
				if (memcmp(entrada, contra_guardada, 4) == 0) {
					// Correcta ? pedir nueva
					lq_clear(&device);
					char t2[] = "Nueva:";
					lq_setCursor(&device, 0, 0);
					lq_print(&device, t2);
					lq_setCursor(&device, 1, 0);
					idx = 0; entrada[0] = '\0';
					ui = UI_CAMBIO_NUEVA;
					} else {
					lq_clear(&device);
					char err[] = "ERROR";
					lq_setCursor(&device, 0, 0);
					lq_print(&device, err);
					_delay_ms(1000);

					// Volver al menú
					lq_clear(&device);
					lq_print(&device, ingresarContraTexto);
					lq_setCursor(&device, 1, 0);
					lq_print(&device, cambiarContraTexto);
					idx = 0; entrada[0] = '\0';
					ui = UI_MENU;
				}
			}
			break;

			case UI_CAMBIO_NUEVA:   // Captura nueva contraseña y guarda
			if (key == '#') {
				// Cancelar
				lq_clear(&device);
				lq_print(&device, ingresarContraTexto);
				lq_setCursor(&device, 1, 0);
				lq_print(&device, cambiarContraTexto);
				ui = UI_MENU;
				break;
			}

			if (idx < 4) {
				entrada[idx++] = key; entrada[idx] = '\0';
				char star[2] = { '*', '\0' };
				lq_print(&device, star);
			}

			if (idx == 4) {
				// Guardar en EEPROM y actualizar copia en RAM
				guardar_password_eeprom(entrada);
				memcpy(contra_guardada, entrada, 4);
				contra_guardada[4] = '\0';

				lq_clear(&device);
				char ok[] = "OK";
				lq_setCursor(&device, 0, 0);
				lq_print(&device, ok);
				_delay_ms(1000);

				// Volver al menú
				lq_clear(&device);
				lq_print(&device, ingresarContraTexto);
				lq_setCursor(&device, 1, 0);
				lq_print(&device, cambiarContraTexto);
				idx = 0; entrada[0] = '\0';
				ui = UI_MENU;
			}
			break;
		}
	}
}
ISR(TIMER0_COMPA_vect) {
	millis++; // 1 ms tick
}
