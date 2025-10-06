/*
Funcionalidades detalladas:

1. Reproducción de Notas:
	o	Cada uno de los 8 pulsadores está asignado a una nota musical
		específica (Do, Re, Mi, Fa, Sol, La, Si, Do agudo).
	o	Al presionar un pulsador, el buzzer reproduce la frecuencia
		correspondiente a la nota seleccionada.

2. Selección de Canciones Predefinidas:
	o	Mediante la comunicación UART, el usuario puede seleccionar
		entre dos canciones almacenadas en el microcontrolador.
	o	Cuando se recibe el comando UART adecuado, el
		microcontrolador detiene la funcionalidad del piano y reproduce la
		canción seleccionada de manera automática.

3. Control por UART:
	o	El sistema recibe comandos a través de UART para seleccionar
		una canción.
	o	Los comandos pueden ser simples, como 'C1' para la primera
		canción y 'C2' para la segunda.
	o	También se puede enviar un comando para detener la
		reproducción de la canción y volver al modo piano
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