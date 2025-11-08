#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <util/twi.h>

// ======================================
// Pines SPI
// ======================================

#define CS   PB2 //SS del Arduino Slave
#define MOSI PB3
#define MISO PB4
#define SCK  PB5


// ======================================
// Protocolos SPI
// ======================================


void spi_init(void);
uint8_t spi_transfer(uint8_t data);
void SS_HIGH(void);
void SS_LOW(void);

// ======================================
// I2C Inicializacion
// ======================================
void I2C_init(void);
void I2C_start(void);
void I2C_stop(void);
uint8_t I2C_write(uint8_t v);


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
// UART
// ======================================

// Configuración general del sistema

#define BAUD           9600UL                     // Velocidad UART
#define UBRR_VALUE     ((F_CPU/16/BAUD) - 1)      // Valor de UBRR según BAUD
#define TX_BUFFER_SIZE 128                        // Tamaño del buffer TX
#define RX_BUFFER_SIZE 128                        // Tamaño del buffer RX
#define precarger      10000                      // Valor de precarga (genérico)


// Buffers de comunicación UART
volatile char    serialBuffer[TX_BUFFER_SIZE];  // Buffer de transmisión UART
volatile uint8_t serialReadPos  = 0;            // Posición de lectura en buffer TX
volatile uint8_t serialWritePos = 0;            // Posición de escritura en buffer TX

volatile char    rxBuffer[RX_BUFFER_SIZE];      // Buffer de recepción UART
volatile uint8_t rxReadPos  = 0;                // Posición de lectura en buffer RX
volatile uint8_t rxWritePos = 0;                // Posición de escritura en buffer RX


// Macros generales

#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))   // Set bit
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))  // Clear bit
#define UART_TIMEOUT_MS 500                          // Tiempo máximo UART en ms

// Comunicación UART

void uart_init(unsigned int ubrr);
void uart_send(char c);
void uart_print(const char *s);
void uart_print_hex(uint8_t val);
void serialWrite(const char *s);
char Chardos(void);

// ======================================
// DHT
// ======================================



#define DHT_PIN PORTD3
#define FIRE_PIN PORTD2


void DHT_start(void);

uint8_t DHT_response(void);

uint8_t DHT_read(void);

// Conversión y utilidades
char Number_to_ascii(uint8_t val);       // Convierte un número (0–9) en carácter ASCII
bool ascii_to_u16_switch(const char *s, uint16_t *out); // Convierte texto numérico a entero de 16 bits
void add_string(char *s, char c);

void Init_pwm(void);        // Configura PWM en OC0A (pin D6),
void Init_adc(void);        // Configura el ADC (canal A1)
uint16_t adc_read_AC0(void); 


// ======================================
// ISR,s
// ======================================


ISR(USART_RX_vect){
	rxBuffer[rxWritePos] = UDR0;
	rxWritePos++;

	if (rxWritePos >= RX_BUFFER_SIZE)
	{
		rxWritePos = 0;
	}
}

ISR(USART_UDRE_vect){
	if (serialReadPos != serialWritePos){
		UDR0 = serialBuffer[serialReadPos];
		serialReadPos = (serialReadPos + 1) % TX_BUFFER_SIZE;
		} else {
		UCSR0B &= ~(1 << UDRIE0);  // nada m?s que enviar
	}
}



int main(void)
{
	uint8_t Temp = 0;
	uint8_t Hum = 0;
	uint8_t Humdec;
	uint8_t Tdec;
	
	DDRD |= (1 << FIRE_PIN);
	PORTD &= ~(1 << FIRE_PIN);
	
	sei();
	
	uart_init(UBRR_VALUE);
	
	// RX por interrupci?n
	UCSR0B |= (1<<RXCIE0);
	
	spi_init();	
	Init_adc();
	Init_pwm();
	
	
	//I2C_init();
	//twi_lcd_init();

	
	//lcd_twolines("Bienvenido", "A - Encender");
 	uart_print("Inserte A para prender\n\r");

    while(1)
    {
			

		uint16_t buffer_Ac0 = adc_read_AC0();
		
		if(buffer_Ac0 > 250){
 			SS_LOW();
 			spi_transfer(0x01);
 			SS_HIGH();
		}
		else {
			SS_LOW();
			spi_transfer(0x00);
			SS_HIGH();
		}
		
		char c = Chardos();
		if(c == 'A'){
			
 		lcd_twolines("Leyendo", "Temperatura");
 		uart_print("Leyendo Temperatura\n\r");
 		
 		DHT_start();
 		if (DHT_response()) {
 			Hum = DHT_read();
 			Humdec = DHT_read();
 			Temp = DHT_read();
 			Tdec = DHT_read();
 			PORTD |= (1 << PORTD3);
 		}
 		
 		// IMPLEMENTAR Conversion a ascii
 			
 		uart_print("\n\r");
 		uart_print_hex(Hum);
 		uart_print("\n\r");
 		uart_print_hex(Temp);
		
		uint16_t indice = 4; //Cada grado varia en 4 el PWM entonces se va a apreciar entre los 0 y 64
							// El esclavo va a leer hexa, entonces el hexa a leer debe estar entre 0 y 256
		uint8_t transferir_dht =	Temp*indice;
		 
		SS_LOW();
		spi_transfer(transferir_dht);
		SS_HIGH();
		
		}
		if(c == 'B'){
			// Conectado al sensor de fuego para sonar un buzzer y prender una led
			
			uart_print("Mueva el potenciometro para variar \n\r");
			
			while (c != 'O')
			{
				c = Chardos();
				
				
			}
			

			
		}
		if(c == 'C'){
			// Conectado al sensor de distancia y si se puede variar el color de una rgb
			SS_LOW();
			spi_transfer(0xAB);
			SS_HIGH();
			
		}
		       
    }
}



// ======================================
// FUnciones SPI
// ======================================


void spi_init(void) {
	DDRB |= (1 << CS) | (1 << MOSI) | (1 << SCK); // SS, MOSI, SCK salidas
	DDRB &= ~(1 << MISO);                         // MISO entrada

	SPCR = (1 << SPE) | (1 << MSTR)| (1 << SPR0); // Habilita SPI en modo maestro
	SPSR = (1 << SPI2X);              // fosc/8
}

uint8_t spi_transfer(uint8_t data) {
	SPDR = data;
	while (!(SPSR & (1 << SPIF)));
	return SPDR;
}

void SS_HIGH(void) { PORTB |=  (1 << CS); }
void SS_LOW(void)  { PORTB &= ~(1 << CS); }


// ======================================
// FUnciones I2c
// ======================================


void I2C_init(void) {
	TWSR = 0x00;            // Prescaler = 1
	TWBR = ((F_CPU / 100000UL) - 16) / 2;  // 100 kHz
	TWCR = (1<<TWEN);       // Habilitar TWI
}

void I2C_start(void) {
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN); // TWI Interrupt Flag / TWI START Condition Bit / TWI Interrupt Enable
	while(!(TWCR & (1<<TWINT))); // Espera a que salte TWINT
}

void I2C_stop(void) {
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN); // TWSTO STOP Condition Bit
}

uint8_t I2C_write(uint8_t v) {
	TWDR = v;
	TWCR = (1<<TWINT)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
	uint8_t s = TWSR & 0xF8;
	// 0x18: SLA+W ACK, 0x28: DATA ACK  (Confirmacion  de que se encontro el esclavo o de que se escribio correctamente)
	return (s == 0x18 || s == 0x28); 
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


// ======================================
// Funciones UART 
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
	UCSR0B |= (1 << UDRIE0);   // habilita ISR UDRE
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


void DHT_start(void) {
	DDRD |= (1 << DHT_PIN);       // Configurar pin como salida
	PORTD &= ~(1 << DHT_PIN);     // Enviar señal de inicio (bajo)
	_delay_ms(18);                // Esperar al menos 18 ms
	PORTD |= (1 << DHT_PIN);      // Liberar la línea
	_delay_us(40);                // Esperar 40 µs
}

uint8_t DHT_response(void) {
	uint8_t response = 0;
	DDRD &= ~(1 << DHT_PIN);      // Configurar como entrada
	_delay_us(40);

	if (!(PIND & (1 << DHT_PIN))) {  // Esperar respuesta baja
		_delay_us(80);
		if (PIND & (1 << DHT_PIN)) { // Esperar respuesta alta
			_delay_us(80);
			response = 1;            // Respuesta válida
		}
	}
	return response;                // 1 = ok, 0 = sin respuesta
}

uint8_t DHT_read(void) {
	uint8_t result = 0;
	for (int i = 0; i < 8; i++) {
		while (!(PIND & (1 << DHT_PIN)));  // Esperar pulso alto
		_delay_us(30);                     // Esperar 30 µs
		if (PIND & (1 << DHT_PIN))         // Si sigue alto, es 1
		result |= (1 << (7 - i));
		while (PIND & (1 << DHT_PIN));     // Esperar pulso bajo
	}
	return result;
}


char Number_to_ascii(uint8_t val){
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

void add_string(char *s, char c) {
	while (*s++);
	*(s - 1) = c;
	*s = '\0';
}


void Init_pwm(void){
	DDRD |= (1 << DDD6);

	// Fast PWM (modo 3, TOP=255), salida no inversora en OC0A
	TCCR0A = (1 << WGM01) | (1 << WGM00) | (1 << COM0A1); // COM0A1=1, COM0A0=0
	TCCR0B = (1 << CS01) | (1 << CS00); // Prescaler = 64  (? 976 Hz @16MHz)
}


// ======================================
// Funciones sensor flama
// ======================================



void Init_adc(void) {
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Enable, prescaler 128
	DIDR0  = (1 << ADC0D);  // Disable digital buffer on ADC0
}

uint16_t adc_read_AC0(void) {
	ADMUX  = (1 << REFS0);  // AVcc reference, MUX=0000 (ADC0)
	ADCSRA |= (1 << ADSC);  // Start conversion
	while (ADCSRA & (1 << ADSC));  // Wait until finished
	return ADC;  // Read result
}
