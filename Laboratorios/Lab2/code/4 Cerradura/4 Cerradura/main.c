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
 */ 

#include <xc.h>

int main(void)
{
    while(1)
    {
        //TODO:: Please write your application code 
    }
}