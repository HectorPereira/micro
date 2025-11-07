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

#define LCD_EN          0x04   // Enable
#define LCD_RW          0x02   // Read/Write
#define LCD_RS          0x01   // Register Select
#define LCD_BACKLIGHT   0x08   // Retroiluminación

uint8_t nibble_to_bus(uint8_t nibble);
uint8_t pcf8574_autodetect(void);
void pcf8574_write(uint8_t b);
void lcd_strobe(uint8_t data);
void lcd_write4(uint8_t nibble, uint8_t rs);
void lcd_send(uint8_t value, uint8_t rs);
void lcd_cmd(uint8_t c);
void lcd_data(uint8_t d);
void lcd_clear(void);
void lcd_set_cursor(uint8_t col, uint8_t row);
void lcd_print(const char *s);
void lcd_init(void);
void lcd_msg2(const char* l1, const char* l2);
char poll_switch_portc(void);

int main(void)
{
	sei();
	
	I2C_init();
	spi_init();
	SS_LOW();
	spi_transfer(0xF1);
	SS_HIGH();
	
    while(1)
    {
       
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