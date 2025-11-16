#include "Liberia.h"

// ======================================
// Variables globales internas
// ======================================

// DHT crudo (si en algún momento los querés usar)
uint8_t Hum = 0, Humdec = 0, Temp = 0, Tdec = 0, Checksum = 0;

// LCD / I2C
uint8_t PCF_ADDR = 0x27;      // Dirección por defecto del PCF8574
unsigned char lcd = 0x00;     // Registro de control local del LCD

// Buffers UART
volatile char    serialBuffer[TX_BUFFER_SIZE];
volatile uint8_t serialReadPos  = 0;
volatile uint8_t serialWritePos = 0;

volatile char    rxBuffer[RX_BUFFER_SIZE];
volatile uint8_t rxReadPos  = 0;
volatile uint8_t rxWritePos = 0;

// Sensor distancia
uint16_t Distancia_cm = 0;
uint16_t endd = 0;
uint16_t start = 0;
uint16_t width = 0;

// ======================================
// ISRs
// ======================================

// Captura de pulso del HC-SR04 (entrada en ICP1 / PB0 si corresponde)
ISR(TIMER1_CAPT_vect) {
	if (TCCR1B & (1 << ICES1)) {
		// Flanco ascendente: guardar inicio y cambiar a descendente
		start = ICR1;
		TCCR1B &= ~(1 << ICES1);
		} else {
		// Flanco descendente: guardar fin y calcular duración
		endd = ICR1;
		width = endd - start;
		TCCR1B |= (1 << ICES1);
		Distancia_cm = width / 116.0;
	}
}

// Recepción UART
ISR(USART_RX_vect){
	rxBuffer[rxWritePos] = UDR0;
	rxWritePos++;

	if (rxWritePos >= RX_BUFFER_SIZE) {
		rxWritePos = 0;
	}
}

// Data Register Empty UART
ISR(USART_UDRE_vect){
	if (serialReadPos != serialWritePos){
		UDR0 = serialBuffer[serialReadPos];
		serialReadPos = (serialReadPos + 1) % TX_BUFFER_SIZE;
		} else {
		UCSR0B &= ~(1 << UDRIE0);  // Nada más que enviar
	}
}

// ======================================
// SPI
// ======================================

void spi_init(void) {
	DDRB |= (1 << CS) | (1 << MOSI) | (1 << SCK); // SS, MOSI, SCK salidas
	DDRB &= ~(1 << MISO);                         // MISO entrada

	SPCR = (1 << SPE) | (1 << MSTR); // Habilita SPI en modo maestro
}

uint8_t spi_transfer(uint8_t data) {
	SPDR = data;
	while (!(SPSR & (1 << SPIF)));
	return SPDR;
}

void SS_HIGH(void) { PORTB |=  (1 << CS); }
void SS_LOW(void)  { PORTB &= ~(1 << CS); }

// ======================================
// I2C
// ======================================

void I2C_init(void) {
	TWSR = 0x00;            // Prescaler = 1
	TWBR = ((F_CPU / 100000UL) - 16) / 2;  // 100 kHz
	TWCR = (1<<TWEN);       // Habilitar TWI
}

void I2C_start(void) {
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT))); // Espera a que salte TWINT
}

void I2C_stop(void) {
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
}

uint8_t I2C_write(uint8_t v) {
	TWDR = v;
	TWCR = (1<<TWINT)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
	uint8_t s = TWSR & 0xF8;
	// 0x18: SLA+W ACK, 0x28: DATA ACK
	return (s == 0x18 || s == 0x28);
}

// ======================================
// LCD por I2C (PCF8574)
// ======================================

void pcf8574_autodetect(void) {
	for (uint8_t a = 0x20; a <= 0x27; a++) {
		I2C_start();
		uint8_t ok = I2C_write((a<<1) | 0); // write
		I2C_stop();
		if (ok) PCF_ADDR = a;
	}
	for (uint8_t a = 0x38; a <= 0x3F; a++) {
		I2C_start();
		uint8_t ok = I2C_write((a<<1) | 0);
		I2C_stop();
		if (ok) PCF_ADDR = a;
	}
}

void PCF8574_write(uint8_t b) {
	I2C_start();
	I2C_write((PCF_ADDR<<1) | 0); // Dirección del módulo y W
	I2C_write(b);
	I2C_stop();
}

/* Function to Write to LCD Command Register */
void twi_lcd_cmd(const unsigned char x)
{
	lcd = 0x08;                     // Backlight ON
	lcd &= ~(0x01);                 // RS = 0 (comando)
	PCF8574_write(lcd);
	twi_lcd_4bit_send(x);
}

/* Function to Write to LCD Data Register */
void twi_lcd_dwr(unsigned char x)
{
	lcd |= 0x09;                    // Backlight + RS = 1 (datos)
	PCF8574_write(lcd);
	twi_lcd_4bit_send(x);
}

/* Function to Send String of Data */
void twi_lcd_msg(const char *c)
{
	while (*c != '\0')
	twi_lcd_dwr(*c++);
}

/* Function to Execute Clear LCD Command */
void twi_lcd_clear(void)
{
	twi_lcd_cmd(0x01);
}

/* Function to Write 4-bit data to LCD */
void twi_lcd_4bit_send(unsigned char x)
{
	unsigned char temp = 0x00;
	lcd &= 0x0F;                     // Mantener RS, RW, EN, Backlight
	temp = (x & 0xF0);               // Parte alta
	lcd |= temp;
	lcd |= (0x04);                   // EN = 1
	PCF8574_write(lcd);
	_delay_us(1);
	lcd &= ~(0x04);                  // EN = 0
	PCF8574_write(lcd);
	_delay_us(5);

	temp = ((x & 0x0F)<<4);          // Parte baja
	lcd &= 0x0F;
	lcd |= temp;
	lcd |= (0x04);
	PCF8574_write(lcd);
	_delay_us(1);
	lcd &= ~(0x04);
	PCF8574_write(lcd);
	_delay_us(5);
}

/* Inicialización LCD */
void twi_lcd_init(void)
{
	pcf8574_autodetect();
	lcd = 0x04;                      // EN = 1 para secuencia init
	PCF8574_write(lcd);
	_delay_us(25);

	twi_lcd_cmd(0x03);
	twi_lcd_cmd(0x03);
	twi_lcd_cmd(0x03);
	twi_lcd_cmd(0x02);              // 4 bits
	twi_lcd_cmd(0x28);              // 4 bits, 2 líneas
	twi_lcd_cmd(0x0F);              // Cursor on, blink
	twi_lcd_cmd(0x01);              // Clear
	twi_lcd_cmd(0x06);              // Auto-incremento
	twi_lcd_cmd(0x80);              // Posición inicial
	twi_lcd_msg("Initializing...");
	_delay_ms(1000);
	twi_lcd_clear();
	twi_lcd_cmd(0x80);
}

void lcd_twolines(const char *line1, const char *line2){
	twi_lcd_cmd(0x80);
	twi_lcd_msg(line1);
	twi_lcd_cmd(0xC0);
	twi_lcd_msg(line2);
}

// ======================================
// UART
// ======================================

void uart_init(unsigned int ubrr) {
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = (1<<TXEN0) | (1<<RXEN0);
	UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);
}

void uart_send(char c) {
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = c;
}

void uart_print(const char *s) {
	while (*s) uart_send(*s++);
}

void uart_print_hex(uint8_t val) {
	char buf[6];
	sprintf(buf, "0x%02X", val);
	uart_print(buf);
}

void serialWrite(const char *s){
	for (uint8_t i = 0; i < (uint8_t)strlen(s); i++){
		serialBuffer[serialWritePos] = s[i];
		serialWritePos = (serialWritePos + 1) % TX_BUFFER_SIZE;
	}
	UCSR0B |= (1 << UDRIE0);   // Habilita ISR UDRE
}

char Chardos(void)
{
	char ret = '\0';

	if (rxReadPos != rxWritePos)
	{
		ret = rxBuffer[rxReadPos];

		rxReadPos++;

		if (rxReadPos >= RX_BUFFER_SIZE)
		{
			rxReadPos = 0;
		}
	}

	return ret;
}

// ======================================
// PWM (servo / salida general en OC0A)
// ======================================

void Init_pwm(void){
	DDRD |= (1 << DDD6);

	// Fast PWM (modo 3, TOP=255), salida no inversora en OC0A
	TCCR0A = (1 << WGM01) | (1 << WGM00) | (1 << COM0A1);
	TCCR0B = (1 << CS01) | (1 << CS00); // Prescaler = 64 (~976 Hz)
}

// ======================================
// Sensor Distancia / Timer1
// ======================================

void Init_timer1(void) {
	TCCR1A = 0;  // Modo normal

	// ICNC1=1 (antirruido), ICES1=1 (flanco ascendente), CS11=1 (prescaler /8)
	TCCR1B = (1 << ICNC1) | (1 << ICES1) | (1 << CS11);

	TCNT1  = 0;
	TIMSK1 = (1 << ICIE1); // Habilitar interrupción de captura
	sei();                 // Habilitar interrupciones globales
}

// ======================================
// ADC
// ======================================

void Init_adc(void) {
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Enable, prescaler 128
	DIDR0  = (1 << ADC0D) | (1 << ADC1D);
}

uint16_t adc_read_AC0(void) {
	ADMUX  = (1 << REFS0);  // AVcc, canal ADC0
	ADCSRA |= (1 << ADSC);  // Start
	while (ADCSRA & (1 << ADSC));
	return ADC;
}

uint16_t adc_read_AC1(void) {
	ADMUX  = (1 << REFS0)|(1 << MUX0);   // AVcc, canal ADC1
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADC;
}

// ======================================
// Utilidades conversión
// ======================================

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

char* Add_to_string(char *out, uint16_t val){
	if (val == 0) {
		out[0] = '0';
		out[1] = '\0';
		return out;
	}

	char tmp[5];
	uint8_t n = 0;

	while (val) {
		uint8_t digit = val % 10;
		tmp[n++] = Number_to_ascii(digit);
		val /= 10;
	}

	for (uint8_t i = 0; i < n; ++i)
	out[i] = tmp[n - 1 - i];

	out[n] = '\0';
	return out;
}

bool ascii_to_u16_switch(const char *s, uint16_t *out) {
	uint8_t digs[5];
	uint8_t n = 0;

	for (; *s && *s != '\r' && *s != '\n'; ++s) {
		char c = *s;
		uint8_t d;

		switch (c) {
			case '0': d = 0; break;
			case '1': d = 1; break;
			case '2': d = 2; break;
			case '3': d = 3; break;
			case '4': d = 4; break;
			case '5': d = 5; break;
			case '6': d = 6; break;
			case '7': d = 7; break;
			case '8': d = 8; break;
			case '9': d = 9; break;
			default: return false;
		}

		if (n >= 5) return false;
		digs[n++] = d;
	}

	if (n == 0) return false;

	uint32_t val = 0;
	uint32_t mult = 1;

	for (int8_t i = (int8_t)n - 1; i >= 0; --i) {
		val += (uint32_t)digs[i] * mult;
		mult *= 10;
	}

	if (val > 65535u) return false;

	*out = (uint16_t)val;
	return true;
}

// ======================================
// DHT11 (versión compacta en PD7)
// ======================================

bool dht11_read2(uint8_t *t, uint8_t *h){
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
