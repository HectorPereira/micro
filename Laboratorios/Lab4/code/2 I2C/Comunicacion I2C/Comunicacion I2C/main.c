#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <util/twi.h>



char* Add_to_string(char *out, uint16_t val);
char Number_to_ascii(uint16_t val);

uint16_t adc_read_AC0(void);
void Init_adc(void);


void I2C_init(void);
void I2C_start(void);
void I2C_stop(void);
uint8_t I2C_write(uint8_t data);


// ======================================
// Control del LCD - I2C
// ======================================

uint8_t PCF_ADDR = 0x27; // Direccion predeterminada del modulo I2C
#define LCD_EN          0x04   // Enable
#define LCD_RW          0x02   // Read/Write
#define LCD_RS          0x01   // Register Select
#define LCD_BACKLIGHT   0x08   // Retroiluminación
#define LCD_1line 0x80
#define LCD_2line 0xC0

uint8_t nibble_to_bus(uint8_t nibble);
void pcf8574_autodetect(void);
void PCF8574_write(uint8_t b);
void twi_lcd_cmd(const unsigned char x);
void twi_lcd_dwr(unsigned char x);
void twi_lcd_msg(const char *c);
void twi_lcd_clear();
void twi_lcd_4bit_send(unsigned char x);
void twi_lcd_init();

void lcd_twolines(char c, char b);

unsigned char lcd = 0x00;


// ======================================
// HC-SR04
// ======================================

#define PIN_T PORTD4
#define PIN_ECHO PORTB0

uint16_t Distancia_cm = 0;

void Init_timer1();


uint16_t endd = 0;
uint16_t start = 0;
uint16_t width = 0;

// ISR de Input Capture para PB0(ICP1)
ISR(TIMER1_CAPT_vect) {
	if (TCCR1B & (1 << ICES1)) {
		// Flanco ascendente ? guardar inicio y cambiar a descendente
		start = ICR1;
		TCCR1B &= ~(1 << ICES1);  // Detectar próximo flanco descendente
		} else {
		// Flanco descendente > guardar fin y calcular duración
		endd = ICR1;
		width = endd - start;
		TCCR1B |= (1 << ICES1);   // Volver a detectar flanco ascendente
		Distancia_cm = width / 116.0;
	}
	

}


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
		
		char Buffer1[16];
		char Buffer2[16];
		char str_temp[10];
		char str_hum[10];
		char str_grados[10];
		char str_led_adc[10];
		
		Add_to_string(str_grados, grados);

		// Disparo del ultrasonido
		PORTD |= (1 << PIN_T);
		_delay_us(15);
		PORTD &= ~(1 << PIN_T);


		uint8_t color_code = 0x00;
		char str_dist[10];
		//Distancia_cm = 30;
		// ======= 6 niveles de distancia =======
		if (Distancia_cm < 10){
			color_code = 0xB9;   // rojo intenso
			str_dist[0]= '\0';
			strcat(str_dist, "Rojo");
		}
		else if (Distancia_cm < 40){
			color_code = 0xBA;   // naranja
			str_dist[0]= '\0';
			strcat(str_dist, "Naranja");
		}
		else if (Distancia_cm < 90){
			color_code = 0xBB;   // amarillo
			str_dist[0]= '\0';
			strcat(str_dist, "Amarillo");
		}
		else if (Distancia_cm < 150){
			color_code = 0xBC;   // verde
			str_dist[0]= '\0';
			strcat(str_dist, "Verde");
		}
		else if (Distancia_cm < 300){
			color_code = 0xBD;   // celeste
			str_dist[0]= '\0';
			strcat(str_dist, "Celeste");
		}
		else{
			color_code = 0xBF;   // azul
			str_dist[0]= '\0';
			strcat(str_dist, "Azul");
		}
		// Enviar el código al esclavo
		I2C_start();
		I2C_write(0x50 << 1);   // dirección esclavo + Write
		I2C_write(color_code);
		I2C_stop();
		
		
		_delay_us(100);
		
		Buffer1[0]= '\0';
		Buffer2[0]= '\0';
		
		//if(escribirLCD){
		strcat(Buffer1, "G");
		strcat(Buffer1, str_grados);
		strcat(Buffer1, "   ");
		
		char d[10];
		strcat(Buffer2, "D");
		strcat(Buffer2, str_dist);	
		strcat(Buffer2, "  ");
		strcat(Buffer2, Add_to_string(d, Distancia_cm));
		strcat(Buffer2, "  ");
		twi_lcd_cmd(0x80);
		twi_lcd_msg(Buffer1);
		twi_lcd_cmd(0xC0);
		twi_lcd_msg(Buffer2);
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




// Sirve para detectar las posibles direcciones del modulo I2C
void pcf8574_autodetect(void) {
	for (uint8_t a=0x20; a<=0x27; a++) {
		I2C_start();
		uint8_t ok = I2C_write((a<<1) | 0); // write
		I2C_stop();
		if (ok) PCF_ADDR = a;
	}
	for (uint8_t a=0x38; a<=0x3F; a++) {
		I2C_start();
		uint8_t ok = I2C_write((a<<1) | 0);
		I2C_stop();
		if (ok) PCF_ADDR = a;
	}
	return 0; // no encontrado
}

// LCDDDDDD
void PCF8574_write(uint8_t b) {
	I2C_start();
	I2C_write((PCF_ADDR<<1) | 0); // Direccion del modulo y 0 de lectura
	I2C_write(b); // Mandar el comando/data y si es escritura o lectura
	I2C_stop();
}


/* Function to Write to LCD Command Register */
void twi_lcd_cmd(const unsigned char x)
{
	lcd = 0x08;									//--- Enable Backlight Pin
	lcd &= ~(0x01);								//--- Select Command Register By RS = 0
	PCF8574_write(lcd);							//--- Send Data From PCF8574 to LCD PORT
	twi_lcd_4bit_send(x);						//--- Function to Write 4-bit data to LCD
}


/* Function to Write to LCD Command Register */
void twi_lcd_dwr(unsigned char x)
{
	lcd |= 0x09;								//--- Enable Backlight Pin & Select Data Register By RS = 1
	PCF8574_write(lcd);							//--- Send Data From PCF8574 to LCD PORT
	twi_lcd_4bit_send(x);						//--- Function to Write 4-bit data to LCD
}

/* Function to Send String of Data */
void twi_lcd_msg(const char *c)
{
	while (*c != '\0')							//--- Check Pointer for Null
	twi_lcd_dwr(*c++);							//--- Send the String of Data
}

/* Function to Execute Clear LCD Command */
void twi_lcd_clear()
{
	twi_lcd_cmd(0x01);
}

/* Function to Write 4-bit data to LCD */
void twi_lcd_4bit_send(unsigned char x)
{
	unsigned char temp = 0x00;					//--- temp variable for data operation
	lcd &= 0x0F;								//--- Masking last four bit to prevent the RS, RW, EN, Backlight
	temp = (x & 0xF0);							//--- Masking higher 4-Bit of Data and send it LCD
	lcd |= temp;								//--- 4-Bit Data and LCD control Pin
	lcd |= (0x04);								//--- Latching Data to LCD EN = 1
	PCF8574_write(lcd);							//--- Send Data From PCF8574 to LCD PORT
	_delay_us(1);								//--- 1us Delay
	lcd &= ~(0x04);								//--- Latching Complete
	PCF8574_write(lcd);							//--- Send Data From PCF8574 to LCD PORT
	_delay_us(5);								//--- 5us Delay to Complete Latching
	temp = ((x & 0x0F)<<4);						//--- Masking Lower 4-Bit of Data and send it LCD
	lcd &= 0x0F;								//--- Masking last four bit to prevent the RS, RW, EN, Backlight
	lcd |= temp;								//--- 4-Bit Data and LCD control Pin
	lcd |= (0x04);								//--- Latching Data to LCD EN = 1
	PCF8574_write(lcd);							//--- Send Data From PCF8574 to LCD PORT
	_delay_us(1);								//--- 1us Delay
	lcd &= ~(0x04);								//--- Latching Complete
	PCF8574_write(lcd);							//--- Send Data From PCF8574 to LCD PORT
	_delay_us(5);								//--- 5us Delay to Complete Latching
	
}

/* Function to Initialize LCD in 4-Bit Mode, Cursor Setting and Row Selection */
void twi_lcd_init()
{
	pcf8574_autodetect();
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
}


void lcd_twolines(char c, char b){
	twi_lcd_cmd(0x80);
	twi_lcd_msg(c);
	twi_lcd_cmd(0xC0);
	twi_lcd_msg(b);
}



char* Add_to_string(char *out, uint16_t val){
	if (val == 0) { out[0] = '0'; out[1] = '\0'; return out; }

	char tmp[5];                 // holds digits in reverse (max 5 for uint16_t)
	uint8_t n = 0;

	while (val) {
		uint8_t digit = val % 10;
		tmp[n++] = Number_to_ascii(digit);
		val /= 10;
	}

	// reverse into out
	for (uint8_t i = 0; i < n; ++i) out[i] = tmp[n - 1 - i];
	out[n] = '\0';
	return out;
}


char Number_to_ascii(uint16_t val){
	switch (val) {
		case 0: return '0';
		case 1: return '1';
		case 2: return '2';
		case 3: return '3';
		case 4: return '4';
		case 5: return '5';
		case 6: return '6';
		case 7: return '7';
		case 8: return '8';
		case 9: return '9';
		
		default: return '?';
	}
}


void Init_timer1(void) {
	TCCR1A = 0;  // Modo normal (WGM11:0 = 0)

	// ICNC1=1 (antirruido), ICES1=1 (flanco ascendente), CS11=1 (prescaler /8)
	TCCR1B = (1 << ICNC1) | (1 << ICES1) | (1 << CS11);

	TCNT1 = 0;           // Reiniciar contador
	TIMSK1 = (1 << ICIE1); // Habilitar interrupción de captura
	sei();                // Habilitar interrupciones globales
}
