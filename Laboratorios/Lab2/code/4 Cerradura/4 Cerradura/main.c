/*
Funcionalidades detalladas:

1. Bienvenida y Men� Interactivo:
	-	Al encender el sistema, la pantalla LCD muestra un mensaje de
		bienvenida e instrucciones para ingresar la contrase�a.
	-	El men� tambi�n da la opci�n de cambiar la contrase�a si el
		usuario lo desea.

2. Ingreso de Contrase�a:
	-	El usuario ingresa la contrase�a mediante el teclado matricial.
	-	La contrase�a es comparada con la almacenada en la EEPROM.
	-	Si es correcta, se enciende el LED verde.
	-	Si es incorrecta, se enciende el LED rojo. Despu�s de 3 intentos
		fallidos, se activa la alarma (buzzer).

3. Almacenamiento en EEPROM:
	-	La contrase�a se guarda en la EEPROM para que est� disponible
		despu�s de un reinicio o apagado del sistema.
	-	El microcontrolador puede leer la contrase�a desde la EEPROM y
		comparar los valores con lo que el usuario ingresa.

4. Cambio de Contrase�a:
	-	El sistema permite al usuario cambiar la contrase�a ingresando
		primero la contrase�a actual.
	-	Luego, el usuario puede elegir una nueva contrase�a de entre 4 y
		6 d�gitos, que ser� almacenada en la EEPROM.
		
Requerimientos:
	-	Microcontrolador: ATmega328P
	-	Teclado Matricial: 4x4 (16 teclas)
	-	Pantalla LCD: 16x2 o similar
	-	LED Verde: Indica �xito (contrase�a correcta)
	-	LED Rojo: Indica error (contrase�a incorrecta)
	-	Alarma: Buzzer o LED de advertencia
	-	EEPROM: Almacena la contrase�a de manera persistente
 */ 


#define F_CPU 16000000
#include <xc.h>
#include <util/twi.h>
#include <avr/interrupt.h>
#include "i2c_master.h"
#include "i2c_master.c"
#include "liquid_crystal_i2c.h"
#include "liquid_crystal_i2c.c"

	

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

int main(void){
	
	LiquidCrystalDevice_t device = lq_init(0x27, 16, 2, LCD_5x8DOTS); 

	lq_turnOnBacklight(&device); 
	lq_turnOnCursor(&device);
	lq_turnOnBlink(&device);
	
	char text1[] = "Password:";
	char text2[] = "How are you?";
	lq_print(&device, text1);
	lq_setCursor(&device, 1, 0); // moving cursor to the next line
	lq_print(&device, text2);
}