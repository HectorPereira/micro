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
#include <stdint.h>

#include "i2c_master.h"
#include "i2c_master.c"
#include "liquid_crystal_i2c.h"
#include "liquid_crystal_i2c.c"

#define ALARM_DURATION_MS 10000
#define ALARM_TOGGLE_MS 200
#define MAX_INTENTOS 3


uint32_t buzzer_on_at = 0;
uint32_t buzzer_off_at = 0;

uint8_t alarm_active = 0;
uint32_t alarm_until = 0;
uint8_t alarm_phase = 0;
uint32_t alarm_next_toggle = 0;

uint32_t led_on_at = 0;
uint32_t led_off_at = 0;
uint8_t led_state = 0;
uint8_t led_toggle_enable = 0;

uint32_t keypad_on_at = 0;
uint8_t keypad_enable = 1;

uint32_t millis_counter = 0;	

#define MAX_PASSWORD_LENGTH 6
#define EEPROM_MAGIC 0x42

uint8_t EEMEM ee_magic;                
char EEMEM ee_password[MAX_PASSWORD_LENGTH + 1];

char storedPassword[MAX_PASSWORD_LENGTH + 1] = "123456";
char typedPassword[MAX_PASSWORD_LENGTH + 1];


uint8_t typedPassword_counter = 0;
uint8_t storedPassword_length = 0;

const char keypad[4][4] = {
	{'1', '2', '3', 'A'},
	{'4', '5', '6', 'B'},
	{'7', '8', '9', 'C'},
	{'*', '0', '#', 'D'}
};

ISR(TIMER0_OVF_vect){
	millis_counter++;
}




void eeprom_load_password(void);
void eeprom_save_password(const char *pwd);

void keypad_init(void);
char keypad_scan(void);
void keypad_debounce_ms(uint16_t delay_ms);
void keypad_task(void);

void buzzer_init(void);
void buzzer_task(void);
void buzzer_beep(uint16_t duration_ms);

void alarm_start(void);
void alarm_task(void);

void timer0_init(void);
uint32_t millis_now(void);

void led_init(void);
void led_mode(uint8_t mode, uint16_t delay);
void led_task(void);
void led_green_on(void);
void led_red_on(void);

void state_menu_UI(LiquidCrystalDevice_t device);
void state_ingreso_UI(LiquidCrystalDevice_t device);
void state_cambio_actual_UI(LiquidCrystalDevice_t device);
void state_cambio_nueva_UI(LiquidCrystalDevice_t device);
void state_abierto_UI(LiquidCrystalDevice_t device);
void state_alarma_UI(LiquidCrystalDevice_t device);

void reset_typed_password(void);




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
	
	eeprom_load_password();
	
	LiquidCrystalDevice_t device = lq_init(0x27, 16, 2, LCD_5x8DOTS);

	lq_turnOnBacklight(&device);
	char welcomeText[] = "Bienvenido!";
	lq_print(&device, welcomeText);
	
	_delay_ms(1000);
	
	state_menu_UI(device);
	ui_state_t ui_state = UI_MENU;
	
	storedPassword_length = strlen(storedPassword);
	

	reset_typed_password();
	uint8_t intentos = MAX_INTENTOS;
	

	while (1) {
		buzzer_task();
		led_task();
		keypad_task();
		alarm_task();
		
		char key = keypad_scan();
		if (key) {
			buzzer_beep(20);
			
			switch (ui_state)
			{
				case UI_MENU:
				if (key == 'A'){
					state_ingreso_UI(device);
					ui_state = UI_INGRESO;
					
				} else if (key == 'B'){
					state_cambio_actual_UI(device);
					ui_state = UI_CAMBIO_ACTUAL;
					
				}
				break;
				case UI_INGRESO:
				if (key == '#'){ // Home button 
					reset_typed_password();
					state_menu_UI(device);
					ui_state = UI_MENU;
				} else if (key == 'D'){ // Send button
					if (strcmp(storedPassword, typedPassword) != 0){
						if (--intentos == 0){
							state_alarma_UI(device);
							alarm_start();
							
							ui_state = UI_ALARMA;
							reset_typed_password();
							
							continue;
						}
						lq_clear(&device);
						char text[] = "Incorrecto!";
						lq_setCursor(&device,0,0);
						lq_print(&device, text);
						lq_setCursor(&device,1,0);
						
						_delay_ms(500);
					
						reset_typed_password();
						state_ingreso_UI(device);
						ui_state = UI_INGRESO;
					} else {
						intentos = MAX_INTENTOS;
						reset_typed_password();
						led_green_on();
						state_abierto_UI(device);
						ui_state = UI_ABIERTO;
					}
					
						
				} else if (key == 'C'){
					if (typedPassword_counter == 0) continue;
					char text[] = " ";
					lq_setCursor(&device,1, --typedPassword_counter);
					lq_print(&device, text);
					lq_setCursor(&device,1, typedPassword_counter);
					typedPassword[typedPassword_counter] = '\0';
				}
				
				else { // Any other key
					if (typedPassword_counter >= MAX_PASSWORD_LENGTH) continue;
					typedPassword[typedPassword_counter++] = key;
					typedPassword[typedPassword_counter] = '\0';  
					char star[] =  "*";
					lq_print(&device, star);
					
				}
				
				break;
				case UI_CAMBIO_ACTUAL:
				if (key == '#'){ // Home button
					reset_typed_password();
					state_menu_UI(device);
					ui_state = UI_MENU;
				} else if (key == 'D'){ // Send button
					if (strcmp(storedPassword, typedPassword) != 0){ 
						// Password incorrect
						if (--intentos == 0){
							state_alarma_UI(device);
							alarm_start();
							ui_state = UI_ALARMA;
							reset_typed_password();
							continue;
						}
						lq_clear(&device);
						char text[] = "Incorrecto!";
						lq_setCursor(&device,0,0);
						lq_print(&device, text);
						lq_setCursor(&device,1,0);
						
						_delay_ms(500);
						
						reset_typed_password();
						state_cambio_actual_UI(device);
						ui_state = UI_CAMBIO_ACTUAL;
						} else {
						
						// Password correct
						reset_typed_password();
						state_cambio_nueva_UI(device);
						ui_state = UI_CAMBIO_NUEVA;
					}
				} else if (key == 'C'){ // Delete character
					if (typedPassword_counter == 0) continue;
					char text[] = " ";
					lq_setCursor(&device,1, --typedPassword_counter);
					lq_print(&device, text);
					lq_setCursor(&device,1, typedPassword_counter);
					typedPassword[typedPassword_counter] = '\0';
				} else { // Any other key
					if (typedPassword_counter >= MAX_PASSWORD_LENGTH) continue;
					typedPassword[typedPassword_counter++] = key;
					typedPassword[typedPassword_counter] = '\0';
					char star[] =  "*";
					lq_print(&device, star);
					
				}
				break;
				case UI_CAMBIO_NUEVA:
				if (key == '#'){ // Home button
					reset_typed_password();
					state_menu_UI(device);
					ui_state = UI_MENU;
				} else if (key == 'D'){ // Send button
					if (strlen(typedPassword) < 4) continue;

					strncpy(storedPassword, typedPassword, MAX_PASSWORD_LENGTH);
					storedPassword[MAX_PASSWORD_LENGTH] = '\0';
					storedPassword_length = strlen(storedPassword);

					eeprom_save_password(storedPassword);  // <<< persist change

					reset_typed_password();
					lq_clear(&device);
					char text[] = "Contra cambiada!";
					lq_setCursor(&device,0,0);
					lq_print(&device, text);
					lq_setCursor(&device,1,0);

					_delay_ms(500);
					state_menu_UI(device);
					ui_state = UI_MENU;
					
				} else if (key == 'C'){ // Delete character
					if (typedPassword_counter == 0) continue;
					char text[] = " ";
					lq_setCursor(&device,1, --typedPassword_counter);
					lq_print(&device, text);
					lq_setCursor(&device,1, typedPassword_counter);
					typedPassword[typedPassword_counter] = '\0';
				} else { // Any other key
					if (typedPassword_counter >= MAX_PASSWORD_LENGTH) continue;
					typedPassword[typedPassword_counter++] = key;
					typedPassword[typedPassword_counter] = '\0';
					char star[] =  "*";
					lq_print(&device, star);
				}
				break;
				case UI_ALARMA:
				if (key == 'D'){ // Send button
					if (strcmp(storedPassword, typedPassword) != 0){
						alarm_start(); //Pasworkd incorrect
						reset_typed_password();
						state_alarma_UI(device);
						continue;
						} else {
						// Password correct
						intentos = MAX_INTENTOS;
						alarm_until = millis_now();
						reset_typed_password();
						state_menu_UI(device);
						ui_state = UI_MENU;
						}
					
					} else if (key == 'C'){ // Delete character
						if (typedPassword_counter == 0) continue;
						char text[] = " ";
						lq_setCursor(&device,1, --typedPassword_counter);
						lq_print(&device, text);
						lq_setCursor(&device,1, typedPassword_counter);
						typedPassword[typedPassword_counter] = '\0';
					} else { // Any other key
						if (typedPassword_counter >= MAX_PASSWORD_LENGTH) continue;
						typedPassword[typedPassword_counter++] = key;
						typedPassword[typedPassword_counter] = '\0';
						char star[] =  "*";
						lq_print(&device, star);
					}
				break;
				case UI_ABIERTO:
				if (key == 'D'){ // Home button
					
					lq_clear(&device);
					char text[] = "Cerrado!";
					lq_setCursor(&device,0,0);
					lq_print(&device, text);
					lq_setCursor(&device,1,0);
					
					led_red_on();
					_delay_ms(500);
					state_menu_UI(device);
					ui_state = UI_MENU;
				} 
				break;
			}
		} 
	}
}







void state_menu_UI(LiquidCrystalDevice_t device ){
	char integresarTexto[] = "A-> Ingr. contra";
	char cambiarTexto[] = "B-> Camb. contra";
	lq_clear(&device);
	lq_setCursor(&device,0,0);
	lq_print(&device, integresarTexto);
	lq_setCursor(&device,1,0);
	lq_print(&device, cambiarTexto);
}

void state_ingreso_UI(LiquidCrystalDevice_t device){
	lq_clear(&device);
	char text[] = "Ingresar contra:";
	lq_setCursor(&device,0,0);
	lq_print(&device, text);
	lq_setCursor(&device,1,0);
}

void state_cambio_actual_UI(LiquidCrystalDevice_t device){
	lq_clear(&device);
	char text[] = "Contra actual:";
	lq_setCursor(&device,0,0);
	lq_print(&device, text);
	lq_setCursor(&device,1,0);
}

void state_cambio_nueva_UI(LiquidCrystalDevice_t device){
	lq_clear(&device);
	char text[] = "Nueva contra:";
	lq_setCursor(&device,0,0);
	lq_print(&device, text);
	lq_setCursor(&device,1,0);
}

void state_abierto_UI(LiquidCrystalDevice_t device){
	lq_clear(&device);
	char text[] = "Candado abierto";
	lq_setCursor(&device,0,0);
	lq_print(&device, text);
	lq_setCursor(&device,1,0);
}

void state_alarma_UI(LiquidCrystalDevice_t device){
	lq_clear(&device);
	char text[] = "!!!!!ALERTA!!!!!";
	lq_setCursor(&device,0,0);
	lq_print(&device, text);
	lq_setCursor(&device,1,0);
}




void eeprom_load_password(void) {
	if (eeprom_read_byte(&ee_magic) != EEPROM_MAGIC) {
		eeprom_update_block(storedPassword, ee_password, sizeof(storedPassword));
		eeprom_update_byte(&ee_magic, EEPROM_MAGIC);
		} else {
		eeprom_read_block(storedPassword, ee_password, sizeof(storedPassword));
	}
}

void eeprom_save_password(const char *pwd) {
	char buf[MAX_PASSWORD_LENGTH + 1];
	strncpy(buf, pwd, MAX_PASSWORD_LENGTH);
	buf[MAX_PASSWORD_LENGTH] = '\0';
	eeprom_update_block(buf, ee_password, sizeof(buf));
}





 void reset_typed_password(void){
	typedPassword[0] = '\0';
	typedPassword_counter = 0;
}





 void keypad_init(void){
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
				
				keypad_debounce_ms(200);
				prevKey = keypad[row][col];
				return keypad[row][col];  
			} 
		}
	}
	prevKey = 0;
	return 0; 
}

 void keypad_task(void){
	if (!keypad_enable && (millis_now() > keypad_on_at)){
		keypad_enable = 1;
	}
}

 void keypad_debounce_ms(uint16_t delay_ms){
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







 void buzzer_init(){
	DDRB |= (1<<PORTB5);
	PORTB &= ~(1<<PORTB5);
}


 void buzzer_task(){
	if (millis_now() > buzzer_off_at){
		PORTB &= ~(1<<PORTB5);
	}
}

 void buzzer_beep(uint16_t duration_ms){
	PORTB |= (1<<PORTB5);
	buzzer_on_at = millis_now();
	buzzer_off_at = millis_now() + duration_ms;
}




 void alarm_start(void) {
	uint32_t now = millis_now();
	alarm_active = 1;
	alarm_phase = 0;
	alarm_until = now + ALARM_DURATION_MS;
	alarm_next_toggle = now; 
}

 void alarm_task(void){
	if (!alarm_active) return;
	uint32_t now = millis_now();
	
	if (now > alarm_until){
		alarm_active = 0;
		PORTB &= ~(1<<PORTB5);
		led_red_on();
		return;
	}
	
	if (now > alarm_next_toggle){
		alarm_next_toggle = now + ALARM_TOGGLE_MS;
		if (alarm_phase){
			led_red_on();
		} else {
			led_green_on();
		}
		alarm_phase ^= 1;
		
		buzzer_beep(100);
	}
	
	
}





 void led_red_on(){ // Enciende rojo
	PORTB &= ~(1<<PORTB1);
	PORTB |= (1<<PORTB0);
	led_state = 0;
}

 void led_green_on(){  // Enciende verde
	PORTB &= ~(1<<PORTB0);
	PORTB |= (1<<PORTB1);
	led_state = 1;
}

 void led_init(void){ // Inicia rojo prendido
	DDRB |= (1<<PORTB0) | (1<<PORTB1);
	PORTB |= (1<<PORTB0);
	PORTB &= ~(1<<PORTB1);
}

 void led_mode(uint8_t mode, uint16_t delay){
	led_on_at = millis_now();
	led_off_at = led_on_at + delay;
	led_toggle_enable = 1;
	
	if (mode == 0){
		led_red_on();
		
	} else if (mode == 1){
		led_green_on();
	}
}

void led_task(void){
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








