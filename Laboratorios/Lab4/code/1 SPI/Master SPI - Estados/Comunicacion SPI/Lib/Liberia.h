#ifndef MEGALIB_H_
#define MEGALIB_H_

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// ======================================
// Pines / Macros de hardware
// ======================================

// SPI
#define CS   PB2
#define MOSI PB3
#define MISO PB4
#define SCK  PB5

// DHT, luz, ultrasonido
#define DHT_PIN   PD7
#define LIGHT_PIN PD2
#define PIN_T     PD4
#define PIN_ECHO  PB0

// UART
#define BAUD           9600UL
#define UBRR_VALUE     ((F_CPU/16/BAUD) - 1)
#define TX_BUFFER_SIZE 128
#define RX_BUFFER_SIZE 128

#define UART_TIMEOUT_MS 500

// Macros generales
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif

// ======================================
// Variables globales visibles desde main
// ======================================

extern uint16_t Distancia_cm;

// ======================================
// SPI
// ======================================

void spi_init(void);
uint8_t spi_transfer(uint8_t data);
void SS_HIGH(void);
void SS_LOW(void);

// ======================================
// I2C + LCD
// ======================================

void I2C_init(void);
void I2C_start(void);
void I2C_stop(void);
uint8_t I2C_write(uint8_t v);

void pcf8574_autodetect(void);
void PCF8574_write(uint8_t b);
void twi_lcd_cmd(const unsigned char x);
void twi_lcd_dwr(unsigned char x);
void twi_lcd_msg(const char *c);
void twi_lcd_clear(void);
void twi_lcd_4bit_send(unsigned char x);
void twi_lcd_init(void);
void lcd_twolines(const char *line1, const char *line2);

// ======================================
// UART
// ======================================

void uart_init(unsigned int ubrr);
void uart_send(char c);
void uart_print(const char *s);
void uart_print_hex(uint8_t val);
void serialWrite(const char *s);
char Chardos(void);

// ======================================
// ADC / PWM / Distancia
// ======================================

void Init_pwm(void);
void Init_adc(void);
uint16_t adc_read_AC0(void);
uint16_t adc_read_AC1(void);

void Init_timer1(void);

// ======================================
// DHT11
// ======================================

bool dht11_read2(uint8_t *t, uint8_t *h);

// ======================================
// Utilidades conversión
// ======================================

char Number_to_ascii(uint16_t val);
char* Add_to_string(char *out, uint16_t val);
bool ascii_to_u16_switch(const char *s, uint16_t *out);

#endif
