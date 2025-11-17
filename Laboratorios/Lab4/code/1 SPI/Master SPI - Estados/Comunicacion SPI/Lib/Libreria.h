#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <util/twi.h>


#ifndef LIBRERIA_H_
#define LIBRERIA_H_




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





// Macros generales

                    // Tiempo máximo UART en ms

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






void DHT_start(void);
bool DHT_response(void);
uint8_t DHT_read(void);


void Init_pwm(void);        // Configura PWM en OC0A (pin D6),
void Init_adc(void);        // Configura el ADC (canal A1)
uint16_t adc_read_AC0(void);
uint16_t adc_read_AC1(void);


// ======================================
// HC-SR04
// ======================================



void Init_timer1();

static bool dht11_read2(uint8_t *t, uint8_t *h);


// ======================================
// ISR,s
// ======================================



// Conversión y utilidades
char Number_to_ascii(uint16_t val);       // Convierte un número (0–9) en carácter ASCII
bool ascii_to_u16_switch(const char *s, uint16_t *out); // Convierte texto numérico a entero de 16 bits
char* Add_to_string(char *out, uint16_t val);



#endif /* LIBRERIA_H_ */