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

uint8_t SPI_masterReceive();
// ---------------------------------------
// ISRs
// ---------------------------------------

// Interrupcion para detectar los botones

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

// Salta cada vez que UDR0 se llena.

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
		UCSR0B &= ~(1 << UDRIE0);  // nada mas que enviar
	}
}


// --------------------------------------
// Programa principal
// --------------------------------------


int main(void)
{
	sei();
	
	uart_init(UBRR_VALUE);
	// RX por interrupcion
	UCSR0B |= (1<<RXCIE0); // Se habilita antes porque el resto de funciones lo utilizan
	
	I2C_init();
	spi_init();
	init_leds();
	lcd_init();
	
	
	guardar_uid(0);

	uart_print("\r\n1) Leer y comparar tarjeta\r\n2) Registrar nueva tarjeta\r\n3) Borrar tarjeta\r\n");
	uart_print("> ");
	while(1)
	{
		
		
		// esperar un carcter del buffer RX
		c = '\0';
		c = Chardos();            // UART no bloqueante
		if (c != '\0') {
			_delay_ms(1); // respiro
			
			lcd_msg2("Acerque su", "tarjeta");
			uart_print("Acerce la tarjeta.\r\n");
			bool ok1 = false;
			if (c == '1') {
				lcd_msg2("Acerque su", "tarjeta");
				uart_print("Acerce la tarjeta.\r\n");
				SS_LOW();
				uint8_t datos[4];
				for(uint8_t i=0;i<4;i++){
					SPDR=0xFF;
					while(!(SPSR&(1<<SPIF)));
					datos[i]=SPDR; // Lectura de mas de un dato, si leo uno solo recibe basura
				}
				SS_HIGH();
				for(uint8_t i=0;i<4;i++){
					uart_print_hex(datos[i]);
				}
				
			}
			else if (c == '2') {
				lcd_msg2("Acerque su", "tarjeta");
				
				uart_print("Acerque la tarjeta a registrar...\r\n");
			}
			else if (c == '3')
			{
				uart_print("se borro exitosamente");
				lcd_msg2("Se borro", "exitosamente");
				guardar_uid(0);
			}
			uart_print("> ");
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

void init_leds(){
	DDRD |= (1<<DDD6) | (1<<DDD7);                 // PD6 y PD7 salidas
	PORTD &= ~((1<<PORTD6) | (1<<PORTD7));         // ambos en 0
	// Estado inicial: NO tarjeta -> PD6 = 1, PD7 = 0
	PORTD |= (1<<PORTD6);
}
void Cambiar_estado(uint8_t nuevo_estado){
	if (estado == nuevo_estado) return;            // nada que hacer si no cambi?
	estado = nuevo_estado;

	if (estado){                                    // HAY tarjeta
		PORTD |=  (1<<PORTD7);
		PORTD &= ~(1<<PORTD6);
		}else{                                          // NO hay tarjeta
		PORTD |=  (1<<PORTD6);
		PORTD &= ~(1<<PORTD7);
	}
}

void uart_init(unsigned int ubrr) {
	UBRR0H = (char)(ubrr>>8);
	UBRR0L = (char)ubrr;
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
	DDRB |= (1<<CS)|(1<<MOSI)|(1<<SCK); // SS, MOSI, SCK salidas
	DDRB &= ~(1<<MISO); // MISO entrada

	SPCR = (1<<SPE)|(1<<MSTR); // Configuracion de maestro para el Arduino
	SPSR = (1<<SPI2X); // fosc/8
}

uint8_t spi_transfer(uint8_t data) {
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));
	return SPDR;
}

void SPI_masterTransmitByte(uint8_t data)
{
	// load data into register
	SPDR = data;

	// Wait for transmission complete
	while(!(SPSR & (1 << SPIF)));
}

uint8_t SPI_masterReceive()
{
	// transmit dummy byte
	SPDR = 0xFF;

	// Wait for reception complete
	while(!(SPSR & (1 << SPIF)));

	// return Data Register
	return SPDR;
}

uint8_t SPI_masterTxRx(uint8_t data)
{
	// transmit data
	SPDR = data;

	// Wait for reception complete
	while(!(SPSR & (1 << SPIF)));

	// return Data Register
	return SPDR;
}

void serialWrite(const char *s){
	for (uint8_t i = 0; i < (uint8_t)strlen(s); i++){
		serialBuffer[serialWritePos] = s[i];
		serialWritePos = (serialWritePos + 1) % TX_BUFFER_SIZE;
	}
	UCSR0B |= (1 << UDRIE0);   // habilita ISR UDRE
}

// Lectura  de RX

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

void I2C_init(void) {
	TWSR = 0x00;            // Prescaler = 1
	TWBR = ((F_CPU / 100000UL) - 16) / 2;  // 100 kHz --> 72
	TWCR = (1<<TWEN);       // Habilitar TWI para usar SDA y SCL
}

static inline void I2C_start(void) {
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN); // Condicion de start de I2C --> TWSTA
	while(!(TWCR & (1<<TWINT))); // Esperar a que termine la accion
}

static inline void I2C_stop(void) {
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN); // TWSTO para terminar
}

static inline uint8_t I2C_write(uint8_t v) {
	TWDR = v; // BYTE de datos a transmitir ya echo para escritura
	TWCR = (1<<TWINT)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT))); // Esperar a que termine la accion
	uint8_t s = TWSR & 0xF8; 	// 0x18: SLA+W ACK, 0x28: DATA ACK  (Verifica si el esclavo acepto la escritura o si existe)
	if (s == 0x18 || s == 0x28){
		return true;
	}
	else{
		return false;
	}
}


static uint8_t pcf8574_autodetect(void) { // Funcion para detectar el esclavo
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
static inline void lcd_cmd(uint8_t c)    { lcd_send(c, 0); } // Para comandos
static inline void lcd_data(uint8_t d)   { lcd_send(d, 1); } // Para escritura

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
	_delay_ms(50);
	// Autodetecta direccion
	uint8_t found = pcf8574_autodetect();
	if (found) PCF_ADDR = found;
	/*PCF_ADDR = 0x27;*/
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
static inline char poll_switch_portc(void) {
	// Con pull-ups: presionado = 0 (activo en bajo)
	uint8_t p = PINC;
	if (!(p & (1<<PC0))) { _delay_ms(15); if (!(PINC & (1<<PC0))) { C = 1; return '1'; } }
	if (!(p & (1<<PC1))) { _delay_ms(15); if (!(PINC & (1<<PC1))) { C = 2; return '2'; } }
	if (!(p & (1<<PC2))) { _delay_ms(15); if (!(PINC & (1<<PC2))) { C = 3; return '3'; } }
	return '\0';
}



