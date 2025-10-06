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
#include <util/delay.h>

void play(uint16_t freq){
	if (!freq) return;
	DDRD |= (1 << PORTD6);

	uint8_t presc_bits = 0b0; // Valor por defecto
	uint16_t ocr;

	// Probar todos los prescalers
	const uint16_t presc_list[] = {8, 64, 256, 1024};
	const uint8_t  bits_list[]  = { 0b010, 0b011, 0b100, 0b101 };

	for (uint8_t i=0;i<4;i++) {
		ocr = (F_CPU / (2UL * presc_list[i] * freq)) - 1;
		if (ocr <= 255) { presc_bits = bits_list[i]; break; }
	}

	TCCR0A = (1 << COM0A0) | (1 << WGM01);
	TCCR0B = presc_bits;        // prescaler elegido
	OCR0A  = (uint8_t)ocr;
}
		

int main(void) {
	play(300);
	_delay_ms(500);
	play(400);
	_delay_ms(500);
	play(500);
	_delay_ms(500);
	play(600);
	
	while (1);
}