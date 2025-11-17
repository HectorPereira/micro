#include "Liberia.h"

/* Variables globales */
uint8_t PCF_ADDR = 0x27;
unsigned char lcd = 0x00;

uint16_t Distancia_cm = 0;

uint16_t endd = 0;
uint16_t start = 0;
uint16_t width = 0;

uint8_t count_t2 = 0;
uint8_t leer_dht = 0;

/* ======================================
   ISR TIMER1 - HC-SR04
====================================== */
ISR(TIMER1_CAPT_vect) {
	if (TCCR1B & (1 << ICES1)) {
		start = ICR1;
		TCCR1B &= ~(1 << ICES1);
	} else {
		endd = ICR1;
		width = endd - start;
		TCCR1B |= (1 << ICES1);
		Distancia_cm = width / 116.0;
	}
}

ISR(TIMER2_COMPA_vect) {
	count_t2++;
	if (count_t2 >= 92) {
		count_t2 = 0;
		leer_dht = 1;
	}
}

/* === ADC === */

void Init_adc(void) {
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	DIDR0  = (1 << ADC0D) | (1 << ADC1D);
}

uint16_t adc_read_AC0(void) {
	ADMUX  = (1 << REFS0);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADC;
}

uint16_t adc_read_AC1(void) {
	ADMUX  = (1 << REFS0)|(1 << MUX0);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADC;
}

/* === I2C === */

void I2C_init(void) {
	TWSR = 0x00;
	TWBR = ((F_CPU / 100000UL) - 16) / 2;
	TWCR = (1 << TWEN);
}

void I2C_start(void) {
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
}

void I2C_stop(void) {
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
}

uint8_t I2C_write(uint8_t data) {
	TWDR = data;
	TWCR = (1<<TWINT)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
	uint8_t status = TWSR & 0xF8;
	return (status == 0x18 || status == 0x28);
}

/* detect pcf8574 */
void pcf8574_autodetect(void) {
	for (uint8_t a=0x20; a<=0x27; a++) {
		I2C_start();
		uint8_t ok = I2C_write((a<<1) | 0);
		I2C_stop();
		if (ok) PCF_ADDR = a;
	}
	for (uint8_t a=0x38; a<=0x3F; a++) {
		I2C_start();
		uint8_t ok = I2C_write((a<<1) | 0);
		I2C_stop();
		if (ok) PCF_ADDR = a;
	}
	return 0;
}

void PCF8574_write(uint8_t b) {
	I2C_start();
	I2C_write((PCF_ADDR<<1) | 0);
	I2C_write(b);
	I2C_stop();
}

/* LCD driver */
void twi_lcd_cmd(const unsigned char x) {
	lcd = 0x08;
	lcd &= ~(0x01);
	PCF8574_write(lcd);
	twi_lcd_4bit_send(x);
}

void twi_lcd_dwr(unsigned char x) {
	lcd |= 0x09;
	PCF8574_write(lcd);
	twi_lcd_4bit_send(x);
}

void twi_lcd_msg(const char *c) {
	while (*c != '\0')
		twi_lcd_dwr(*c++);
}

void twi_lcd_clear() {
	twi_lcd_cmd(0x01);
}

void twi_lcd_4bit_send(unsigned char x) {
	unsigned char temp = 0x00;
	lcd &= 0x0F;
	temp = (x & 0xF0);
	lcd |= temp;
	lcd |= (0x04);
	PCF8574_write(lcd);
	_delay_us(1);
	lcd &= ~(0x04);
	PCF8574_write(lcd);
	_delay_us(5);
	temp = ((x & 0x0F)<<4);
	lcd &= 0x0F;
	lcd |= temp;
	lcd |= (0x04);
	PCF8574_write(lcd);
	_delay_us(1);
	lcd &= ~(0x04);
	PCF8574_write(lcd);
	_delay_us(5);
}

void twi_lcd_init() {
	pcf8574_autodetect();
	lcd = 0x04;
	PCF8574_write(lcd);
	_delay_us(25);
	twi_lcd_cmd(0x03);
	twi_lcd_cmd(0x03);
	twi_lcd_cmd(0x03);
	twi_lcd_cmd(0x02);
	twi_lcd_cmd(0x28);
	twi_lcd_cmd(0x0F);
	twi_lcd_cmd(0x01);
	twi_lcd_cmd(0x06);
	twi_lcd_cmd(0x80);
	twi_lcd_msg("Initializing...");
	_delay_ms(1000);
	twi_lcd_clear();
	twi_lcd_cmd(0x80);
}

void lcd_twolines(char c, char b){
	twi_lcd_cmd(0x80);
	twi_lcd_msg(c);
	twi_lcd_cmd(0xC0);
	twi_lcd_msg(b);
}

/* Conversión numérica */

char* Add_to_string(char *out, uint16_t val){
	if (val == 0) { out[0] = '0'; out[1] = '\0'; return out; }
	char tmp[5];
	uint8_t n = 0;
	while (val) {
		uint8_t digit = val % 10;
		tmp[n++] = Number_to_ascii(digit);
		val /= 10;
	}
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

/* Timer1 */

void Init_timer1(void) {
	TCCR1A = 0;
	TCCR1B = (1 << ICNC1) | (1 << ICES1) | (1 << CS11);
	TCNT1 = 0;
	TIMSK1 = (1 << ICIE1);
	sei();
}

/* DHT11 */
#define DHT_PIN PD7
static bool dht11_read2(uint8_t *t, uint8_t *h){
	uint8_t d[5]={0};
	DDRD |= (1<<DHT_PIN); PORTD &= ~(1<<DHT_PIN); _delay_ms(20);
	PORTD |= (1<<DHT_PIN); _delay_us(40);
	DDRD &= ~(1<<DHT_PIN); PORTD |= (1<<DHT_PIN);
	uint16_t to=0;
	while(PIND&(1<<DHT_PIN)){ if(++to>200) return false; _delay_us(1); }
	to=0; while(!(PIND&(1<<DHT_PIN))){ if(++to>200) return false; _delay_us(1); }
	to=0; while(PIND&(1<<DHT_PIN)){ if(++to>200) return false; _delay_us(1); }
	for(uint8_t i=0;i<40;i++){
		to=0; while(!(PIND&(1<<DHT_PIN))){ if(++to>200) return false; _delay_us(1); }
		uint16_t w=0; while(PIND&(1<<DHT_PIN)){ if(++w>255) break; _delay_us(1); }
		d[i/8]<<=1; if(w>40) d[i/8]|=1;
	}
	if((uint8_t)(d[0]+d[1]+d[2]+d[3])!=d[4]) return false;
	*h=d[0]; *t=d[2]; return true;
}

/* Timer2 */
void timer2_init(void) {
	TCCR2A = (1 << WGM21);
	TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);
	OCR2A  = 255;
	TIMSK2 = (1 << OCIE2A);
}
