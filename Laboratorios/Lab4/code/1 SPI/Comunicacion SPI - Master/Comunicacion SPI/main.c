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

uint8_t nibble_to_bus(uint8_t nibble);
void pcf8574_autodetect(void);
void PCF8574_write(uint8_t b);

/* Function to Write to LCD Command Register */
void twi_lcd_cmd(const unsigned char x);

/* Function to Write to LCD Command Register */
void twi_lcd_dwr(unsigned char x);
/* Function to Send String of Data */
void twi_lcd_msg(const char *c);
/* Function to Execute Clear LCD Command */
void twi_lcd_clear();
/* Function to Write 4-bit data to LCD */
void twi_lcd_4bit_send(unsigned char x);
/* Function to Initialize LCD in 4-Bit Mode, Cursor Setting and Row Selection */
void twi_lcd_init();

unsigned char lcd = 0x00;	

int main(void)
{
	sei();
	
	spi_init();	
	//pruebra de SPI
	SS_LOW();
	spi_transfer(0xF1);
	SS_HIGH();
	
	
	I2C_init();
	twi_lcd_init();

	
	
    while(1)
    {
       //--- Select 2nd Row
       twi_lcd_cmd(0xC0);
       //--- Send a String to LCD
       twi_lcd_msg("Linea 2!!");
       
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