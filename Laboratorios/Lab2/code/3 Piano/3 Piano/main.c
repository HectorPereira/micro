/*
Funcionalidades detalladas:

1. Reproducci�n de Notas:
	o	Cada uno de los 8 pulsadores est� asignado a una nota musical
		espec�fica (Do, Re, Mi, Fa, Sol, La, Si, Do agudo).
	o	Al presionar un pulsador, el buzzer reproduce la frecuencia
		correspondiente a la nota seleccionada.

2. Selecci�n de Canciones Predefinidas:
	o	Mediante la comunicaci�n UART, el usuario puede seleccionar
		entre dos canciones almacenadas en el microcontrolador.
	o	Cuando se recibe el comando UART adecuado, el
		microcontrolador detiene la funcionalidad del piano y reproduce la
		canci�n seleccionada de manera autom�tica.

3. Control por UART:
	o	El sistema recibe comandos a trav�s de UART para seleccionar
		una canci�n.
	o	Los comandos pueden ser simples, como 'C1' para la primera
		canci�n y 'C2' para la segunda.
	o	Tambi�n se puede enviar un comando para detener la
		reproducci�n de la canci�n y volver al modo piano
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>

void play(uint16_t Fout){
	DDRD |= (1 << PORTD6);				// OC0A = PD6
	
	uint16_t prescaler = 0;
	if (Fout < 128) prescaler = 1024;
	else if (Fout < 490) prescaler = 256;
	else prescaler = 64;

	OCR0A = (F_CPU/(2*prescaler*Fout))+1;
	
	TCCR0A = (1 << WGM01);              // CTC mode
	TCCR0A |= (1 << COM0A0);            // Toggle OC0A on match
	
	if (Fout < 128) TCCR0B  = 0b101;
	else if (Fout < 490) TCCR0B  = 0b100;
	else TCCR0B  = 0b011;
	
}
		

int main(void) {
	play(500);

	while (1);
}