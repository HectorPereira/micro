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
void dibujar_triangulo(void);
void dibujar_cuadrado(void);

int main(void)
{
	
	UBRR0H = (BRC >> 8);
	UBRR0L = BRC;
	
	TCCR1A = 0x00;
	TCCR1B |=  (1 << CS02) | (0 << CS01) | (1 << CS00); //Timer 64
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
		if (c == '2'){
			dibujar_triangulo();
		}
		//peurbas(c);
		}
		
		

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

 //Para hacer cada pixel

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
	
	while(CONTADOR < 4){
	AbajoDerecha();	
	}
	apagar();
}


void apagar(void){
chan = false;
PORTD = (PORTD & 0b00000011) | 0b00001000;

}


void Subir_s(void)
{
PORTD = (PORTD & 0b00000011) | 0b00000100;
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




