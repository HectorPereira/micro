#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "Lib/Liberia.h"

int main(void)
{
	uint8_t Temp = 0;
	uint8_t Hum  = 0;
	uint8_t Humdec;
	uint8_t Tdec;

	DDRD |= (1 << LIGHT_PIN) | (1 << PIN_T);
	PORTD &= ~(1 << PIN_T);

	sei();

	uart_init(UBRR_VALUE);

	// RX por interrupción
	UCSR0B |= (1<<RXCIE0);

	spi_init();
	Init_adc();
	Init_pwm();
	Init_timer1();

	I2C_init();
	twi_lcd_init();

	twi_lcd_cmd(0x80);
	twi_lcd_msg("BIENVENIDO");

	uart_print("Inserte A para medir temperatura\r\n");
	uart_print("Inserte B para mover servo\r\n");
	uart_print("Inserte C para medir distancia\r\n");

	while(1)
	{
		char d[10];

		// --- Lectura LDR / LED por SPI ---
		uint16_t lectura_led = adc_read_AC1();
		if (lectura_led > 650)
		{
			twi_lcd_cmd(0xC0);
			twi_lcd_msg("LED  - ON");
			SS_LOW();
			spi_transfer(0xB6);
			SS_HIGH();
		}
		else if(lectura_led < 650){
			twi_lcd_cmd(0xC0);
			twi_lcd_msg("LED - OFF");
			SS_LOW();
			spi_transfer(0xB7);
			SS_HIGH();
		}
		_delay_ms(100);

		char c = Chardos();

		// ============================
		// A: DHT11 (Temperatura / Hum)
		// ============================
		if (c == 'A') {
			uart_print("\r\nLeyendo DHT11...\r\n");

			if (dht11_read2(&Temp, &Hum)) {
				uart_print("Lectura correcta!\r\n");

				char buf[16];
				sprintf(buf, "Temp: %u C\r\n", Temp);
				uart_print(buf);
				sprintf(buf, "Hum: %u %%\r\n", Hum);
				uart_print(buf);

				if(Temp > 26){
					SS_LOW();
					spi_transfer(0xF0); // Prender ventilador
					SS_HIGH();
					} else {
					SS_LOW();
					spi_transfer(0xB5); // Apagar ventilador
					SS_HIGH();
				}
			}
			else {
				uart_print("Error de lectura\r\n");
			}

			_delay_ms(1500);  // DHT11 necesita >1s entre lecturas
		}

		// ============================
		// B: Servo por potenciómetro
		// ============================
		if(c == 'B'){
			SS_LOW();
			spi_transfer(0xFA); // Activar servo en esclavo
			SS_HIGH();

			twi_lcd_clear();
			twi_lcd_cmd(0x80);
			twi_lcd_msg("Angulo actual:");

			uart_print("Mueva el potenciometro para variar el angulo del servo\r\n");
			uart_print("Presione X para salir\r\n");

			while(1){
				char c2 = Chardos();
				if(c2 == 'X'){
					break;
				}

				uint16_t potenciometro = adc_read_AC0();

				char b[10];

				float grados = potenciometro * 0.17;

				Add_to_string(d, (uint16_t)grados);
				Add_to_string(b, potenciometro);

				uart_print("Grados:\r\n");
				uart_print(d);
				uart_print("\r\n");
				uart_print(b);
				uart_print("\r\n");

				_delay_ms(1000);

				twi_lcd_cmd(0xC0);
				twi_lcd_msg("                ");
				twi_lcd_cmd(0xC0);
				twi_lcd_msg(d);

				// Transmitimos los grados directo (cast a uint8_t)
				SS_LOW();
				spi_transfer((uint8_t)grados);
				SS_HIGH();
			}
			uart_print("termino\r\n");
		}

		// ============================
		// C: Sensor distancia HC-SR04
		// ============================
		if (c == 'C') {
			SS_LOW();
			spi_transfer(0xAA);
			SS_HIGH();

			uart_print("Midiendo distancia... (X para salir)\r\n");

			DDRD |= (1 << PIN_T); // Trig salida

			while ((c = Chardos()) != 'X') {
				// Disparo del ultrasonido
				PORTD |= (1 << PIN_T);
				_delay_us(15);
				PORTD &= ~(1 << PIN_T);

				// Mostrar distancia por UART
				char d2[10];
				Add_to_string(d2, Distancia_cm);
				uart_print("Distancia: ");
				uart_print(d2);
				uart_print(" cm\r\n");

				uint8_t color_code = 0x00;

				// 6 niveles de distancia
				if (Distancia_cm < 10)
				color_code = 0xB9;   // rojo intenso
				else if (Distancia_cm < 40)
				color_code = 0xBA;   // naranja
				else if (Distancia_cm < 90)
				color_code = 0xBB;   // amarillo
				else if (Distancia_cm < 150)
				color_code = 0xBC;   // verde
				else if (Distancia_cm < 300)
				color_code = 0xBD;   // celeste
				else
				color_code = 0xBF;   // azul / apagado

				// Enviar color al esclavo
				SS_LOW();
				spi_transfer(color_code);
				SS_HIGH();

				_delay_ms(300);
			}
		}
	}
}
