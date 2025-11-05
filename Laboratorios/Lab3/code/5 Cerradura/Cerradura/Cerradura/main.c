#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/twi.h>


// --------------------------------------
// Definiciones
// --------------------------------------

// Configuración general del sistema
#define BAUD           9600UL                     // Velocidad UART
#define UBRR_VALUE     ((F_CPU/16/BAUD) - 1)      // Valor de UBRR según BAUD
#define TX_BUFFER_SIZE 128                        // Tamaño del buffer TX
#define RX_BUFFER_SIZE 128                        // Tamaño del buffer RX
#define precarger      10000                      // Valor de precarga (genérico)
#define UID_LEN        5                          // Longitud del UID RFID

// Macros generales
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))   // Set bit
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))  // Clear bit
#define UART_TIMEOUT_MS 500                          // Tiempo máximo UART en ms

// Pines SPI y control del RC522
#define SS_LOW()   (PORTB &= ~(1<<PB2))  // Activa SS (Slave Select)
#define SS_HIGH()  (PORTB |=  (1<<PB2))  // Desactiva SS
#define RST_PIN    PB1                   // Pin de reset del RC522

// Registros del RC522
#define CommandReg      0x01
#define CommIEnReg      0x02
#define CommIrqReg      0x04
#define DivIrqReg       0x05
#define ErrorReg        0x06
#define FIFODataReg     0x09
#define FIFOLevelReg    0x0A
#define ControlReg      0x0C
#define BitFramingReg   0x0D
#define ModeReg         0x11
#define TxModeReg       0x12
#define RxModeReg       0x13
#define TxControlReg    0x14
#define TxASKReg        0x15
#define RFCfgReg        0x26
#define TModeReg        0x2A
#define TPrescalerReg   0x2B
#define TReloadRegH     0x2C
#define TReloadRegL     0x2D
#define VersionReg      0x37

// Comandos RFID
#define PCD_IDLE        0x00   // Inactivo
#define PCD_TRANSCEIVE  0x0C   // Transmitir y recibir datos
#define PICC_REQIDL     0x26   // Solicitud de tarjeta en reposo
#define PICC_ANTICOLL   0x93   // Anticolisión (lectura de UID)

// Control LCD I2C (PCF8574)
#define LCD_EN          0x04   // Enable
#define LCD_RW          0x02   // Read/Write
#define LCD_RS          0x01   // Register Select
#define LCD_BACKLIGHT   0x08   // Retroiluminación


// --------------------------------------
// Variables
// --------------------------------------


// UID almacenado en EEPROM
uint8_t EEMEM ee_uid[5];                 // UID guardado permanentemente en EEPROM

// Buffers de comunicación UART
volatile char    serialBuffer[TX_BUFFER_SIZE];  // Buffer de transmisión UART
volatile uint8_t serialReadPos  = 0;            // Posición de lectura en buffer TX
volatile uint8_t serialWritePos = 0;            // Posición de escritura en buffer TX

volatile char    rxBuffer[RX_BUFFER_SIZE];      // Buffer de recepción UART
volatile uint8_t rxReadPos  = 0;                // Posición de lectura en buffer RX
volatile uint8_t rxWritePos = 0;                // Posición de escritura en buffer RX

// Variables generales del sistema
uint8_t uid[16];             // UID leído desde la tarjeta RFID
uint8_t estado = 0;          // Estado general del sistema
static uint8_t PCF_ADDR = 0x27;  // Dirección I2C del expansor PCF8574 (por defecto)
char c = '\0';               // Variable temporal para lectura UART
volatile uint8_t C = 0;      // Variable auxiliar, 0..3 según selección (opcional)

// Declaración externa del UID almacenado en EEPROM
extern uint8_t EEMEM ee_uid[UID_LEN];



// --------------------------------------
// Prototipos
// --------------------------------------



// Funciones generales
void Sonar_Buzzer(void);
void EEPROM_write(uint16_t address, uint8_t data);
uint8_t EEPROM_read(uint16_t address);

// Manejo de UID (RFID)
static inline uint8_t uid_es_vacio(const uint8_t u[UID_LEN]);
static inline void uid_imprimir(const uint8_t u[UID_LEN]);
static inline uint8_t uid_iguales(const uint8_t *a, const uint8_t *b);
void guardar_uid(const uint8_t u[UID_LEN]);
void leer_uid(uint8_t u[UID_LEN]);

// Control de estado e indicadores
void init_leds(void);
void Cambiar_estado(uint8_t nuevo_estado);

// Comunicación SPI
void spi_init(void);
uint8_t spi_transfer(uint8_t data);

// Comunicación UART
void uart_init(unsigned int ubrr);
void uart_send(char c);
void uart_print(const char *s);
void uart_print_hex(uint8_t val);
void serialWrite(const char *s);
char Chardos(void);

// Control RC522 (RFID)
void mfrc522_resetPinInit(void);
void mfrc522_write(uint8_t reg, uint8_t value);
uint8_t mfrc522_read(uint8_t reg);
void mfrc522_setBitMask(uint8_t reg, uint8_t mask);
void mfrc522_clearBitMask(uint8_t reg, uint8_t mask);
void mfrc522_printRegister(const char* name, uint8_t reg);
void mfrc522_reset(void);
void mfrc522_init(void);
void mfrc522_standard(uint8_t *card_uid);

// Comunicación I2C
static inline void I2C_start(void);
static inline void I2C_stop(void);
static inline uint8_t I2C_write(uint8_t v);

// Control del LCD (I2C)
static inline uint8_t nibble_to_bus(uint8_t nibble);
static uint8_t pcf8574_autodetect(void);
static inline void pcf8574_write(uint8_t b);
static inline void lcd_strobe(uint8_t data);
static inline void lcd_write4(uint8_t nibble, uint8_t rs);
static inline void lcd_send(uint8_t value, uint8_t rs);
static inline void lcd_cmd(uint8_t c);
static inline void lcd_data(uint8_t d);
static inline void lcd_clear(void);
static inline void lcd_set_cursor(uint8_t col, uint8_t row);
static void lcd_print(const char *s);
void lcd_init(void);
void lcd_msg2(const char* l1, const char* l2);
static inline char poll_switch_portc(void);


// ---------------------------------------
// ISRs
// ---------------------------------------

ISR(PCINT1_vect) {
	if  ((1<<PORTC0) == 0 ){        // PC0 presionado
		c = '1';
		} else if ((1<<PORTC1) == 0 ) { // PC1 presionado
		c = '2';
		} else if ((1<<PORTC2) == 0 ) { // PC2 presionado
		c = '3';
		} else {
		c = 0;                          // ninguno
	}
}

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
		UCSR0B &= ~(1 << UDRIE0);  // nada m�s que enviar
	}
}


void I2C_init(void) {
	TWSR = 0x00;            // Prescaler = 1
	TWBR = ((F_CPU / 100000UL) - 16) / 2;  // 100 kHz
	TWCR = (1<<TWEN);       // Habilitar TWI
}


// --------------------------------------
// Programa principal
// --------------------------------------


int main(void)
{
	uart_init(UBRR_VALUE);


	// RX por interrupci�n
	UCSR0B |= (1<<RXCIE0);
    I2C_init();
	
	spi_init();
	mfrc522_resetPinInit();
	mfrc522_init();
	init_leds();
	lcd_init();


	lcd_msg2("Bienvenido!!", "RFID listo");
	
	uart_print("\r\n1) Leer y comparar tarjeta\r\n2) Registrar nueva tarjeta\r\n3) Borrar tarjeta\r\n");
	
	while(1)
	{
		uart_print("> ");
	
		// esperar un car�cter del buffer RX
		c = '\0';
		while (1) {
			char u = Chardos();            // UART no bloqueante
			if (u != '\0') { c = u; break; }

			char s = poll_switch_portc();  // switch en PC0..PC2
			if (s != '\0') {
				c = s;
				// espera a que suelten para no repetir disparos
				while ( !(PINC & (1<<PORTC0)) || !(PINC & (1<<PORTC1)) || !(PINC & (1<<PORTC2)) ) {
					_delay_ms(1);
				}
				break;
			}
			_delay_ms(1); // respiro
		}
	bool ok1 = false;
	if (c == '1') {
	lcd_msg2("Acerque su", "tarjeta");
	
		while(!ok1){
			memset(uid, 0, sizeof(uid));
			mfrc522_standard(uid);
			if (uid_es_vacio(uid)){
				uart_print("Acerce la tarjeta.\r\n");
				Cambiar_estado(0);
				} else {
				uint8_t guardado[UID_LEN];
				leer_uid(guardado);
				
				if (uid_iguales(uid, guardado)){
					lcd_msg2("Tarjeta aprobada", "");
					uart_print("Acceso: UID coincide. UID = ");
					uid_imprimir(uid);
					uart_print("\r\n");
					Cambiar_estado(1);
					ok1 = true;
					
					} else {
					lcd_msg2("Tarjeta Denegada", "");			
					uart_print("UID distinto / no registrado. Leido = ");
					uid_imprimir(uid);
					uart_print("\r\n");
					Sonar_Buzzer();

					
					ok1 = true;
					c = '\0';
					break;
				}
			}
		}
		}
	else if (c == '2') {
		lcd_msg2("Acerque su", "tarjeta");
	
		uart_print("Acerque la tarjeta a registrar...\r\n");

		// Esperar hasta detectar una tarjeta v�lida
		uint16_t timeout = 3000/50; // ~3 s a pasos de 50 ms
		uint8_t ok = 0;
		while (timeout--){
			memset(uid, 0, sizeof(uid));
			mfrc522_standard(uid);
			if (!uid_es_vacio(uid)){ ok = 1; break; }
			_delay_ms(50);
		}
		if (!ok){
			uart_print("No se detecto tarjeta.\r\n");
			lcd_msg2("No se detecto", "tarjeta");
		
			Cambiar_estado(0);
			} else {
			lcd_msg2("Se registro su", "tarjeta");
			guardar_uid(uid);
			uart_print("UID registrado: ");
			uid_imprimir(uid);
			uart_print("\r\n");
			Cambiar_estado(1);
		}

	}
	else if (c == '3')
	{
		uart_print("se borro exitosamente");
		lcd_msg2("Se borro", "exitosamente");
		guardar_uid(0);
	}
	}
}


// --------------------------------------
// Funciones
// --------------------------------------



void Sonar_Buzzer(){
	DDRC |= (1<<PORTC3);  // PC3 salida


	for (uint32_t i = 0; i < 1000; i++){
		PORTC |=  (1<<PORTC3);   // ON
		_delay_us(250);
		PORTC &= ~(1<<PORTC3);   // OFF
		_delay_us(250);
	}
}
void EEPROM_write(uint16_t address, uint8_t data) {
	// Esperar a que termine una escritura previa
	while (EECR & (1 << EEPE));

	// Cargar direcci�n y dato
	EEAR = address;
	EEDR = data;

	// Secuencia de escritura: EEMPE y dentro de 4 ciclos EEPE
	uint8_t s = SREG;  // guardar estado de interrupciones
	cli();
	EECR |= (1 << EEMPE);   // Habilitar la escritura en EEPROM
	EECR |= (1 << EEPE);    // Iniciar la operaci�n de escritura
	SREG = s;               // restaurar interrupciones
}
uint8_t EEPROM_read(uint16_t address) {
	// Asegurar que no haya escritura en progreso
	while (EECR & (1 << EEPE));

	// Cargar direcci�n y disparar lectura
	EEAR = address;
	EECR |= (1 << EERE);    // Leer el dato

	return EEDR;
}
static inline uint8_t uid_es_vacio(const uint8_t u[UID_LEN]){
	for (uint8_t i=0;i<UID_LEN;i++) if (u[i] != 0) return 0;
	return 1;
}
static inline void uid_imprimir(const uint8_t u[UID_LEN]){
	for (uint8_t i=0;i<UID_LEN;i++){ uart_print_hex(u[i]); uart_print(" "); }
}
static inline uint8_t uid_iguales(const uint8_t *a, const uint8_t *b){
	return memcmp(a, b, UID_LEN) == 0;
}
void guardar_uid(const uint8_t u[UID_LEN]){
	for (uint8_t i=0;i<UID_LEN;i++){
		EEPROM_write((uint16_t)&ee_uid[i], u[i]);
	}
}
void leer_uid(uint8_t u[UID_LEN]){
	for (uint8_t i=0;i<UID_LEN;i++){
		u[i] = EEPROM_read((uint16_t)&ee_uid[i]);
	}
}
static inline uint8_t nibble_to_bus(uint8_t nibble) { return (nibble << 4); } // nibble en P4..P7

void init_leds(){
	DDRD |= (1<<DDD6) | (1<<DDD7);                 // PD6 y PD7 salidas
	PORTD &= ~((1<<PORTD6) | (1<<PORTD7));         // ambos en 0
	// Estado inicial: NO tarjeta -> PD6 = 1, PD7 = 0
	PORTD |= (1<<PORTD6);
}
void Cambiar_estado(uint8_t nuevo_estado){
	if (estado == nuevo_estado) return;            // nada que hacer si no cambi�
	estado = nuevo_estado;

	if (estado){                                    // HAY tarjeta
		PORTD |=  (1<<PORTD7);
		PORTD &= ~(1<<PORTD6);
		}else{                                          // NO hay tarjeta
		PORTD |=  (1<<PORTD6);
		PORTD &= ~(1<<PORTD7);
	}
}

void mfrc522_resetPinInit() {
	DDRB |= (1<<RST_PIN);
	// Realizar un reset hardware
	PORTB &= ~(1<<RST_PIN);
	_delay_ms(10);
	PORTB |= (1<<RST_PIN);
	_delay_ms(50);
}
void mfrc522_write(uint8_t reg, uint8_t value) {
	SS_LOW();
	spi_transfer((reg<<1) & 0x7E);
	spi_transfer(value);
	SS_HIGH();
}
uint8_t mfrc522_read(uint8_t reg) {
	uint8_t val;
	SS_LOW();
	spi_transfer(((reg<<1)&0x7E) | 0x80);
	val = spi_transfer(0x00);
	SS_HIGH();
	return val;
}
void mfrc522_setBitMask(uint8_t reg, uint8_t mask) {
	uint8_t tmp = mfrc522_read(reg);
	mfrc522_write(reg, tmp | mask);
}
void mfrc522_clearBitMask(uint8_t reg, uint8_t mask) {
	uint8_t tmp = mfrc522_read(reg);
	mfrc522_write(reg, tmp & (~mask));
}
void mfrc522_printRegister(const char* name, uint8_t reg) {
	uart_print(name);
	uart_print(": ");
	uart_print_hex(mfrc522_read(reg));
	uart_print("\r\n");
}

void mfrc522_reset() {
	uart_print("Soft Reset...\r\n");
	mfrc522_write(CommandReg, (1<<4));
	_delay_ms(50);
}
void mfrc522_init() {
	mfrc522_reset();

	uart_print("Configurando temporizadores y modulacion...\r\n");
	// Configuracion de temporizadores para 106 kbps
	mfrc522_write(TModeReg, 0x8D);     // 1000 1101 - Auto restart, timer starts
	mfrc522_write(TPrescalerReg, 0x3E); // 0011 1110 - Prescaler
	mfrc522_write(TReloadRegL, 30);     // Timer reload value
	mfrc522_write(TReloadRegH, 0);

	// Configuracion de la modulacion
	mfrc522_write(TxASKReg, 0x40);     // 100% ASK modulation
	mfrc522_write(ModeReg, 0x3D);      // CRC enabled, MSB first
	
	// Configurar ganancia del receptor
	mfrc522_write(RFCfgReg, 0x7F);     // Ganancia maxima (48dB)
	
	// Activar la antena
	mfrc522_write(TxControlReg, 0x83); // Antena ON, con control de ganancia
	_delay_ms(5); // Espera a que la antena estabilice
}

void mfrc522_standard(uint8_t *card_uid) {
	uint8_t req[1] = {PICC_REQIDL};
	uint8_t buffer[16];
	uint8_t bufferLength = sizeof(buffer);
	
	// Preparar registro de bits y FIFO
	mfrc522_write(BitFramingReg, 0x07); // 7 bits para REQA
	mfrc522_write(CommIrqReg, 0x7F);    // Limpiar IRQ
	mfrc522_write(FIFOLevelReg, 0x80);  // Limpiar FIFO

	uart_print("\r\n=== Enviando REQA ===\r\n");
	// Escribir FIFO
	for (uint8_t i=0; i<1; i++) {
		mfrc522_write(FIFODataReg, req[i]);
	}

	// Iniciar transaccion
	mfrc522_write(CommandReg, PCD_TRANSCEIVE);
	mfrc522_setBitMask(BitFramingReg, 0x80); // StartSend

	// Esperar IRQ (RxIRq o Timeout)
	uint16_t count = 1000; // Reducir el tiempo de espera
	uint8_t irq;
	do {
		irq = mfrc522_read(CommIrqReg);
		count--;
	} while (!(irq & 0x30) && count); // RxIRq o IdleIRq

	mfrc522_clearBitMask(BitFramingReg, 0x80);

	if (count == 0) {
		uart_print("Timeout REQA, tarjeta no detectada\r\n");
		memset(card_uid, 0, 16);
		} else {
		uint8_t fifoLevel = mfrc522_read(FIFOLevelReg);
		for (uint8_t i=0; i<fifoLevel; i++) {
			uint8_t val = mfrc522_read(FIFODataReg);
			if (i < bufferLength) {
				buffer[i] = val;
			}
		}
		
		// Si hay datos, intentar leer el UID
		if (fifoLevel > 0) {
			uart_print("Tarjeta detectada! Intentando leer UID...\r\n");
			
			// Anticollision
			mfrc522_write(BitFramingReg, 0x00);
			mfrc522_write(CommIrqReg, 0x7F);
			mfrc522_write(FIFOLevelReg, 0x80);
			
			// Escribir comando Anticollision en FIFO
			mfrc522_write(FIFODataReg, PICC_ANTICOLL);
			mfrc522_write(FIFODataReg, 0x20); // NVB - Number of Valid Bits
			
			// Iniciar transaccion
			mfrc522_write(CommandReg, PCD_TRANSCEIVE);
			mfrc522_setBitMask(BitFramingReg, 0x80); // StartSend
			
			// Esperar IRQ
			count = 1000;
			do {
				irq = mfrc522_read(CommIrqReg);
				count--;
			} while (!(irq & 0x30) && count);
			
			mfrc522_clearBitMask(BitFramingReg, 0x80);
			
			if (count == 0) {
				uart_print("Timeout Anticollision\r\n");
				} else {
				fifoLevel = mfrc522_read(FIFOLevelReg);
				uart_print("UID leido!");
				for (uint8_t i=0; i<fifoLevel; i++) {
					uint8_t val = mfrc522_read(FIFODataReg);

					card_uid[i] = val;
				}
				uart_print("\r\n");
			}
		}
	}
}
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
void spi_init(void) {
	DDRB |= (1<<PORTB2)|(1<<PORTB3)|(1<<PORTB5); // SS, MOSI, SCK salidas
	DDRB &= ~(1<<PORTB4); // MISO entrada
	SPCR = (1<<SPE)|(1<<MSTR);
	SPSR = (1<<SPI2X); // fosc/8
}
uint8_t spi_transfer(uint8_t data) {
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));
	return SPDR;
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
	_delay_us(50); // tiempo m�nimo entre nibbles
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
	// Autodetecta direcci�n (opcional pero �til)
// 	uint8_t found = pcf8574_autodetect();
// 	if (found) PCF_ADDR = found;
	PCF_ADDR = 0x27;
	// Secuencia de 4 bits �oficial�
	lcd_write4(0x03, 0); _delay_ms(5);
	lcd_write4(0x03, 0); _delay_us(150);
	lcd_write4(0x03, 0); _delay_us(150);
	lcd_write4(0x02, 0); _delay_us(150);     // 4-bit

	lcd_cmd(0x28);                            // 4-bit, 2 l�neas, 5x8
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
static inline char poll_switch_portc(void) {
	// Con pull-ups: presionado = 0 (activo en bajo)
	uint8_t p = PINC;
	if (!(p & (1<<PC0))) { _delay_ms(15); if (!(PINC & (1<<PC0))) { C = 1; return '1'; } }
	if (!(p & (1<<PC1))) { _delay_ms(15); if (!(PINC & (1<<PC1))) { C = 2; return '2'; } }
	if (!(p & (1<<PC2))) { _delay_ms(15); if (!(PINC & (1<<PC2))) { C = 3; return '3'; } }
	return '\0';
}

