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

// Seccion para USART
#define F_CPU 16000000UL    // Frecuencia del reloj del micro (16 MHz)
#define BAUD 9600           // Velocidad de transmisión (baudios)
#define BRC ((F_CPU / 16 / BAUD) - 1)   // Valor para UBRR
#define TX_BUFFER_SIZE 128
#define RX_BUFFER_SIZE 128
#define precarger 10000



volatile char    serialBuffer[TX_BUFFER_SIZE];
volatile uint8_t serialReadPos  = 0;
volatile uint8_t serialWritePos = 0;

volatile char    rxBuffer[RX_BUFFER_SIZE];
volatile uint8_t rxReadPos  = 0;
volatile uint8_t rxWritePos = 0;
volatile uint8_t CONTADOR = 0; // La idea es usarlo para mover el puntero
volatile uint8_t CONTADOR2 = 0; // La idea es usarlo para las pausas
volatile bool chan = true; // La idea es usarlo para las figuras simples

void appendSerial(char c);
void serialWrite(const char *c);
char peekChar(void);
char Chardos(void);


// macro para setear
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

// macro para resetear
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))



// Seccion para algoritmo de dibujar
#define N 8


uint8_t clamp8(int v) {
	if (v < 0)   return 0;
	if (v >= N)  return N - 1;
	return (uint8_t)v;
}


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
void dibujar_cuadrado(void);

int main(void)
{
	
	UBRR0H = (BRC >> 8);
	UBRR0L = BRC;
	
	TCCR1A = 0x00;
	TCCR1B |=  (0 << CS02) | (1 << CS01) | (1 << CS00); //Timer 64
	TIMSK1 |=  (1 << TOIE1);
	
	// Habilitar transmisor
	
	
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
		
		if (c == '1'){
			dibujar_cuadrado();
		}
		
		peurbas(c);
		}
		
		

    }
  
    

void dibujar_cuadrado(void){
	CONTADOR = 0;

	int n = 1;
	
	Subir_s();
	
	// 0–2 s: ARRIBA
	while (CONTADOR < 1) {
		Subir();
		// opcional: serialWrite("Arriba\n");
	}

	// 2–4 s: DERECHA
	while (CONTADOR < 2) {
		Derecha();
		// opcional: serialWrite("Derecha\n");
	}

	// 4–6 s: ABAJO
	while (CONTADOR < 3) {
		Bajar();
		// opcional: serialWrite("Abajo\n");
	}

	// 6–8 s: IZQUIERDA
	while (CONTADOR < 4) {
		Izquierda();
		// opcional: serialWrite("Izquierda\n");
	}
	apagar();        // detener salidas al terminar el cuadrado
}
 //Para hacer cada pixel

// Con TCNT1\H = 0xC2; TCNT1L = 0xF7; 19 bit en x , 21 en y
void peurbas(char c){
	switch (c)
	{
		case 'x':  // apagar todo
		apagar();
		break;

		case 's':  // subir solenoide (D2)
		Subir_s();
		break;

		case 'u':  // arriba (D5)
		Subir();
		break;

		case 'd':  // abajo (D4)
		Bajar();
		break;

		case 'l':  // izquierda (D6)
		Izquierda();
		break;

		case 'r':  // derecha (D7)
		Derecha();
		break;

		case 'z':  // abajo-izquierda (D4 + D6)
		AbajoIzquierda();
		break;

		case 'c':  // abajo-derecha (D4 + D7)
		AbajoDerecha();
		break;

		case 'q':  // arriba-izquierda (D5 + D6)
		ArribaIzquierda();
		break;

		case 'e':  // arriba-derecha (D5 + D7)
		ArribaDerecha();
		
		case 'j':  // arriba-derecha (D5 + D7)
		centrar();
		break;

		default:
		// opcional: no-op o mensaje de comando inválido
		break;
	}
}

void centrar(void){
	CONTADOR = 0;
	TCNT1H = 0xC2;
	TCNT1L = 0xF7;
	
	while(CONTADOR < 2){
	Bajar();
	}
	while(CONTADOR < 4){
	Derecha();	
	}
	apagar();
}


void apagar(void){
chan = false;
PORTD = (PORTD & 0b00000011) | 0b00001000;

}


void Subir_s(void)
{
PORTD = (PORTD & 0b00001111) | 0b00000100;
chan = false;

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
	TCNT1H = 0xC2;
	TCNT1L = 0xF7;
	if(chan){
	CONTADOR ++;	
	} else {
		if (5 < CONTADOR2){
			chan = true;
		}
	}
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




