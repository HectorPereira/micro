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

#define N 8

// Estado actual del "cursor"
uint8_t fila = 0, col = 0;


uint8_t clamp8(int v) {
	if (v < 0)   return 0;
	if (v >= N)  return N - 1;
	return (uint8_t)v;
}

// Inicializa (fila,col) buscando el único '1' en la matriz.
// Si no lo encuentra, deja (0,0) y escribe un 1 ahí.
void posicion_init(uint8_t M[N][N]) {
	for (uint8_t i = 0; i < N; i++) {
		for (uint8_t j = 0; j < N; j++) {
			if (M[i][j] == 1) { fila = i; col = j; return; }
		}
	}
	fila = 0; col = 0;
	M[0][0] = 1;
}

// Mueve por delta (df, dc) con choque en bordes
void mover_delta(uint8_t M[N][N], int df, int dc) {
	M[fila][col] = 0;                          // borra posición actual
	fila = clamp8((int)fila + df);             // actualiza con límites
	col  = clamp8((int)col  + dc);
	M[fila][col] = 1;                          // escribe nueva posición
}

// --- Movimientos cardinales ---
void mover_arriba   (uint8_t M[N][N]) { mover_delta(M, -1,  0); }
void mover_abajo    (uint8_t M[N][N]) { mover_delta(M,  1,  0); }
void mover_izquierda(uint8_t M[N][N]) { mover_delta(M,  0, -1); }
void mover_derecha  (uint8_t M[N][N]) { mover_delta(M,  0,  1); }

// --- Diagonales ---
void mover_arr_izq(uint8_t M[N][N]) { mover_delta(M, -1, -1); }
void mover_arr_der(uint8_t M[N][N]) { mover_delta(M, -1,  1); }
void mover_aba_izq(uint8_t M[N][N]) { mover_delta(M,  1, -1); }
void mover_aba_der(uint8_t M[N][N]) { mover_delta(M,  1,  1); }
	
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
volatile uint8_t CONTADOR = 0; // La idea es usarlo para las figuras simples

void appendSerial(char c);
void serialWrite(const char *c);
char peekChar(void);
char Chardos(void);


// macro para setear
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

// macro para resetear
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))




// Matrices para dibujar
uint8_t Posicion[8][8] = {
	{0,0,0,0,0,0,0,1},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0}
};

uint8_t mascara[8][8] = {
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0}
}; 

uint8_t Triangulo[8][8] = {
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,1,0,0,0},
	{0,0,0,1,0,1,0,0},
	{0,0,1,0,0,0,1,0},
	{0,1,1,1,1,1,1,1},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0}
};
//Funciones para moverse en la matriz

void Subir(void);
void bajar(void);
void izquierda(void);
void Derecha(void);
void diagI_sup(void);
void diagI_inf(void);
void diagD_sup(void);
void diagD_inf(void);
void X(void);
void Y(void);

void sumar_m(const uint8_t A[N][N], const uint8_t B[N][N], uint8_t R[N][N]); // A + B = R

void detectar_uno(void);

int main(void)
{
	
	UBRR0H = (BRC >> 8);
	UBRR0L = BRC;
	
	TCCR1A = 0x00;
	TCCR1B |=  (1 << CS02) | (0 << CS01) | (1 << CS00); //Timer 1024
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

    }
  
    
}



void sumar_m(const uint8_t A[N][N], const uint8_t B[N][N], uint8_t R[N][N]){ //Asignar const a A y B, asegura que no sean modificadas
	for(uint8_t i = 0 ; i < N ; i++ ){ // recorrer Filas
		for(uint8_t h = 0 ; h < N ; h++ ){ // recorrer Columnas
			R[i][h]= (uint8_t)(A[i][h] + B[i][h]); //especificar que se asigna el valor 
		}
	}
}



void apagar(void){
	cbi(PORTD, 4);
	cbi(PORTD, 5);
	cbi(PORTD, 6);
	cbi(PORTD, 2);
	sbi(PORTD, 3);
}

void Subir(void)
{
	cbi(PORTD, 4);   
	sbi(PORTD, 5);
}

void Bajar(void)
{
	cbi(PORTD, 5);   
	sbi(PORTD, 4);
}

void Izquierda(void)
{
	cbi(PORTD, 6);   
	sbi(PORTD, 7);
}

void Derecha(void)
{
	cbi(PORTD, 7);   
	sbi(PORTD, 6);
}

void bajar_s(void)
{
	sbi(PORTD, 2);   
	cbi(PORTD, 3);  
}

void Subir_s(void)
{
	sbi(PORTD, 3);  
	cbi(PORTD, 2);   
}


ISR(TIMER1_OVF_vect) {
	TCNT1H = 0xC2;
	TCNT1L = 0xF7;
	
	CONTADOR ++;
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
