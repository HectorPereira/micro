#include "Liberia/Liberia.h"

int main(void)
{
	DDRD |= (1 << PORTD4);
	sei();

	DDRD |= (1 << PIN_T);
	PORTD &= ~(1 << PIN_T);

	I2C_init();
	Init_adc();
	twi_lcd_init();
	Init_timer1();

	char str_led[10];

	while(1)
	{
		uint16_t lectura_led = adc_read_AC1();
		if (lectura_led > 650)
		{
			I2C_start();
			I2C_write(0x50 << 1);
			I2C_write(0xB6);
			I2C_stop();
		}
		else if(lectura_led < 650){
			I2C_start();
			I2C_write(0x50 << 1);
			I2C_write(0xB7);
			I2C_stop();
		}

		uint16_t potenciometro = adc_read_AC0();

		char b[10];
		uint8_t grados = potenciometro/5.88;

		I2C_start();
		I2C_write(0x50 << 1);
		I2C_write(grados);
		I2C_stop();
		_delay_us(100);

		char Buffer1[16];
		char Buffer2[16];
		char str_temp[10];
		char str_hum[10];
		char str_grados[10];
		char str_led_adc[10];

		Add_to_string(str_grados, grados);

		PORTD |= (1 << PIN_T);
		_delay_us(15);
		PORTD &= ~(1 << PIN_T);

		uint8_t color_code = 0x00;
		char str_dist[10];

		if (Distancia_cm < 10){
			color_code = 0xB9;
			str_dist[0]= '\0';
			strcat(str_dist, "Rojo");
		}
		else if (Distancia_cm < 40){
			color_code = 0xBA;
			str_dist[0]= '\0';
			strcat(str_dist, "Naranja");
		}
		else if (Distancia_cm < 90){
			color_code = 0xBB;
			str_dist[0]= '\0';
			strcat(str_dist, "Amarillo");
		}
		else if (Distancia_cm < 150){
			color_code = 0xBC;
			str_dist[0]= '\0';
			strcat(str_dist, "Verde");
		}
		else if (Distancia_cm < 300){
			color_code = 0xBD;
			str_dist[0]= '\0';
			strcat(str_dist, "Celeste");
		}
		else{
			color_code = 0xBF;
			str_dist[0]= '\0';
			strcat(str_dist, "Azul");
		}

		I2C_start();
		I2C_write(0x50 << 1);
		I2C_write(color_code);
		I2C_stop();

		uint8_t Tem, Hum;

		if (dht11_read2(&Tem, &Hum)) {
			leer_dht = 0;
			if(Tem > 26){
				I2C_start();
				I2C_write(0x50 << 1);
				I2C_write(0xF0);
				I2C_stop();
				} else {
				I2C_start();
				I2C_write(0x50 << 1);
				I2C_write(0xB5);
				I2C_stop();
			}
		}

		_delay_us(100);

		Buffer1[0]= '\0';
		Buffer2[0]= '\0';
		char baba[10];

		Add_to_string(baba , Tem);

		strcat(Buffer1, "G");
		strcat(Buffer1, str_grados);
		strcat(Buffer1, "    ");

		strcat(Buffer1, "T ");
		strcat(Buffer1, baba);
		strcat(Buffer1, "   ");

		char d[10];
		strcat(Buffer2, "D");
		strcat(Buffer2, str_dist);
		strcat(Buffer2, " cm  ");
		strcat(Buffer2, Add_to_string(d, Distancia_cm));
		strcat(Buffer2, " ");

		twi_lcd_cmd(0x80);
		twi_lcd_msg(Buffer1);
		twi_lcd_cmd(0xC0);
		twi_lcd_msg(Buffer2);
	}
}
