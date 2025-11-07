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
// FUNCIONES Y PINES LED 
// ======================================

#define LED_G PORTC0
#define LED_R PORTC1

void INIT_LED(){
	DDRC |= (1 << LED_G) | (1 << LED_R);
	PORTC |= (1 << LED_G);
	PORTC &= ~(1 << LED_R);
}


// Control LCD I2C (PCF8574)
#define LCD_EN          0x04   // Enable
#define LCD_RW          0x02   // Read/Write
#define LCD_RS          0x01   // Register Select
#define LCD_BACKLIGHT   0x08   // Retroiluminación
static uint8_t PCF_ADDR = 0x27;

// ======================================
// CONFIGURACIÓN UART
// ======================================
#define BAUD           9600UL
#define UBRR_VALUE     ((F_CPU/16/BAUD) - 1)
#define TX_BUFFER_SIZE 128
#define RX_BUFFER_SIZE 128

volatile char    serialBuffer[TX_BUFFER_SIZE];
volatile uint8_t serialReadPos  = 0;
volatile uint8_t serialWritePos = 0;

volatile char    rxBuffer[RX_BUFFER_SIZE];
volatile uint8_t rxReadPos  = 0;
volatile uint8_t rxWritePos = 0;

// ======================================
// PROTOTIPOS
// ======================================
void uart_init(unsigned int ubrr);
void uart_send(char c);
void uart_print(const char *s);
void uart_print_hex(uint8_t val);
char Chardos(void);
void serialWrite(const char *s);

void spi_init(void);
uint8_t spi_transfer(uint8_t data);
void SS_HIGH(void);
void SS_LOW(void);

// ======================================
// INTERRUPCIONES UART
// ======================================

// RX Complete
ISR(USART_RX_vect) {
	rxBuffer[rxWritePos] = UDR0;
	rxWritePos++;
	if (rxWritePos >= RX_BUFFER_SIZE)
	rxWritePos = 0;
}


// UDRE (Data Register Empty)
ISR(USART_UDRE_vect) {
	if (serialReadPos != serialWritePos) {
		UDR0 = serialBuffer[serialReadPos];
		serialReadPos = (serialReadPos + 1) % TX_BUFFER_SIZE;
		} else {
		UCSR0B &= ~(1 << UDRIE0); // Nada más que enviar
	}
}

// ======================================
// FUNCIONES UART
// ======================================

static inline uint8_t nibble_to_bus(uint8_t nibble) { return (nibble << 4); } // nibble en P4..P7



void uart_init(unsigned int ubrr) {
	UBRR0H = (char)(ubrr >> 8);
	UBRR0L = (char)ubrr;
	UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8 bits
}

void uart_send(char c) {
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = c;
}

void uart_print(const char *s) {
	while (*s) uart_send(*s++);
}

void uart_print_hex(uint8_t val) {
	char buf[6];
	sprintf(buf, "0x%02X ", val);
	uart_print(buf);
}

char Chardos(void) {
	char ret = '\0';
	if (rxReadPos != rxWritePos) {
		ret = rxBuffer[rxReadPos];
		rxReadPos++;
		if (rxReadPos >= RX_BUFFER_SIZE)
		rxReadPos = 0;
	}
	return ret;
}



// ======================================
// FUNCIONES SPI
// ======================================

#define CS   PB2
#define MOSI PB3
#define MISO PB4
#define SCK  PB5

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

static inline void I2C_start(void) {
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
}
static inline void I2C_stop(void) {
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
}
static inline uint8_t I2C_write(uint8_t v) {
	TWDR = v;
	TWCR = (1<<TWINT)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
	uint8_t s = TWSR & 0xF8;
	// 0x18: SLA+W ACK, 0x28: DATA ACK  (lo suficiente para escritura)
	return (s == 0x18 || s == 0x28);
}


static uint8_t pcf8574_autodetect(void) {
	for (uint8_t a=0x20; a<=0x27; a++) {
		I2C_start();
		uint8_t ok = I2C_write((a<<1) | 0); // write
		I2C_stop();
		if (ok) return a;
	}
	for (uint8_t a=0x38; a<=0x3F; a++) {
		I2C_start();
		uint8_t ok = I2C_write((a<<1) | 0);
		I2C_stop();
		if (ok) return a;
	}
	return 0; // no encontrado
}
static inline void pcf8574_write(uint8_t b) {
	I2C_start();
	I2C_write((PCF_ADDR<<1) | 0);
	I2C_write(b);
	I2C_stop();
}

static inline void lcd_strobe(uint8_t data) {
	// Pulso en EN con BL siempre activo
	pcf8574_write(data | LCD_EN | LCD_BACKLIGHT);
	_delay_us(1);
	pcf8574_write((data & ~LCD_EN) | LCD_BACKLIGHT);
	_delay_us(50); // tiempo m?nimo entre nibbles
}
static inline void lcd_write4(uint8_t nibble, uint8_t rs) {
	uint8_t bus = nibble_to_bus(nibble) | (rs ? LCD_RS : 0);
	// RW=0 (escritura)
	bus &= ~LCD_RW;
	pcf8574_write(bus | LCD_BACKLIGHT);
	lcd_strobe(bus);
}
static inline void lcd_send(uint8_t value, uint8_t rs) {
	lcd_write4(value >> 4, rs);
	lcd_write4(value & 0x0F, rs);
}
static inline void lcd_cmd(uint8_t c)    { lcd_send(c, 0); }
static inline void lcd_data(uint8_t d)   { lcd_send(d, 1); }

static inline void lcd_clear(void) {
	lcd_cmd(0x01);           // clear
	_delay_ms(2);            // >1.5ms
}
static inline void lcd_set_cursor(uint8_t col, uint8_t row) {
	static const uint8_t offs[] = {0x00, 0x40, 0x14, 0x54}; // 16x2 / 20x4
	lcd_cmd(0x80 | (offs[row] + col));
}
static void lcd_print(const char *s) {
	while (*s) lcd_data((uint8_t)*s++);
}

void lcd_init(void) {
	_delay_ms(50);                   // power-up
	// Autodetecta direcci?n (opcional pero ?til)
	uint8_t found = pcf8574_autodetect();
	if (found) PCF_ADDR = found;
	
	// Secuencia de 4 bits ?oficial?
	lcd_write4(0x03, 0); _delay_ms(5);
	lcd_write4(0x03, 0); _delay_us(150);
	lcd_write4(0x03, 0); _delay_us(150);
	lcd_write4(0x02, 0); _delay_us(150);     // 4-bit

	lcd_cmd(0x28);                            // 4-bit, 2 l?neas, 5x8
	lcd_cmd(0x0C);                            // display ON, cursor OFF, blink OFF
	lcd_cmd(0x06);                            // entry mode: inc, no shift
	lcd_clear();
}
void lcd_msg2(const char* l1, const char* l2){
	lcd_clear();
	lcd_set_cursor(0,0);
	lcd_print(l1);
	lcd_set_cursor(0,1);
	lcd_print(l2);
}

void I2C_init(void) {
	TWSR = 0x00;            // Prescaler = 1
	TWBR = ((F_CPU / 100000UL) - 16) / 2;  // 100 kHz
	TWCR = (1<<TWEN);       // Habilitar TWI
}




// ======================================
// PROGRAMA PRINCIPAL
// ======================================





int main(void) {
	
	INIT_LED();
	uart_init(UBRR_VALUE);
	
	
	sei(); // Habilitar interrupciones globales
	
	
	spi_init();
	I2C_init();
	lcd_init();


	lcd_msg2("Bienvenido!!", "Sistema listo");
	
	uart_print("Opciones disponibles:\r\n");
	uart_print("1) Tomar contraseña del SPI\r\n");
	uart_print("2) Borrar contraseña actual\r\n");
	uart_print("3) Desbloquear sistema\r\n");
	
	
	uart_print("> ");
	
	uint8_t Contrasena_Guardar[5];
	uint8_t Contrasena_comparar[5];
	
	
	char c = '\0';
	while (1) {
		c = Chardos(); // Lectura no bloqueante UART

		if (c != '\0') {
			_delay_ms(1);
			if (c == '1') {
				uart_print("Leyendo SPI...\r\n");
				lcd_msg2("Leyendo SPI", "");

				SS_LOW();
				
				for (uint8_t i=0; i<5; i++){
					Contrasena_Guardar[i] = spi_transfer(0xFF);
				}
				SS_HIGH();
				lcd_msg2("Contraseña", "recibida");
				uart_print("Contraseña recibida: ");
				for (uint8_t i=0; i<5; i++){
					uart_print_hex(Contrasena_Guardar[i]);
				}
				uart_print("\r\n");
			}
		
		else if(c == '2'){
			lcd_msg2("Borrando", "Contraseña");
				
			uart_print("Borrando Contraseña....\r\n");
			for (uint8_t i=0; i<5; i++){
				Contrasena_Guardar[i] = 0x00;
			}
			uart_print("Borrada correctamente\r\n");
		}
		else if(c == '3'){
			lcd_msg2("Inserte", "Contraseña");
			
			uart_print("Inserte Contraseña:\r\n");
			SS_LOW();
			
			for (uint8_t i=0; i<5; i++){
				Contrasena_comparar[i] = spi_transfer(0xFF);
			}
			SS_HIGH();
			
			if (memcmp(Contrasena_Guardar, Contrasena_comparar, 5) == 0) {
				lcd_msg2("Contraseña ", "Correcta");
			
				uart_print("Sistema desbloqueado Correctamente\r\n");
				PORTC |= (1 << LED_G);
				PORTC &= ~(1 << LED_R);
				} else {
				uart_print("Contraseña incorrecta\r\n");
				lcd_msg2("Contraseña ", "incorrecta");
				
				PORTC |= (1 << LED_R);
				PORTC &= ~(1 << LED_G);
			}
		}
		}
	}
}
