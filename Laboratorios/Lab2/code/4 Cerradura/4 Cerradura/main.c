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

#define PCF8574	0x27	

#include <xc.h>
#include <util/twi.h>
#include <avr/interrupt.h>
#include "./twi_lcd.h"



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

void lcd_write(const char *text){
    twi_lcd_cmd(0x06);
	twi_lcd_cmd(0x06);
	twi_lcd_cmd(0x06);
	twi_lcd_cmd(0x06);
	twi_lcd_cmd(0x06);
	twi_lcd_msg(text);
}

int main(void)
{
	twi_init();
	twi_lcd_init();
	
	DDRC |= (1<<PORTC0) | (1<<PORTC1);
	PORTC |= (1<<PORTC0) | (1<<PORTC1);
	
	char key;
    while(1)
    {
		lcd_write("Hello world!");
		_delay_ms(50);
		twi_lcd_clear();
		
		key = read_keypad();
		if (boton_presionado) {
			
			Escribir_LCD(key);
			
			_delay_ms(500);
			
			setearLCD(d);
		}
    }
}

// ---- Interrupci�n por cambio de pin ----
ISR(PCINT2_vect) {
	boton_presionado = 1;   // Marca el evento
}
	