/*
 * main.c
 *
 * Created: 10/6/2025 7:11:20 PM
 *  Author: isacm
 */ 
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <avr/pgmspace.h>

// Seccion para USART
#define F_CPU 16000000UL    // Frecuencia del reloj del micro (16 MHz)
#define BAUD 9600           // Velocidad de transmisión (baudios)
#define BRC ((F_CPU / 16 / BAUD) - 1)   // Valor para UBRR
#define TX_BUFFER_SIZE 128
#define RX_BUFFER_SIZE 128
#define precarger 10000

// Direcciones base
#define SOLENOID_DOWN   1
#define SOLENOID_UP     2
#define DOWN            3
#define UP              4
#define RIGHT           5
#define LEFT            6
#define STOP            7

// Diagonales 
#define UP_RIGHT        8
#define UP_LEFT         9
#define DOWN_RIGHT      10
#define DOWN_LEFT       11


volatile char    serialBuffer[TX_BUFFER_SIZE];
volatile uint8_t serialReadPos  = 0;
volatile uint8_t serialWritePos = 0;

volatile char    rxBuffer[RX_BUFFER_SIZE];
volatile uint8_t rxReadPos  = 0;
volatile uint8_t rxWritePos = 0;
volatile uint8_t CONTADOR = 0; // La idea es usarlo para mover el puntero
volatile const uint8_t radio = 10; // La idea es usarlo para las figuras simples

void appendSerial(char c);
void serialWrite(const char *c);
char peekChar(void);
char Chardos(void);


// macro para setear
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

// macro para resetear
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))


void Subir(void);
void Bajar(void);
void Izquierda(void);
void Derecha(void);
void diagI_sup(void);
void diagI_inf(void);
void diagD_sup(void);
void diagD_inf(void);
void X(void);
void Y(void);

void centrar(void);
void peurbas(char c);
void dibujar_triangulo(void);
void dibujar_cuadrado(void);
void dibujar_cruz(void);
static inline void generar_escalon(uint16_t ang_deg, const uint16_t tR[91], const uint16_t tU[91]);
void Hacer_circulo(void);
void Hacer_circulo_lut(void);
void ejecutar_circulo(const uint8_t *tabla, uint16_t size);
	
// Solo los comandos (combinaciones separadas por comas)
const uint8_t CIRCLE_DATA[] = {
	SOLENOID_DOWN,

	// Quadrant 1: RIGHT & UP
	UP, UP_RIGHT, UP, UP_RIGHT, UP, UP_RIGHT, UP, UP_RIGHT,
	UP, UP_RIGHT, UP, UP_RIGHT, UP, UP_RIGHT, UP, UP_RIGHT,
	UP, UP_RIGHT, UP, UP_RIGHT, UP, UP_RIGHT, UP, UP_RIGHT,
	UP, UP_RIGHT, UP, UP_RIGHT, UP, UP_RIGHT, UP, UP_RIGHT,
	RIGHT, UP_RIGHT, RIGHT, UP_RIGHT, RIGHT, UP_RIGHT,
	RIGHT, UP_RIGHT, RIGHT, UP_RIGHT, RIGHT, UP_RIGHT,
	RIGHT, UP_RIGHT, RIGHT, UP_RIGHT, RIGHT, UP_RIGHT,
	RIGHT, UP_RIGHT, RIGHT, UP_RIGHT, RIGHT, UP_RIGHT,

	// Quadrant 2: DOWN & RIGHT
	RIGHT, DOWN_RIGHT, RIGHT, DOWN_RIGHT, RIGHT, DOWN_RIGHT,
	RIGHT, DOWN_RIGHT, RIGHT, DOWN_RIGHT, RIGHT, DOWN_RIGHT,
	RIGHT, DOWN_RIGHT, RIGHT, DOWN_RIGHT, RIGHT, DOWN_RIGHT,
	RIGHT, DOWN_RIGHT, RIGHT, DOWN_RIGHT, RIGHT, DOWN_RIGHT,
	DOWN, DOWN_RIGHT, DOWN, DOWN_RIGHT, DOWN, DOWN_RIGHT,
	DOWN, DOWN_RIGHT, DOWN, DOWN_RIGHT, DOWN, DOWN_RIGHT,
	DOWN, DOWN_RIGHT, DOWN, DOWN_RIGHT, DOWN, DOWN_RIGHT,
	DOWN, DOWN_RIGHT, DOWN, DOWN_RIGHT, DOWN, DOWN_RIGHT,

	// Quadrant 3: LEFT & DOWN
	DOWN, DOWN_LEFT, DOWN, DOWN_LEFT, DOWN, DOWN_LEFT,
	DOWN, DOWN_LEFT, DOWN, DOWN_LEFT, DOWN, DOWN_LEFT,
	DOWN, DOWN_LEFT, DOWN, DOWN_LEFT, DOWN, DOWN_LEFT,
	DOWN, DOWN_LEFT, DOWN, DOWN_LEFT, DOWN, DOWN_LEFT,
	LEFT, DOWN_LEFT, LEFT, DOWN_LEFT, LEFT, DOWN_LEFT,
	LEFT, DOWN_LEFT, LEFT, DOWN_LEFT, LEFT, DOWN_LEFT,
	LEFT, DOWN_LEFT, LEFT, DOWN_LEFT, LEFT, DOWN_LEFT,
	LEFT, DOWN_LEFT, LEFT, DOWN_LEFT, LEFT, DOWN_LEFT,

	// Quadrant 4: UP & LEFT
	LEFT, UP_LEFT, LEFT, UP_LEFT, LEFT, UP_LEFT,
	LEFT, UP_LEFT, LEFT, UP_LEFT, LEFT, UP_LEFT,
	LEFT, UP_LEFT, LEFT, UP_LEFT, LEFT, UP_LEFT,
	LEFT, UP_LEFT, LEFT, UP_LEFT, LEFT, UP_LEFT,
	UP, UP_LEFT, UP, UP_LEFT, UP, UP_LEFT,
	UP, UP_LEFT, UP, UP_LEFT, UP, UP_LEFT,
	UP, UP_LEFT, UP, UP_LEFT, UP, UP_LEFT,
	UP, UP_LEFT, UP, UP_LEFT, UP, UP_LEFT,

	SOLENOID_UP
};


const uint8_t CIRCLE_DATA_TIME[] PROGMEM = {
	255,

	// Quadrant 1
	100,1,96,5,92,8,88,10,85,19,81,23,77,27,73,32,
	68,36,64,40,60,45,55,50,50,54,46,59,41,65,35,70,
	30,76,24,82,18,89,11,96,4,96,4,89,11,82,18,76,
	24,70,30,65,35,59,41,54,46,50,50,45,55,40,60,36,
	64,32,68,27,73,23,77,19,81,15,85,12,88,8,92,4,
	96,100,

	// Quadrant 2
	100,4,96,8,92,12,88,15,85,19,81,23,77,27,73,32,
	68,36,64,40,60,45,55,50,50,54,46,59,41,65,35,70,
	30,76,24,82,18,89,11,96,4,96,4,89,11,82,18,76,
	24,70,30,65,35,59,41,54,46,50,50,45,55,40,60,36,
	64,32,68,27,73,23,77,19,81,15,85,12,88,8,92,4,
	96,100,

	// Quadrant 3
	100,4,96,8,92,12,88,15,85,19,81,23,77,27,73,32,
	68,36,64,40,60,45,55,50,50,54,46,59,41,65,35,70,
	30,76,24,82,18,89,11,96,4,96,4,89,11,82,18,76,
	24,70,30,65,35,59,41,54,46,50,50,45,55,40,60,36,
	64,32,68,27,73,23,77,19,81,15,85,12,88,8,92,4,
	96,100,

	// Quadrant 4
	100,4,96,8,92,12,88,15,85,19,81,23,77,27,73,32,
	68,36,64,40,60,45,55,50,50,54,46,59,41,65,35,70,
	30,76,24,82,18,89,11,96,4,96,4,89,11,82,18,76,
	24,70,30,65,35,62,41,57,46,57,50,48,55,44,60,40,
	64,36,68,30,73,26,79,23,85,18,87,20,90,10,100,150,
	130,255,1
};

int main(void)
{
	
	UBRR0H = (BRC >> 8);
	UBRR0L = BRC;
	
	timer1_init_1ms();
	
	UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);
	
	// Modo asincrónico, 8 bits, 1 stop, sin paridad
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
	
	sei();
	
	DDRB |= (1 << PORTB0);
 	DDRD = 0b11111110;
	
	
	
	serialWrite("\nSelecciona la figura a dibujar:\n");
	serialWrite("1 - Triangulo\n");
	
	_delay_ms(5);
	
    while(1)
    {
	    char c = Chardos();
	    if (c != '\0') {
		    serialWrite("Recibido: ");
		    appendSerial(c);
		    appendSerial('\n');
	    }
	    
	    if (c == '1') {
		    ejecutar_circulo_sinc(CIRCLE_DATA, CIRCLE_DATA_TIME, sizeof(CIRCLE_DATA));
	    }
	    else {
		    peurbas(c);
	    }
    }

    }
 
void timer1_init_1ms(void) {
	// CTC: WGM12 = 1, WGM13:0 = 0100
	TCCR1A = 0;
	TCCR1B = 0;
	TCCR1B |= (1 << WGM12);

	// OCR1A = F_CPU / (prescaler * 1000) - 1
	// Con prescaler 64 y F_CPU=16 MHz -> 16000000/64/1000 - 1 = 249
	OCR1A = (uint16_t)(F_CPU / 64UL / 1000UL - 1);

	// Arrancar con prescaler = 64 (CS11=1, CS10=1)
	TCCR1B |= (1 << CS11) | (1 << CS10);

	// Limpia cualquier bandera pendiente
	TIFR1 = (1 << OCF1A);
}

static inline void delay_ms_timer1(uint16_t ms) {
	while (ms--) {
		// Reinicia conteo y bandera
		TCNT1 = 0;
		TIFR1 = (1 << OCF1A);
		// Espera hasta que ocurra la comparación (1 ms)
		while ((TIFR1 & (1 << OCF1A)) == 0) { /* espera */ }
	}
}

void precompute_semicirculo_arrays_90(uint16_t I_ms,
uint16_t tR90[90], uint16_t tU90[90]) {
	for (uint16_t i = 0; i < 90; ++i) {          // i => 0..89  (0°,2°,4°,...,178°)
		uint16_t semi = i * 2;                   // ángulo en 0..178
		uint16_t base = (semi <= 90) ? semi : (180 - semi); // 0..90 (simetría)
		float th = (float)base * (float)M_PI / 180.0f;
		float c = cosf(th);
		float s = sinf(th);
		if (c < 0) c = 0; else if (c > 1) c = 1;
		if (s < 0) s = 0; else if (s > 1) s = 1;
		tR90[i] = (uint16_t)lrintf(c * (float)I_ms);
		tU90[i] = (uint16_t)lrintf(s * (float)I_ms);
	}
}


// ---- Un solo bucle, 0.5° por paso (0..719). Sin otras funciones salvo precálculo y delay. ----
void Hacer_circulo(void) {
	const uint16_t I_ms = 500;          // escala por escalón (ajusta a tu plotter)
	uint16_t tR180[180], tU180[180];   // 180 valores decimales por semicírculo

	precompute_semicirculo_arrays_90(I_ms, tR180, tU180);

	Subir_s();
	delay_ms_timer1(100);

	cli();
	for (uint16_t ang_u = 0; ang_u < 720; ++ang_u) {   // 0..719 (cada unidad = 0.5°)
		// Saltar bordes exactos de cuadrante: 90°, 180°, 270° -> 180, 360, 540 unidades
		if (ang_u == 180 || ang_u == 360 || ang_u == 540) continue;

		uint16_t q     = ang_u / 180;          // 0..3 cuadrante (180 unidades = 90°)
		uint16_t local = ang_u % 180;          // 0..179 índice en tR180/tU180 (0.5°)

		switch (q) {
			case 0: // Q1: Subir -> Derecha
			Subir();
			delay_ms_timer1(tR180[local]);
			Derecha();
			delay_ms_timer1(tU180[local]);
			break;

			case 1: // Q2: Derecha -> Bajar
			Derecha();
			delay_ms_timer1(tR180[local]);
			Bajar();
			delay_ms_timer1(tU180[local]);
			break;

			case 2: // Q3: Bajar -> Izquierda
			Bajar();
			delay_ms_timer1(tR180[local]);
			Izquierda();
			delay_ms_timer1(tU180[local]);
			break;

			default: // Q4: Izquierda -> Subir
			Izquierda();
			delay_ms_timer1(tR180[local]);
			Subir();
			delay_ms_timer1(tU180[local]);
			break;
		}
	}
	sei();

	delay_ms_timer1(100);
	apagar();
}




 void dibujar_cruz(void){
	 centrar();
	 
	 CONTADOR = 0;
	 TCNT1H = 0xC2;
	 TCNT1L = 0xF7;
		Subir_s();
		
		
	 while (CONTADOR < 4) { ArribaDerecha();   }   // 2 s
	 while (CONTADOR < 6) { AbajoIzquierda();   }   // 2 s
	 while (CONTADOR < 8) {AbajoDerecha();     }   // 2 s
	 while (CONTADOR < 12) {ArribaIzquierda();     }   // 2 s
	 
 }
 
 void dibujar_triangulo(void){
		centrar();
		
		Subir_s();
	
		CONTADOR = 0;
		TCNT1H = 0xC2;
		TCNT1L = 0xF7;
		
		while (CONTADOR < 2) { Bajar();     }   // 2 s
		while (CONTADOR < 3) { ArribaDerecha();   }   // 2 s
		while (CONTADOR < 4) { ArribaIzquierda();     }   // 2 s
			
		apagar();
		
 }
		
 void dibujar_cuadrado(void){
	 
	_delay_ms(100);
	 Subir_s();          
	 _delay_ms(100);
	CONTADOR = 0;
	
	
	 while (CONTADOR < 2) { Subir();     }   // 2 s
	 while (CONTADOR < 4) { Derecha();   }   // 2 s
	 while (CONTADOR < 6) { Bajar();     }   // 2 s
	 while (CONTADOR < 8) { Izquierda(); }   // 2 s
	 //x
	 
	  _delay_ms(100);
	 apagar();
	 _delay_ms(100);
	 
 }


void ejecutar_circulo_sinc(const uint8_t *tablaDir, const uint8_t *tablaTime, uint16_t size) {
	uint8_t cmd;
	uint8_t t;

	_delay_ms(1000);  // Espera inicial

	for (uint16_t i = 0; i < size; i++) {
		cmd = pgm_read_byte(&tablaDir[i]);   // dirección
		t   = pgm_read_byte(&tablaTime[i]);  // tiempo correspondiente

		if (cmd == STOP)
		break;

		// Ejecutar la acción según comando
		switch (cmd) {
			case UP:            Subir(); break;
			case DOWN:          Bajar(); break;
			case LEFT:          Izquierda(); break;
			case RIGHT:         Derecha(); break;

			case UP_LEFT:       ArribaIzquierda(); break;
			case UP_RIGHT:      ArribaDerecha(); break;
			case DOWN_LEFT:     AbajoIzquierda(); break;
			case DOWN_RIGHT:    AbajoDerecha(); break;

			case SOLENOID_UP:   apagar(); break;
			case SOLENOID_DOWN: Subir_s(); break;

			default:            apagar(); break;
		}

		for (uint8_t j = 0; j < t; j++) {
			_delay_ms(10);
		}

	}

	_delay_ms(1000);  // Espera final
	apagar();
}

void peurbas(char c){
	switch (c) {
		case 'x': apagar(); break;
		case 's': Subir_s(); break;
		case 'u': Subir(); break;
		case 'd': Bajar(); break;
		case 'l': Izquierda(); break;
		case 'r': Derecha(); break;
		case 'z': AbajoIzquierda(); break;
		case 'c': AbajoDerecha(); break;
		case 'q': ArribaIzquierda(); break;
		case 'e': ArribaDerecha(); break;   // <-- faltaba break
		case 'j': centrar(); break;
		default: break;
	}
}

void centrar(void){
	CONTADOR = 0;
	TCNT1H = 0xC2;
	TCNT1L = 0xF7;
	
	while(CONTADOR < 6){
		AbajoDerecha();
	}
	apagar();
}


void apagar(void){

PORTD = (PORTD & 0b00000011) | 0b00001000;

}

void apagar2(void){

	PORTD = (PORTD & 0b00001111) | 0b00000000;

}
void Subir_s(void)
{
PORTD = (PORTD & 0b00000011) | 0b00000100;


}

void Bajar(void)
{
PORTD = (PORTD & 0b00001111) | 0b00010000;
}

void Subir(void)
{
PORTD = (PORTD & 0b00001111) | 0b00100000;
}

void Izquierda(void)
{
PORTD = (PORTD & 0b00001111) | 0b01000000;
}


void Derecha(void)
{
PORTD = (PORTD & 0b00001111) | 0b10000000;
}

void AbajoIzquierda(void)
{
	PORTD = (PORTD & 0b00001111) | 0b01010000; // D6 y D4
}

void AbajoDerecha(void){
	PORTD = (PORTD & 0b00001111) | 0b10010000; // D6 y D4
}

void ArribaIzquierda(void)
{
	PORTD = (PORTD & 0b00001111) | 0b01100000; // D3 y D4
}

void ArribaDerecha(void){
	PORTD = (PORTD & 0b00001111) | 0b10100000; // D6 y D4
}


ISR(TIMER1_OVF_vect) {
	CONTADOR++;
	TCNT1H = 0xC2;
	TCNT1L = 0xF7;
}




void appendSerial(char c)
{
	serialBuffer[serialWritePos] = c;
	serialWritePos++;

	if (serialWritePos >= TX_BUFFER_SIZE) {
		serialWritePos = 0;   // wrap-around
	}
}
void serialWrite(const char *s){
	for (uint8_t i = 0; i < (uint8_t)strlen(s); i++){
		serialBuffer[serialWritePos] = s[i];
		serialWritePos = (serialWritePos + 1) % TX_BUFFER_SIZE;
	}
	UCSR0B |= (1 << UDRIE0);   // habilita ISR UDRE
}
ISR(USART_UDRE_vect){
	if (serialReadPos != serialWritePos){
		UDR0 = serialBuffer[serialReadPos];
		serialReadPos = (serialReadPos + 1) % TX_BUFFER_SIZE;
		} else {
		UCSR0B &= ~(1 << UDRIE0);  // nada más que enviar
	}
}
char peekChar(void)
{
	char ret = '\0';

	if (rxReadPos != rxWritePos)
	{
		ret = rxBuffer[rxReadPos];
	}

	return ret;
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



