#ifndef PROYECTO_H
#define PROYECTO_H

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <util/twi.h>

/* === Prototipos generales === */

char* Add_to_string(char *out, uint16_t val);
char Number_to_ascii(uint16_t val);

uint16_t adc_read_AC0(void);
uint16_t adc_read_AC1(void);
void Init_adc(void);

static bool dht11_read2(uint8_t *t, uint8_t *h);

void I2C_init(void);
void I2C_start(void);
void I2C_stop(void);
uint8_t I2C_write(uint8_t data);

/* LCD I2C */
extern uint8_t PCF_ADDR;
#define LCD_EN          0x04
#define LCD_RW          0x02
#define LCD_RS          0x01
#define LCD_BACKLIGHT   0x08
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

extern unsigned char lcd;

/* HC-SR04 */
#define PIN_T PORTD4
#define PIN_ECHO PORTB0

extern uint16_t Distancia_cm;

void Init_timer1(void);

extern uint16_t endd;
extern uint16_t start;
extern uint16_t width;

extern uint8_t count_t2;
extern uint8_t leer_dht;

void timer2_init(void);

#endif
