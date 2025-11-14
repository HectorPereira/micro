#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint8_t byte = 0;
void Init_pwm(void);


void I2C_init(void)
{
	// SDA = PC4, SCL = PC5 ? ENTRADAS
	DDRC &= ~((1<<PC4) | (1<<PC5));

	// Pull-ups internos (ayudan, pero IGUAL necesitas 4.7k externas)
	PORTC |= (1<<PC4) | (1<<PC5);

	// Dirección del esclavo = 0x50
	TWAR = (0x50 << 1);

	// Habilitar TWI + ACK + interrupción
	TWCR = (1<<TWEN)|(1<<TWEA)|(1<<TWIE);
}

ISR(TWI_vect)
{
	uint8_t status = TWSR & 0xF8;

	switch(status)
	{
		case 0x60: // SLA+W recibido
		break;

		case 0x80: // Datos desde el maestro
		byte = TWDR;
		break;

		default:
		break;
	}

	// Continuar con ACK
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA)|(1<<TWIE);
}

int main(void)
{
	sei();
	// LED en PD7
	DDRD |= (1 << PD7);

	I2C_init();
	
	Init_pwm();
	
	
	uint8_t deg;
	uint16_t ticks;



	uint16_t TMIN = 100;  // Medido
	uint16_t TMAX = 620;  // Medido

	DDRD |= (1 << PORTD2)|(1 << PORTD3)|(1 << PORTD4)|(1 << PORTD5)|(1<< PORTD6);
	
	while(1)
	{
	switch (byte) {

		// ------------------------
		// Control de PORTD2
		// ------------------------
		case 0xB6:
		PORTD |= (1 << PORTD2);
		break;

		case 0xB7:
		PORTD &= ~(1 << PORTD2);
		break;

	

		// ------------------------
		// Colores RGB
		// ------------------------
		case 0xB9: // Rojo
		PORTD |=  (1 << PD3);
		PORTD &= ~(1 << PD4);
		PORTD &= ~(1 << PD5);
		break;

		case 0xBA: // Naranja (R+G)
		PORTD |=  (1 << PD3);
		PORTD |=  (1 << PD4);
		PORTD &= ~(1 << PD5);
		break;

		case 0xBB: // Amarillo
		PORTD |=  (1 << PD3);
		PORTD |=  (1 << PD4);
		PORTD &= ~(1 << PD5);
		break;

		case 0xBC: // Verde
		PORTD &= ~(1 << PD3);
		PORTD |=  (1 << PD4);
		PORTD &= ~(1 << PD5);
		break;

		case 0xBD: // Celeste (G+B)
		PORTD &= ~(1 << PD3);
		PORTD |=  (1 << PD4);
		PORTD |=  (1 << PD5);
		break;

		case 0xBF: // Azul
		PORTD &= ~(1 << PD3);
		PORTD &= ~(1 << PD4);
		PORTD |=  (1 << PD5);
		break;

		// ------------------------
		// Control PWM (OCR0A)
		// ------------------------
		case 0xF0:
		PORTD |= (1 << PORTD6);
		break;

		case 0xB5:
		PORTD  &=  ~(1 << PORTD6);
		break;
	
		// ------------------------
		// Servo (0° a 180°)
		// ------------------------
	
		default:
		if (byte < 180) {
			deg = byte;
			ticks = TMIN + ((uint32_t)(TMAX - TMIN) * deg) / 180;
			OCR1A = ticks;
		}
		break;
	}
	}
}



void Init_pwm(void){
	// --- Servo PWM on PB1 (OC1A)
	// PB1 (OC1A)
	DDRB |= (1 << DDB1);

	TCCR1A = (1 << WGM11);
	TCCR1B = (1 << WGM13) | (1 << WGM12);

	
	TCCR1A |= (1 << COM1A1);
	TCCR1B |= (1 << CS11) | (1 << CS10);
	ICR1 = 4999;
	OCR1A = 90;
}