#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "Liberia/Liberia.h"

int main(void)
{
	uint8_t Temp = 0;
	uint8_t Hum  = 0;

	DDRD |= (1<<LIGHT_PIN)|(1<<PIN_T);
	PORTD &= ~(1<<PIN_T);

	sei();

	uart_init(UBRR_VALUE);
	UCSR0B |= (1<<RXCIE0);

	spi_init();
	Init_adc();
	Init_pwm();
	Init_timer1();
	timer2_init();

	I2C_init();
	twi_lcd_init();

	char str_led[10];
	char str_dist[10];
	char str_grados[10];
	char str_temp[10];
	char str_hum[10];

	while(1)
	{
		uint16_t lectura_led = adc_read_AC1();
		if (lectura_led > 650){
			SS_LOW(); spi_transfer(0xB6); SS_HIGH();
			strcpy(str_led,"Prendido");
			} else {
			SS_LOW(); spi_transfer(0xB7); SS_HIGH();
			strcpy(str_led,"Apagado");
		}

		SS_LOW(); spi_transfer(0xFA); SS_HIGH();

		uint16_t pot = adc_read_AC0();
		float grados = pot * 0.17;
		Add_to_string(str_grados, (uint16_t)grados);
		SS_LOW(); spi_transfer((uint8_t)grados); SS_HIGH();

		PORTD |= (1<<PIN_T);
		_delay_us(15);
		PORTD &= ~(1<<PIN_T);

		if (Distancia_cm < 10){
			strcpy(str_dist,"Rojo");
			SS_LOW(); spi_transfer(0xB9); SS_HIGH();
		}
		else if (Distancia_cm < 40){
			strcpy(str_dist,"Naranja");
			SS_LOW(); spi_transfer(0xBA); SS_HIGH();
		}
		else if (Distancia_cm < 90){
			strcpy(str_dist,"Amarillo");
			SS_LOW(); spi_transfer(0xBB); SS_HIGH();
		}
		else if (Distancia_cm < 150){
			strcpy(str_dist,"Verde");
			SS_LOW(); spi_transfer(0xBC); SS_HIGH();
		}
		else if (Distancia_cm < 300){
			strcpy(str_dist,"Celeste");
			SS_LOW(); spi_transfer(0xBD); SS_HIGH();
		}
		else {
			strcpy(str_dist,"Azul");
			SS_LOW(); spi_transfer(0xBF); SS_HIGH();
		}

		if(dht11_read2(&Temp,&Hum) && leer_dht){
			leer_dht=0;
			if(Temp>26){
				SS_LOW(); spi_transfer(0xF0); SS_HIGH();
				} else {
				SS_LOW(); spi_transfer(0xB5); SS_HIGH();
			}
		}

		Add_to_string(str_temp, Temp);
		Add_to_string(str_hum, Hum);

		char L1[16], L2[16];
		L1[0]=0; L2[0]=0;

		strcat(L1,"G"); strcat(L1,str_grados);
		strcat(L1," T "); strcat(L1,str_temp);

		strcat(L2,"D "); strcat(L2,str_dist);

		twi_lcd_cmd(0x80);
		twi_lcd_msg(L1);
		twi_lcd_cmd(0xC0);
		twi_lcd_msg(L2);
	}
}
