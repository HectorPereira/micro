/*
 * main.c
 *
 * Created: 10/6/2025 7:11:20 PM
 *  Author: isacm
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define F_CPU 16000000UL    // Frecuencia del reloj del micro (16 MHz)
#define BAUD 9600           // Velocidad de transmisión (baudios)
#define BRC ((F_CPU / 16 / BAUD) - 1)   // Valor para UBRR
#define TX_BUFFER_SIZE 128
// ---- Buffer RX circular ----
#define RX_BUFFER_SIZE 128


// Set Bit in IO register
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

// Clear Bit in IO register
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))


// Si los usás dentro de una ISR, marcá como volatile
volatile char    rxBuffer[RX_BUFFER_SIZE];
volatile uint8_t rxReadPos  = 0;
volatile uint8_t rxWritePos = 0;
volatile uint8_t CONTADOR = 0; // La idea es usarlo para las figuras simples


char    serialBuffer[TX_BUFFER_SIZE];
uint8_t serialReadPos  = 0;
uint8_t serialWritePos = 0;


void appendSerial(char c);
void serialWrite(const char *c);
char peekChar(void);
char Chardos(void);

void Centrar(void);
void Hacer_cuadrado(void);


int main(void)
{
	
	UBRR0H = (BRC >> 8);
	UBRR0L = BRC;
	
	TCCR0A = 0x00;
	TCCR0B |=  (1 << CS02) | (0 << CS01) | (1 << CS00); //Timer 1024
	TIMSK0 |=  (1 << TOIE0);
	
	// Habilitar transmisor
	
	UCSR0B = (1 << TXEN0) | (1 << TXCIE0) | (1 << RXEN0) | (1 << RXCIE0);
	 // RXENn: Receiver Enable n ; TXCIEn: TX Complete Interrupt Enable n ;  TXENn: Transmitter Enable n; RXCIEn:  RX Complete Interrupt Enable n

	// Modo asincrónico, 8 bits, 1 stop, sin paridad
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
	
	sei();
	
	DDRB |= (1 << PORTB0);
	DDRD |= 0b11111100;
	
	serialWrite("\nSelecciona la figura a dibujar:\n");
	serialWrite("1 - Circulo\n");
	serialWrite("2 - Cuadrado\n");
	serialWrite("3 - Triangulo");
	
	_delay_ms(5);
	
    while(1)
    {
		char c = Chardos();

		if (c == '2')
		{
			Centrar();// Hacer el cuadrado
		}
		else if (c == '0')
		{
			cbi(PORTD, 4);
		}

    }
}


void appendSerial(char c)
{
	serialBuffer[serialWritePos] = c;
	serialWritePos++;

	if (serialWritePos >= TX_BUFFER_SIZE) {
		serialWritePos = 0;
	}
}

void serialWrite(const char *c)
{
	for (uint8_t i = 0; i < strlen(c); i++)
	{
		appendSerial(c[i]);
	}
	if (UCSR0A & (1 << UDRE0))
	{
		UDR0 = 0;
	}
}

ISR(USART_TX_vect)
{
	if (serialReadPos != serialWritePos)
	{
		UDR0 = serialBuffer[serialReadPos];
		serialReadPos++;

		if (serialReadPos >= TX_BUFFER_SIZE)
		{
			serialReadPos = 0;  // Reinicia al inicio del buffer
		}
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

ISR(TIMER1_OVF_vect){
	CONTADOR += 1;
}


void Centrar(void){
	if(CONTADOR < 2)
	{
		sbi(PORTD, 4);   // Pone en 1 el bit 2 de PORTD
	}
	else if (2 < CONTADOR  & 2 > CONTADOR)
	{
		cbi(PORTD, 4);
		sbi(PORTD, 5);
	}
}

void Hacer_cuadrado(void){
	
}


