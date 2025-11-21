#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define LED_PIN PORTD6
#define LED_DDR DDRD
#define NUM_LEDS 256

uint8_t leds[NUM_LEDS * 3];  // Datos GRB para cada LED


uint16_t rainbow_pos = 0;


uint8_t wheel_color(uint8_t pos, uint8_t color) {
	if (pos < 85)
	return (pos * 3 * (color == 0)) + ((255 - pos*3) * (color == 2));
	else if (pos < 170)
	return ((pos-85) * 3 * (color == 1)) + ((255 - (pos-85)*3) * (color == 0));
	else
	return ((pos-170) * 3 * (color == 2)) + ((255 - (pos-170)*3) * (color == 1));
}


// =========================================================
// UART SECTION
// =========================================================
void UART_init(uint16_t baud)
{
	uint16_t ubrr = (F_CPU / (16UL * baud)) - 1;
	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)(ubrr);

	UCSR0B = (1 << RXEN0) | (1 << TXEN0);     // Enable RX & TX
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);   // 8 data bits, 1 stop
}

void uart_tx(char c)
{
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = c;
}

void uart_print(const char *s)
{
	while (*s) uart_tx(*s++);
}

// ------------------------------------------------------------------
// Prototipos
// ------------------------------------------------------------------
uint8_t map_to_0_15(int16_t v);

void ws2812_init(void);
void ws2812_send_pixel(uint8_t r, uint8_t g, uint8_t b);
void ws2812_show(void);
void ws2812_fill(uint8_t r, uint8_t g, uint8_t b, uint16_t n);
void ws2812_set_pixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void ws2812_show_all(void);
void ws2812_clear(void);

void send_bit(uint8_t bitVal);
void send_byte(uint8_t byte);
void turn_led(uint8_t led_x, uint8_t led_y);

uint16_t serpentine_index(uint8_t x, uint8_t y);

uint8_t rand8(void);	


// =========================
// WS2812 (use your existing functions)
// =========================
// serpentine_index()
// ws2812_set_pixel()
// ws2812_show_all()
// ws2812_clear()
// etc.



// =========================================================
// I2C / TWI SECTION
// =========================================================

#define MPU6050_ADDR 0x68

void I2C_init(void)
{
	TWBR = 72;   // 100kHz
	TWSR = 0;
}

uint8_t I2C_start(uint8_t address)
{
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));

	uint8_t status = TWSR & 0xF8;
	if (status != TW_START && status != TW_REP_START) return 0;

	TWDR = address;
	TWCR = (1<<TWINT)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));

	status = TWSR & 0xF8;
	return (status == TW_MT_SLA_ACK || status == TW_MR_SLA_ACK);
}

void I2C_stop(void)
{
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
}

uint8_t I2C_write(uint8_t data)
{
	TWDR = data;
	TWCR = (1<<TWINT)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));

	return (TWSR & 0xF8) == TW_MT_DATA_ACK;
}

uint8_t I2C_readACK(void)
{
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
	while(!(TWCR & (1<<TWINT)));
	return TWDR;
}

uint8_t I2C_readNACK(void)
{
	TWCR = (1<<TWINT)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
	return TWDR;
}

// =========================================================
// MPU6050 SECTION
// =========================================================

void MPU6050_write(uint8_t reg, uint8_t data)
{
	I2C_start(MPU6050_ADDR << 1);
	I2C_write(reg);
	I2C_write(data);
	I2C_stop();
}

void MPU6050_init(void)
{
	_delay_ms(100);
	MPU6050_write(0x6B, 0x00); // Wake up
	MPU6050_write(0x1B, 0x00); // Gyro ±250
	MPU6050_write(0x1C, 0x00); // Acc ±2g
}

void MPU6050_read_raw(int16_t *ax, int16_t *ay, int16_t *az,
int16_t *gx, int16_t *gy, int16_t *gz)
{
	I2C_start(MPU6050_ADDR << 1);
	I2C_write(0x3B); // Start at ACCEL_XOUT_H

	I2C_start((MPU6050_ADDR << 1) | 1); // Restart in read mode

	*ax = (I2C_readACK() << 8) | I2C_readACK();
	*ay = (I2C_readACK() << 8) | I2C_readACK();
	*az = (I2C_readACK() << 8) | I2C_readACK();

	*gx = (I2C_readACK() << 8) | I2C_readACK();
	*gy = (I2C_readACK() << 8) | I2C_readACK();
	*gz = (I2C_readACK() << 8) | I2C_readNACK();  // <----- FIXED HERE

	I2C_stop();
}

// =========================================================
// MAIN PROGRAM
// =========================================================

int main(void)
{
	ws2812_init();
	I2C_init();
	MPU6050_init();

	ws2812_clear();
	ws2812_show_all();

	int16_t ax, ay, az, gx, gy, gz;

	uint16_t rainbow_pos = 0;

	while (1)
	{
		// IMU read
		MPU6050_read_raw(&ax, &ay, &az, &gx, &gy, &gz);

		// Map IMU tilt ? matrix coordinates
		uint8_t x = map_to_0_15(az);
		uint8_t y = map_to_0_15(16-ay);

		if (x > 15) x = 15;
		if (y > 15) y = 15;

		// Compute the rainbow color
		uint8_t r = wheel_color(rainbow_pos, 0);
		uint8_t g = wheel_color(rainbow_pos, 1);
		uint8_t b = wheel_color(rainbow_pos, 2);

		rainbow_pos++;
		if (rainbow_pos >= 255) rainbow_pos = 0;

		// Draw cursor LED
		ws2812_clear();
		uint16_t idx = serpentine_index(x, y);
		ws2812_set_pixel(idx, r, g, b);

		ws2812_show_all();

	}
}




uint8_t rand8(void) {
	static uint16_t seed = 0xACE1;  // any nonzero seed
	seed = (seed >> 1) ^ (-(seed & 1u) & 0xB400u); // 16-bit LFSR
	return seed & 0xFF;
}



void ws2812_init(void) {
	LED_DDR |= (1 << LED_PIN); // Configura pin de salida
}

void send_bit(uint8_t bitVal) {
	if (bitVal) {
		PORTD |= (1 << LED_PIN);
		asm volatile (
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t");
		PORTD &= ~(1 << LED_PIN);
		asm volatile ("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
		} else {
		PORTD |= (1 << LED_PIN);
		asm volatile ("nop\n\t""nop\n\t""nop\n\t");
		PORTD &= ~(1 << LED_PIN);
		asm volatile (
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
	}
}


void send_byte(uint8_t byte) {
	cli();
	for (uint8_t i = 0; i < 8; i++) {
		send_bit(byte & 0x80);
		byte <<= 1;
	}
	sei();
}

void ws2812_send_pixel(uint8_t r, uint8_t g, uint8_t b) {
	send_byte(g);
	send_byte(r);
	send_byte(b);
}

void ws2812_show(void) {
	_delay_us(30);  // tiempo de reset (>50us)
}

void ws2812_fill(uint8_t r, uint8_t g, uint8_t b, uint16_t n) {
	cli();
	for (uint16_t i = 0; i < n; i++) {
		ws2812_send_pixel(r, g, b);
	}
	sei();
	ws2812_show();
}

void ws2812_set_pixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
	if (index >= NUM_LEDS) return;
	leds[index * 3 + 0] = g;
	leds[index * 3 + 1] = r;
	leds[index * 3 + 2] = b;
}

void ws2812_show_all(void) {
	cli();
	for (uint16_t i = 0; i < NUM_LEDS; i++) {
		ws2812_send_pixel(leds[i * 3 + 1], leds[i * 3 + 0], leds[i * 3 + 2]);
	}
	sei();
	ws2812_show();
}

void ws2812_clear(void) {
	for (uint16_t i = 0; i < NUM_LEDS * 3; i++)
	leds[i] = 0;
}

// Encender LED en coordenadas (x, y)
void turn_led(uint8_t led_x, uint8_t led_y) {
	uint8_t index = led_y * 8 + led_x;
	ws2812_clear();
	ws2812_set_pixel(index, 0, 0, 0); // rojo
	ws2812_show_all();
}



uint16_t serpentine_index(uint8_t x, uint8_t y) {
	if (y % 2 == 0) {
		return y * 16 + x;         // fila normal
		} else {
		return y * 16 + (15 - x);  // fila invertida
	}
}


uint8_t map_to_0_15(int16_t v)
{
	int32_t x = (int32_t)v;

	// Clamp
	if (x < -10000) x = -10000;
	if (x > 10000)  x = 10000;

	x += 10000;            // now 0 ? 20000
	return x / 1333;       // now 0 ? 15
}