#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <util/twi.h>

uint16_t adc_read_AC0(void);
void Init_adc(void);


void I2C_init(void);
void I2C_start(void);
void I2C_stop(void);
uint8_t I2C_write(uint8_t data);

int main(void)
{
	I2C_init();
	Init_adc();
	while(1)
	{
		uint16_t potenciometro = adc_read_AC0();
		
		char b[10];
		uint8_t grados = potenciometro/5.88;
	
		I2C_start();
		I2C_write(0x50 << 1);   // dirección esclavo + Write
		I2C_write(grados);
		I2C_stop();
		_delay_us(100);
		
	}
}


void Init_adc(void) {
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Enable, prescaler 128
	DIDR0  = (1 << ADC0D) | (1 << ADC1D);
}

uint16_t adc_read_AC0(void) {
	ADMUX  = (1 << REFS0);  // AVcc reference, MUX=0000 (ADC0)
	ADCSRA |= (1 << ADSC);  // Start conversion
	while (ADCSRA & (1 << ADSC));  // Wait until finished
	return ADC;  // Read result
}



void I2C_init(void)
{
	// 100 kHz
	TWSR = 0x00;
	TWBR = ((F_CPU / 100000UL) - 16) / 2;

	TWCR = (1 << TWEN);
}

void I2C_start(void)
{
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
}

void I2C_stop(void)
{
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
}

uint8_t I2C_write(uint8_t data)
{
	TWDR = data;
	TWCR = (1<<TWINT)|(1<<TWEN);

	while(!(TWCR & (1<<TWINT)));

	uint8_t status = TWSR & 0xF8;

	return (status == 0x18 || status == 0x28);  // SLA+W ACK o DATA ACK
}
