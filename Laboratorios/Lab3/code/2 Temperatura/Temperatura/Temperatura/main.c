#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/twi.h>


#define BAUD 9600UL
#define UBRR_VALUE ((F_CPU/16/BAUD) - 1)
#define TX_BUFFER_SIZE 128
#define RX_BUFFER_SIZE 128
#define precarger 10000

#define VENT_PORT  PORTB
#define VENT_DDR   DDRB

#define VENT_PIN   PB4   // D12 en Arduino UNO
uint8_t contolar = 0;

// preload = 65536 - 15625 = 49911 = 0xC2F7
#define T1_PRELOAD   (uint16_t)(65536UL - (F_CPU/1024UL))


char string_to_send[4] = ""; 
uint16_t valor = 0;

void Init_pwm();
void Init_adc();


// macro para setear
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

// macro para resetear
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))

#define UART_TIMEOUT_MS 500


void spi_init(void);

uint8_t spi_transfer(uint8_t data);

void uart_init(unsigned int ubrr);

char uart_receive(void);

void uart_send(char c);

void uart_print(const char *s);

void uart_print_hex(uint16_t val);

void uart_print_dec(uint16_t val);
void enviar_temperatura(uint16_t tmin, uint16_t tmax);
char Number_to_ascii(uint8_t val);

uint16_t adc_read_blocking_adif(void);

void cambiar_rango(uint16_t *tmin, uint16_t *tmax);
static void pedir_u16_linea(const char *prompt, uint16_t *out, uint16_t vmin, uint16_t vmax);


void uart_print_hex_array(const uint8_t *arr, uint8_t len);

static inline void Init_ventilador(void);
static inline void Ventilador_on(void);
static inline void Ventilador_off(void);

void medicion(void); 

void init_timer(void);


volatile char    serialBuffer[TX_BUFFER_SIZE];
volatile uint8_t serialReadPos  = 0;
volatile uint8_t serialWritePos = 0;

volatile char    rxBuffer[RX_BUFFER_SIZE];
volatile uint8_t rxReadPos  = 0;
volatile uint8_t rxWritePos = 0;

void appendSerial(char c);
void serialWrite(const char *c);
char peekChar(void);
char Chardos(void);
char usart_recibirDato(void);
char * usart_recibirCadena(void);
bool usart_readstring(char *dst, uint8_t cap);
bool ascii_to_u16_switch(const char *s, uint16_t *out);

int main(void)
{
	
	uart_init(UBRR_VALUE);
	

	// RX por interrupción
	UCSR0B |= (1<<RXCIE0);


	Init_pwm();
	uart_init(UBRR_VALUE);
	Init_adc();
	init_timer();
	sei();
	Init_ventilador();
	Ventilador_off();
	
	float tC;
	uint16_t tC2 = 25; //Temperatura minima
	uint16_t tC3 = 30; //Temperatura maxima
	
	uint16_t adc = 0;
	
	OCR0A = 0;
			
	uart_print("Si quiere prender el sistema escriba Encender\r\n");
	//uart_print("Una vez encendido, escriba change para cambiar el punto medio\r\n");
	
	char c[2];	
	char u[64];      
	u[0] = '\0';
    while(1)
    {
	    
	   while (strcmp(u, "Encender") != 0) {
		   usart_readstring(u,  64 );
		   
		   if (strcmp(u, "Encender") == 0) {
				enviar_temperatura(tC2, tC3);
		   }
	   }
	   
	 
	//Ventilador_on();
	    
	    if(contolar){
		    string_to_send[0] = '\0';
		    uint16_t div = 1;
		    
		    adc = adc_read_blocking_adif();
		    
		    tC = ((adc * 500.0f) / 1023.0f );
		    
		    valor = tC;
		    
		    while (valor / div >= 10) {
			    div *= 10;
		    }
		    
		    for (div ; div > 0; div /= 10) {
			    uint8_t d = (valor / div) % 10;
			    add_string(string_to_send, Number_to_ascii(d));
		    }
		    
		    uart_print("\r\n");
		    uart_print(string_to_send);
		    uart_print("\r\n");
		    contolar = 0;
		 
		static uint8_t heater_on = 0;   // 1 = encendido, 0 = apagado

		if (tC <= tC2) {
			heater_on = 1;              // por debajo del mínimo -> encender
			Ventilador_off();
			} else if (tC >= tC3) {
			heater_on = 0; 
			Ventilador_on();             // por encima del máximo -> apagar
		}
		// dentro de la banda [tC2, tC3] se mantiene el último estado
		OCR0A = heater_on ? 100 : 0;
		
		char k = Chardos();
		if (k == '1') {
			cambiar_rango(&tC2, &tC3);
			enviar_temperatura(tC2, tC3);
		}
     }
 	
	}
}


bool ascii_to_u16_switch(const char *s, uint16_t *out) {
	uint8_t digs[5];          // hasta 5 dígitos (0..65535)
	uint8_t n = 0;

	// 1) Leer y validar dígitos (corta en CR/LF)
	for (; *s && *s!='\r' && *s!='\n'; ++s) {
		char c = *s;
		uint8_t d;
		switch (c) {
			case '0': d = '0' - '0'; break;
			case '1': d = '1' - '0'; break;
			case '2': d = '2' - '0'; break;
			case '3': d = '3' - '0'; break;
			case '4': d = '4' - '0'; break;
			case '5': d = '5' - '0'; break;
			case '6': d = '6' - '0'; break;
			case '7': d = '7' - '0'; break;
			case '8': d = '8' - '0'; break;
			case '9': d = '9' - '0'; break;
			default: return false;         // carácter no numérico
		}
		if (n >= 5) return false;          // más de 5 dígitos (posible overflow)
		digs[n++] = d;
	}
	if (n == 0) return false;              // cadena vacía

	// 2) Recombinar: unidades, decenas, centenas, ...
	uint32_t val = 0, mult = 1;
	for (int8_t i = (int8_t)n - 1; i >= 0; --i) {
		val += (uint32_t)digs[i] * mult;   // suma de a 1,10,100,1000...
		mult *= 10;
	}
	if (val > 65535u) return false;        // límite de uint16_t

	*out = (uint16_t)val;
	return true;

}

bool usart_readstring(char *dst, uint8_t cap) {
	char c = Chardos();          // toma 1 char si hay; '\0' si no
	if (c == '\0') return false; // no llegó nada todavía

	if (c == '\r') return false; // ignorar CR
	if (c == '\n') return true;  // fin de línea

	
	 if (strlen(dst) < cap - 1) {add_string(dst, c);}

	return false;
}

char usart_recibirDato(void)
{
	while (!(UCSR0A & (1<<RXC0)));
	return(UDR0);
}

char * usart_recibirCadena(void)
{
	int longitud = 0;
	static char cadena[10];
	
	for (longitud = 0; longitud < 10; longitud++)
	{
		cadena[longitud] = usart_recibirDato();
	}
	
	return cadena;
}

void medicion(){
	contolar = 1;
}

ISR(TIMER1_OVF_vect) {
	TCNT1 = T1_PRELOAD;         // recarga para el próximo segundo
	medicion();                 // callback de usuario cada 1 s
}
//PIN 6 with fast PWM
void Init_pwm(){
    DDRD |= (1 << DDD6);

// Fast PWM (modo 3, TOP=255), salida no inversora en OC0A
    TCCR0A = (1 << WGM01) | (1 << WGM00) | (1 << COM0A1); // COM0A1=1, COM0A0=0
    TCCR0B = (1 << CS01) | (1 << CS00); // Prescaler = 64  (? 976 Hz @16MHz)
}


Init_adc(){
	ADMUX  = 0b01000001;
	ADCSRA = (1 << ADEN)| (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);   // ADPS=111 ? prescaler 128
	DIDR0  = (1 << ADC0D); // Desabilitado la entrada Digital
}


// Lectura  esperando ADIF=1 
uint16_t adc_read_blocking_adif(void) {
	ADCSRA |= (1 << ADIF);                       // Limpia flag previo
	ADCSRA |= (1 << ADSC);                       // Inicia conversión
	while (!(ADCSRA & (1 << ADIF))) { }          // Espera fin (ADIF=1)
	uint16_t v = ADC;                             // Lee resultado
	ADCSRA |= (1 << ADIF);                       // Limpia flag para la próxima
	return v;
}

void uart_init(unsigned int ubrr) {
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = (1<<TXEN0) | (1<<RXEN0) | (1<<RXCIE0);

	UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);
}

char uart_receive(void) {
	while (!(UCSR0A & (1<<RXC0)));
	return UDR0;
}

void uart_send(char c) {
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = c;
}

void uart_print(const char *s) {
	while (*s) uart_send(*s++);
}

void uart_print_hex(uint16_t val) {
	char buf[6];
	sprintf(buf, "0x%02X", val);
	uart_print(buf);
}

void add_string(char *s, char c) {
	while (*s++);
	*(s - 1) = c;
	*s = '\0';
}

void uart_print_dec(uint16_t val){
	if (val == 0){ uart_send('0'); return; }                 // char ? uart_send
	if (val < 10){ uint8_t d = (uint8_t)(val % 10); char c = Number_to_ascii(d); add_string(string_to_send, c); return; }
	if (val >= 10){ uint8_t d = (uint8_t)((val/10) % 10); char c = Number_to_ascii(d); add_string(string_to_send, c); return; }
	if (val >= 100){ uint8_t d = (uint8_t)((val/100) % 10); char c = Number_to_ascii(d); add_string(string_to_send, c); return; }
	if (val >= 1000){ uint8_t d = (uint8_t)((val/1000) % 10); char c = Number_to_ascii(d); add_string(string_to_send, c); return; }
}



char Number_to_ascii(uint8_t val){
	switch (val) {
		case 0: return '0';
		case 1: return '1';
		case 2: return '2';
		case 3: return '3';
		case 4: return '4';
		case 5: return '5';
		case 6: return '6';
		case 7: return '7';
		case 8: return '8';
		case 9: return '9';
		
		default: return '?';
	}
}


void uart_print_hex_array(const uint8_t *arr, uint8_t len) {
	for (uint8_t i = 0; i < len; i++) {
		uart_print_hex(arr[i]);
		uart_send(' ');
	}
	uart_print("\r\n");
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

void init_timer(){
	cli();                      // opcional
	TCCR1A = 0;                 // modo normal (WGM13:0 = 0)
	TCCR1B = 0;
	TCNT1  = T1_PRELOAD;        // primer periodo completo de ~1 s
	TIMSK1 = (1 << TOIE1);      // habilita interrupción por overflow
	TCCR1B = (1 << CS12) | (1 << CS10);  // prescaler = 1024
	//sei();                      // habilita globales
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
ISR(USART_RX_vect)
{
	rxBuffer[rxWritePos] = UDR0;

	rxWritePos++;

	if (rxWritePos >= RX_BUFFER_SIZE)
	{
		rxWritePos = 0;
	}
}

// Lee una línea con usart_readstring() y la convierte a uint16 en [vmin..vmax]
static void pedir_u16_linea(const char *prompt, uint16_t *out, uint16_t vmin, uint16_t vmax) {
	char buf[8];
	uint16_t v;

	while (1) {
		uart_print(prompt);
		uart_print("\r\n");       // salto de línea para comodidad

		buf[0] = '\0';
		// Esperar hasta que usart_readstring finalice la línea ('\n')
		while (!usart_readstring(buf, sizeof(buf))) {
			// spin no bloqueante: usart_readstring consume del buffer RX
		}

		if (ascii_to_u16_switch(buf, &v) && v >= vmin && v <= vmax) {
			*out = v;
			return;
		}
		uart_print("Valor invalido. Intente nuevamente.\r\n");
	}
}

// Pide y valida nuevo rango [tmin..tmax] con tmax>tmin
void cambiar_rango(uint16_t *tmin, uint16_t *tmax) {
	uint16_t a, b;

	uart_print("\r\n--- CAMBIO DE RANGO ---\r\n");
	pedir_u16_linea("Ingrese tC2 (min, entero °C 0..150): ", &a, 0, 150);
	pedir_u16_linea("Ingrese tC3 (max, entero °C 0..150): ", &b, 0, 150);

	if (b <= a) {
		uart_print("tC3 debe ser estrictamente mayor que tC2. Operacion cancelada.\r\n");
		return;
	}

	*tmin = a;
	*tmax = b;
	uart_print("Rango actualizado.\r\n");
}


static inline void Init_ventilador(void) {
	sbi(VENT_DDR, VENT_PIN);   // como salida
	sbi(VENT_PORT, VENT_PIN);  // encender (activo en alto)
}

static inline void Ventilador_on(void)  { sbi(VENT_PORT, VENT_PIN); }
static inline void Ventilador_off(void) { cbi(VENT_PORT, VENT_PIN); }
	
void enviar_temperatura(uint16_t tmin, uint16_t tmax) {
	char buf[12];

	// Línea 1: la palabra "temp"
	uart_print("temp\r\n");

	// Línea 2: tC2 (mínima)
	sprintf(buf, "%u", (unsigned)tmin);
	uart_print(buf);
	uart_print("\r\n");

	// Línea 3: tC3 (máxima)
	sprintf(buf, "%u", (unsigned)tmax);
	uart_print(buf);
	uart_print("\r\n");
}