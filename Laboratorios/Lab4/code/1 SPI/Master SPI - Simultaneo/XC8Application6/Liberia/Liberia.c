#include "Liberia.h"
#include <util/twi.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>

uint8_t Hum = 0, Humdec = 0, Temp = 0, Tdec = 0, Checksum = 0;
uint8_t PCF_ADDR = 0x27;
unsigned char lcd = 0x00;

volatile char serialBuffer[TX_BUFFER_SIZE];
volatile uint8_t serialReadPos = 0, serialWritePos = 0;
volatile char rxBuffer[RX_BUFFER_SIZE];
volatile uint8_t rxReadPos = 0, rxWritePos = 0;

uint16_t Distancia_cm = 0;
uint16_t count_t2 = 0;
uint8_t leer_dht = 0;

uint16_t endd = 0, start = 0, width = 0;

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

ISR(USART_RX_vect){
	rxBuffer[rxWritePos] = UDR0;
	rxWritePos++;
	if (rxWritePos >= RX_BUFFER_SIZE) rxWritePos = 0;
}

ISR(USART_UDRE_vect){
	if (serialReadPos != serialWritePos){
		UDR0 = serialBuffer[serialReadPos];
		serialReadPos = (serialReadPos + 1) % TX_BUFFER_SIZE;
		} else {
		UCSR0B &= ~(1 << UDRIE0);
	}
}

void spi_init(void) {
	DDRB |= (1 << CS) | (1 << MOSI) | (1 << SCK);
	DDRB &= ~(1 << MISO);
	SPCR = (1 << SPE) | (1 << MSTR);
}

uint8_t spi_transfer(uint8_t data) {
	SPDR = data;
	while (!(SPSR & (1 << SPIF)));
	return SPDR;
}

void SS_HIGH(void) { PORTB |= (1 << CS); }
void SS_LOW(void)  { PORTB &= ~(1 << CS); }

void I2C_init(void) {
	TWSR = 0x00;
	TWBR = ((F_CPU / 100000UL) - 16) / 2;
	TWCR = (1<<TWEN);
}

void I2C_start(void) {
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while(!(TWCR&(1<<TWINT)));
}

void I2C_stop(void) {
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
}

uint8_t I2C_write(uint8_t v) {
	TWDR = v;
	TWCR = (1<<TWINT)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
	uint8_t s = TWSR & 0xF8;
	return (s == 0x18 || s == 0x28);
}

void pcf8574_autodetect(void) {
	for (uint8_t a=0x20; a<=0x27; a++) {
		I2C_start(); uint8_t ok = I2C_write((a<<1)|0); I2C_stop();
		if (ok) PCF_ADDR = a;
	}
	for (uint8_t a=0x38; a<=0x3F; a++) {
		I2C_start(); uint8_t ok = I2C_write((a<<1)|0); I2C_stop();
		if (ok) PCF_ADDR = a;
	}
}

void PCF8574_write(uint8_t b) {
	I2C_start();
	I2C_write((PCF_ADDR<<1)|0);
	I2C_write(b);
	I2C_stop();
}

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
	while (*c) twi_lcd_dwr(*c++);
}

void twi_lcd_clear(void) {
	twi_lcd_cmd(0x01);
}

void twi_lcd_4bit_send(unsigned char x) {
	unsigned char temp = x & 0xF0;
	lcd &= 0x0F;
	lcd |= temp;
	lcd |= 0x04;
	PCF8574_write(lcd);
	_delay_us(1);
	lcd &= ~0x04;
	PCF8574_write(lcd);
	_delay_us(5);

	temp = (x & 0x0F)<<4;
	lcd &= 0x0F;
	lcd |= temp;
	lcd |= 0x04;
	PCF8574_write(lcd);
	_delay_us(1);
	lcd &= ~0x04;
	PCF8574_write(lcd);
	_delay_us(5);
}

void twi_lcd_init(void) {
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

void lcd_twolines(const char *c, const char *b){
	twi_lcd_cmd(0x80);
	twi_lcd_msg(c);
	twi_lcd_cmd(0xC0);
	twi_lcd_msg(b);
}

void uart_init(unsigned int ubrr) {
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = (1<<TXEN0)|(1<<RXEN0);
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
}

void uart_send(char c) {
	while(!(UCSR0A&(1<<UDRE0)));
	UDR0=c;
}

void uart_print(const char *s) {
	while(*s) uart_send(*s++);
}

void uart_print_hex(uint8_t val) {
	char buf[6];
	sprintf(buf,"0x%02X",val);
	uart_print(buf);
}

void serialWrite(const char *s){
	for (uint8_t i=0;i<strlen(s);i++){
		serialBuffer[serialWritePos]=s[i];
		serialWritePos=(serialWritePos+1)%TX_BUFFER_SIZE;
	}
	UCSR0B |= (1<<UDRE0);
}

char Chardos(void){
	char ret='\0';
	if(rxReadPos!=rxWritePos){
		ret=rxBuffer[rxReadPos];
		rxReadPos++;
		if(rxReadPos>=RX_BUFFER_SIZE) rxReadPos=0;
	}
	return ret;
}

void Init_pwm(void){
	DDRD |= (1<<DDD6);
	TCCR0A = (1<<WGM01)|(1<<WGM00)|(1<<COM0A1);
	TCCR0B = (1<<CS01)|(1<<CS00);
}

void Init_timer1(void) {
	TCCR1A=0;
	TCCR1B=(1<<ICNC1)|(1<<ICES1)|(1<<CS11);
	TCNT1=0;
	TIMSK1=(1<<ICIE1);
	sei();
}

void timer2_init(void){
	TCCR2A = (1 << WGM21);
	TCCR2B = (1 << CS22)|(1<<CS21)|(1<<CS20);
	OCR2A  = 255;
	TIMSK2 = (1 << OCIE2A);
}

void Init_adc(void) {
	ADCSRA=(1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
	DIDR0=(1<<ADC0D)|(1<<ADC1D);
}

uint16_t adc_read_AC0(void) {
	ADMUX=(1<<REFS0);
	ADCSRA|=(1<<ADSC);
	while(ADCSRA&(1<<ADSC));
	return ADC;
}

uint16_t adc_read_AC1(void) {
	ADMUX=(1<<REFS0)|(1<<MUX0);
	ADCSRA|=(1<<ADSC);
	while(ADCSRA&(1<<ADSC));
	return ADC;
}

char Number_to_ascii(uint16_t val){
	switch(val){
		case 0:return '0'; case 1:return '1'; case 2:return '2';
		case 3:return '3'; case 4:return '4'; case 5:return '5';
		case 6:return '6'; case 7:return '7'; case 8:return '8';
		case 9:return '9';
		default:return '?';
	}
}

char* Add_to_string(char *out, uint16_t val){
	if(val==0){out[0]='0';out[1]='\0';return out;}
	char tmp[5]; uint8_t n=0;
	while(val){tmp[n++]=Number_to_ascii(val%10);val/=10;}
	for(uint8_t i=0;i<n;i++) out[i]=tmp[n-1-i];
	out[n]='\0'; return out;
}

bool ascii_to_u16_switch(const char *s,uint16_t *out){
	uint8_t digs[5],n=0;
	for(;*s && *s!='\r' && *s!='\n';s++){
		char c=*s; uint8_t d;
		switch(c){
			case '0':d=0;break; case '1':d=1;break; case '2':d=2;break;
			case '3':d=3;break; case '4':d=4;break; case '5':d=5;break;
			case '6':d=6;break; case '7':d=7;break; case '8':d=8;break;
			case '9':d=9;break;
			default:return false;
		}
		if(n>=5)return false;
		digs[n++]=d;
	}
	if(n==0)return false;
	uint32_t val=0,m=1;
	for(int8_t i=n-1;i>=0;i--){val+=digs[i]*m;m*=10;}
	if(val>65535u)return false;
	*out=(uint16_t)val;
	return true;
}

bool dht11_read2(uint8_t *t,uint8_t *h){
	uint8_t d[5]={0};
		
	DDRD|=(1<<DHT_PIN);
	PORTD&=~(1<<DHT_PIN); 
	_delay_ms(20);
	PORTD|=(1<<DHT_PIN);
	 _delay_us(40);
	 
	DDRD&=~(1<<DHT_PIN);  //Como entrada
	
	PORTD|=(1<<DHT_PIN);
	
	uint16_t to=0;
	
	while(PIND&(1<<DHT_PIN)){ 
		if(++to>200) 
		return false;
		_delay_us(1);
		}
		to=0;
	 while(!(PIND&(1<<DHT_PIN))){
		  if(++to>200)return false; 
		  _delay_us(1);}
		to=0; 
		while(PIND&(1<<DHT_PIN)){
			 if(++to>200)return false;
			  _delay_us(1);}
	for(uint8_t i=0;i<40;i++){
		to=0; 
		while(!(PIND&(1<<DHT_PIN))){ 
			if(++to>200)return false; 
			_delay_us(1);}
		uint16_t w=0; while(PIND&(1<<DHT_PIN)){ 
			if(++w>255)break;
			_delay_us(1);}
		d[i/8]<<=1; if(w>40) d[i/8]|=1;
	}
	if((uint8_t)(d[0]+d[1]+d[2]+d[3])!=d[4]) return false;
	*h=d[0]; *t=d[2];
	return true;
}
