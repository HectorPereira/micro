#ifndef LIBRERIA_H_
#define LIBRERIA_H_

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdint.h>

#define CS   PB2
#define MOSI PB3
#define MISO PB4
#define SCK  PB5

#define LCD_EN 0x04
#define LCD_RW 0x02
#define LCD_RS 0x01
#define LCD_BACKLIGHT 0x08

#define BAUD 9600UL
#define UBRR_VALUE ((F_CPU/16/BAUD)-1)
#define TX_BUFFER_SIZE 128
#define RX_BUFFER_SIZE 128

#define DHT_PIN PD7
#define LIGHT_PIN PD2

#define PIN_T PD4
#define PIN_ECHO PB0

extern uint8_t Hum, Humdec, Temp, Tdec, Checksum;
extern uint8_t PCF_ADDR;
extern unsigned char lcd;
extern volatile char serialBuffer[TX_BUFFER_SIZE];
extern volatile uint8_t serialReadPos, serialWritePos;
extern volatile char rxBuffer[RX_BUFFER_SIZE];
extern volatile uint8_t rxReadPos, rxWritePos;
extern uint16_t Distancia_cm;
extern uint16_t count_t2;
extern uint8_t leer_dht;

void spi_init(void);
uint8_t spi_transfer(uint8_t data);
void SS_HIGH(void);
void SS_LOW(void);

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
void lcd_twolines(const char *c, const char *b);

void uart_init(unsigned int ubrr);
void uart_send(char c);
void uart_print(const char *s);
void uart_print_hex(uint8_t val);
void serialWrite(const char *s);
char Chardos(void);

void Init_pwm(void);
void Init_adc(void);
uint16_t adc_read_AC0(void);
uint16_t adc_read_AC1(void);
void Init_timer1(void);
void timer2_init(void);

char Number_to_ascii(uint16_t val);
char* Add_to_string(char *out, uint16_t val);
bool ascii_to_u16_switch(const char *s, uint16_t *out);

bool dht11_read2(uint8_t *t, uint8_t *h);

#endif
