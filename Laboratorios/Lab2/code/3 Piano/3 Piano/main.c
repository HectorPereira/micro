/*
Funcionalidades detalladas:

1. Reproducción de Notas:
	o	Cada uno de los 8 pulsadores está asignado a una nota musical
		específica (Do, Re, Mi, Fa, Sol, La, Si, Do agudo).
	o	Al presionar un pulsador, el buzzer reproduce la frecuencia
		correspondiente a la nota seleccionada.

2. Selección de Canciones Predefinidas:
	o	Mediante la comunicación UART, el usuario puede seleccionar
		entre dos canciones almacenadas en el microcontrolador.
	o	Cuando se recibe el comando UART adecuado, el
		microcontrolador detiene la funcionalidad del piano y reproduce la
		canción seleccionada de manera automática.

3. Control por UART:
	o	El sistema recibe comandos a través de UART para seleccionar
		una canción.
	o	Los comandos pueden ser simples, como 'C1' para la primera
		canción y 'C2' para la segunda.
	o	También se puede enviar un comando para detener la
		reproducción de la canción y volver al modo piano
 */ 


// ------------------------------------------------------------------
// LIBRARIES
// ------------------------------------------------------------------

#define F_CPU 16000000UL
#include <xc.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdbool.h>

// ------------------------------------------------------------------
// DEFINITIONS
// ------------------------------------------------------------------

#define G4 392	
#define Fb4 370	
#define E4 330	
#define D4 294	
#define A3 220	
#define Cb4 277	
#define F4 349	
#define C4 262	
#define Ab3 233	
#define A4 440	
#define Ab4 466	
#define B4 494	
#define A2 110	
#define D3 147	
#define Fb3 185	
#define B2 123	
#define E3 165	
#define G3 196	
#define Cb3 139	
#define Ab2 117	
#define F3 175	
#define C3 131	
#define G2 98	

#define UART_BAUD     9600UL
#define UBRR_VAL      ((F_CPU/16/UART_BAUD) - 1)

#define TX_BUF_SZ   128u
#define TX_MASK     (TX_BUF_SZ - 1)

#define RX_BUF_SZ	128u
#define RX_MASK		(RX_BUF_SZ - 1)


// ------------------------------------------------------------------
// SRAM VARIABLES
// ------------------------------------------------------------------

static volatile uint8_t rx_buf[RX_BUF_SZ];
static volatile uint8_t rx_head = 0;     // next write by ISR
static volatile uint8_t rx_tail = 0;     // next read by main
static volatile uint16_t rx_overruns = 0;
static volatile uint16_t rx_errors   = 0;

static volatile uint8_t tx_buf[TX_BUF_SZ];
static volatile uint8_t tx_head = 0;   // next write
static volatile uint8_t tx_tail = 0;   // next read

uint8_t eventAon = 0; // Encender track A
uint8_t eventAoff = 0; // Apagar track A
uint16_t indexA = 0; // Posicion track A
uint16_t countA = 0; // Conteo de overflow de notas de A
uint16_t maxCountAon = 0; // Maximo conteo de overflow encendido en A
uint16_t maxCountAoff = 0; // Maximo conteo de overflow apagado en A
uint8_t enableCountAon = 0; // Habilitar conteo de encendido en A
uint8_t enableCountAoff = 0; // Habilidad conteo de apagado en A

uint8_t eventBon = 0; // Encender track B
uint8_t eventBoff = 0; // Apagar track B
uint16_t indexB = 0; // Posicion track B
uint16_t countB = 0; // Conteo de overflow de notas de B
uint16_t maxCountBon = 0; // Maximo conteo de overflow encendido en B
uint16_t maxCountBoff = 0; // Maximo conteo de overflow apagado en B
uint8_t enableCountBon = 0; // Habilitar conteo encendido en B
uint8_t enableCountBoff = 0; // Habilitar conteo apagado en B



// ------------------------------------------------------------------
// PROGRAM MEMORY
// ------------------------------------------------------------------


// Midi tracks
// Generated using https://github.com/ShivamJoker/MIDI-to-Arduino
const int midiA[349][3] PROGMEM = {
	{G4, 252, 0},
	{Fb4, 252, 0},
	{E4, 252, 0},
	{E4, 252, 0},
	{Fb4, 1008, 1765},
	{A3, 252, 0},
	{G4, 252, 0},
	{Fb4, 252, 0},
	{E4, 252, 0},
	{E4, 504, 0},
	{Fb4, 756, 0},
	{D4, 504, 0},
	{E4, 252, 0},
	{A3, 751, 1266},
	{A3, 252, 0},
	{E4, 504, 0},
	{Fb4, 252, 0},
	{G4, 756, 0},
	{E4, 252, 0},
	{Cb4, 504, 0},
	{D4, 756, 0},
	{E4, 504, 0},
	{A3, 252, 0},
	{A3, 504, 0},
	{Fb4, 762, 2012},
	{G4, 252, 0},
	{Fb4, 252, 0},
	{E4, 252, 0},
	{E4, 252, 0},
	{Fb4, 1008, 1765},
	{A3, 252, 0},
	{G4, 252, 0},
	{Fb4, 252, 0},
	{E4, 252, 0},
	{E4, 756, 0},
	{Fb4, 252, 0},
	{D4, 756, 0},
	{E4, 252, 0},
	{A3, 756, 1513},
	{E4, 504, 0},
	{Fb4, 252, 0},
	{G4, 756, 0},
	{E4, 252, 0},
	{Cb4, 756, 0},
	{D4, 252, 0},
	{E4, 504, 0},
	{A3, 252, 0},
	{D4, 252, 0},
	{E4, 252, 0},
	{F4, 252, 0},
	{E4, 252, 0},
	{D4, 252, 0},
	{C4, 252, 504},
	{A3, 252, 0},
	{Ab3, 252, 0},
	{C4, 504, 0},
	{F4, 504, 0},
	{E4, 252, 0},
	{D4, 252, 0},
	{D4, 252, 0},
	{C4, 252, 0},
	{D4, 252, 0},
	{C4, 252, 0},
	{C4, 504, 0},
	{C4, 504, 0},
	{A3, 252, 0},
	{Ab3, 252, 0},
	{C4, 504, 0},
	{F4, 504, 0},
	{G4, 252, 0},
	{F4, 252, 0},
	{E4, 252, 0},
	{D4, 252, 0},
	{D4, 252, 0},
	{E4, 252, 0},
	{F4, 504, 0},
	{F4, 504, 0},
	{G4, 252, 0},
	{A4, 252, 0},
	{Ab4, 252, 0},
	{Ab4, 252, 0},
	{A4, 504, 0},
	{G4, 504, 0},
	{F4, 252, 0},
	{G4, 252, 0},
	{A4, 252, 0},
	{A4, 252, 0},
	{G4, 504, 0},
	{F4, 504, 0},
	{D4, 252, 0},
	{C4, 252, 0},
	{D4, 252, 0},
	{F4, 252, 0},
	{F4, 252, 0},
	{E4, 504, 0},
	{E4, 252, 0},
	{Fb4, 252, 0},
	{Fb4, 252, 6807},
	{A3, 252, 0},
	{G4, 252, 0},
	{Fb4, 252, 0},
	{E4, 252, 0},
	{E4, 378, 0},
	{Fb4, 882, 2017},
	{G4, 252, 0},
	{Fb4, 252, 0},
	{E4, 252, 0},
	{E4, 756, 0},
	{Fb4, 252, 0},
	{D4, 504, 0},
	{E4, 504, 0},
	{A3, 756, 1513},
	{E4, 504, 0},
	{Fb4, 252, 0},
	{G4, 756, 0},
	{E4, 504, 0},
	{Cb4, 504, 0},
	{D4, 252, 0},
	{E4, 756, 0},
	{A3, 252, 0},
	{A3, 504, 0},
	{Fb4, 747, 1774},
	{A3, 252, 0},
	{G4, 252, 0},
	{Fb4, 252, 0},
	{E4, 252, 0},
	{E4, 252, 0},
	{Fb4, 504, 2269},
	{A3, 252, 0},
	{G4, 252, 0},
	{Fb4, 252, 0},
	{E4, 252, 0},
	{E4, 756, 0},
	{Fb4, 252, 0},
	{D4, 756, 0},
	{E4, 252, 0},
	{A3, 756, 1513},
	{E4, 504, 0},
	{Fb4, 252, 0},
	{G4, 756, 0},
	{E4, 504, 0},
	{Cb4, 504, 0},
	{D4, 252, 0},
	{E4, 504, 0},
	{A3, 252, 0},
	{D4, 252, 0},
	{E4, 252, 0},
	{F4, 252, 0},
	{E4, 252, 0},
	{D4, 252, 0},
	{C4, 756, 0},
	{A3, 252, 0},
	{Ab3, 252, 0},
	{C4, 504, 0},
	{F4, 504, 0},
	{E4, 252, 0},
	{D4, 252, 0},
	{D4, 252, 0},
	{C4, 252, 0},
	{D4, 252, 0},
	{C4, 252, 0},
	{C4, 504, 0},
	{C4, 504, 0},
	{A3, 252, 0},
	{Ab3, 252, 0},
	{C4, 504, 0},
	{F4, 504, 0},
	{G4, 252, 0},
	{F4, 252, 0},
	{E4, 252, 0},
	{D4, 252, 0},
	{D4, 252, 0},
	{E4, 252, 0},
	{F4, 504, 0},
	{F4, 504, 0},
	{G4, 252, 0},
	{A4, 252, 0},
	{Ab4, 252, 0},
	{Ab4, 252, 0},
	{A4, 504, 0},
	{G4, 504, 0},
	{F4, 252, 0},
	{G4, 252, 0},
	{A4, 252, 0},
	{A4, 252, 0},
	{G4, 252, 0},
	{F4, 252, 0},
	{F4, 504, 0},
	{D4, 252, 0},
	{C4, 252, 0},
	{D4, 252, 0},
	{F4, 252, 0},
	{F4, 252, 0},
	{E4, 504, 0},
	{E4, 252, 0},
	{Fb4, 252, 0},
	{Fb4, 760, 6551},
	{G4, 252, 0},
	{Fb4, 252, 0},
	{E4, 252, 0},
	{E4, 504, 0},
	{Fb4, 756, 1765},
	{A3, 252, 0},
	{G4, 252, 0},
	{Fb4, 252, 0},
	{E4, 252, 0},
	{E4, 756, 0},
	{Fb4, 252, 0},
	{D4, 756, 0},
	{E4, 252, 0},
	{A3, 747, 1522},
	{E4, 504, 0},
	{Fb4, 252, 0},
	{G4, 756, 0},
	{E4, 504, 0},
	{Cb4, 504, 0},
	{D4, 252, 0},
	{E4, 504, 252},
	{A3, 252, 0},
	{A3, 504, 0},
	{Fb4, 772, 2006},
	{G4, 252, 0},
	{Fb4, 252, 0},
	{E4, 252, 0},
	{Fb4, 504, 2274},
	{G4, 252, 0},
	{Fb4, 252, 0},
	{E4, 252, 0},
	{E4, 756, 0},
	{Fb4, 252, 0},
	{D4, 756, 0},
	{E4, 252, 0},
	{A3, 756, 1513},
	{E4, 504, 0},
	{Fb4, 252, 0},
	{G4, 756, 0},
	{E4, 504, 0},
	{Cb4, 504, 0},
	{D4, 252, 0},
	{E4, 504, 0},
	{A3, 252, 0},
	{D4, 252, 0},
	{E4, 252, 0},
	{F4, 252, 0},
	{E4, 252, 0},
	{D4, 252, 0},
	{C4, 756, 0},
	{A3, 252, 0},
	{Ab3, 252, 0},
	{C4, 504, 0},
	{F4, 504, 0},
	{E4, 252, 0},
	{D4, 252, 0},
	{D4, 252, 0},
	{C4, 252, 0},
	{D4, 252, 0},
	{C4, 252, 0},
	{C4, 504, 0},
	{C4, 504, 0},
	{A3, 252, 0},
	{Ab3, 252, 0},
	{C4, 504, 0},
	{F4, 504, 0},
	{G4, 252, 0},
	{F4, 252, 0},
	{E4, 252, 0},
	{D4, 252, 0},
	{D4, 252, 0},
	{E4, 252, 0},
	{F4, 504, 0},
	{F4, 504, 0},
	{G4, 252, 0},
	{A4, 252, 0},
	{Ab4, 252, 0},
	{Ab4, 252, 0},
	{G4, 252, 5},
	{G4, 504, 0},
	{F4, 252, 0},
	{G4, 252, 0},
	{A4, 252, 0},
	{A4, 252, 0},
	{G4, 252, 0},
	{F4, 252, 0},
	{F4, 504, 0},
	{D4, 252, 0},
	{C4, 252, 0},
	{D4, 252, 0},
	{F4, 252, 0},
	{F4, 252, 0},
	{E4, 504, 0},
	{E4, 252, 0},
	{Fb4, 252, 0},
	{Fb4, 760, 999},
	{A4, 252, 0},
	{A4, 252, 0},
	{B4, 252, 0},
	{A4, 252, 0},
	{Fb4, 252, 0},
	{D4, 504, 0},
	{E4, 252, 0},
	{Fb4, 252, 0},
	{A4, 756, 756},
	{A4, 252, 0},
	{A4, 252, 0},
	{A4, 252, 0},
	{B4, 252, 0},
	{A4, 252, 0},
	{Fb4, 252, 0},
	{D4, 504, 0},
	{E4, 252, 0},
	{Fb4, 252, 0},
	{Fb4, 756, 756},
	{A4, 252, 0},
	{A4, 252, 0},
	{A4, 252, 0},
	{B4, 252, 0},
	{A4, 252, 0},
	{Fb4, 252, 0},
	{D4, 504, 0},
	{E4, 252, 0},
	{Fb4, 252, 0},
	{Fb4, 756, 1008},
	{A4, 252, 0},
	{A4, 252, 0},
	{B4, 252, 0},
	{A4, 252, 0},
	{Fb4, 252, 0},
	{D4, 504, 0},
	{E4, 252, 0},
	{Fb4, 252, 0},
	{Fb4, 756, 756},
	{A4, 252, 0},
	{A4, 252, 0},
	{A4, 252, 0},
	{B4, 252, 0},
	{A4, 252, 0},
	{Fb4, 252, 0},
	{D4, 504, 0},
	{E4, 252, 0},
	{Fb4, 252, 0},
	{Fb4, 756, 756},
	{G4, 252, 0},
	{A4, 252, 0},
	{A4, 756, 756},
	{G4, 252, 0},
	{Fb4, 252, 0},
	{Fb4, 785, 0},
};
const int midiB[433][3] PROGMEM = {
	{F3, 1, 1007},
	{A2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{B2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{A2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{B2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{A2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{B2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{A2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{B2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{B2, 252, 0},
	{E3, 252, 0},
	{G3, 252, 0},
	{E3, 252, 0},
	{B2, 252, 0},
	{E3, 252, 0},
	{G3, 252, 0},
	{E3, 252, 0},
	{A2, 252, 0},
	{E3, 252, 0},
	{G3, 252, 0},
	{Cb3, 252, 0},
	{A2, 252, 0},
	{E3, 252, 0},
	{G3, 252, 0},
	{Cb3, 252, 0},
	{A2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{B2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{A2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{B2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{A2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{B2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{A2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{B2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{A2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{B2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{A2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{B2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{B2, 252, 0},
	{E3, 252, 0},
	{G3, 252, 0},
	{E3, 252, 0},
	{B2, 252, 0},
	{E3, 252, 0},
	{G3, 252, 0},
	{E3, 252, 0},
	{A2, 252, 0},
	{E3, 252, 0},
	{G3, 252, 0},
	{Cb3, 252, 0},
	{A2, 252, 0},
	{E3, 252, 0},
	{G3, 252, 0},
	{Cb3, 252, 0},
	{Ab2, 252, 0},
	{D3, 252, 0},
	{F3, 252, 15378},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{E3, 756, 0},
	{E3, 252, 0},
	{E3, 252, 0},
	{Fb3, 252, 0},
	{G3, 504, 0},
	{A2, 756, 0},
	{A2, 252, 0},
	{A2, 252, 0},
	{B2, 252, 0},
	{Cb3, 504, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{E3, 756, 0},
	{E3, 252, 0},
	{E3, 252, 0},
	{Fb3, 252, 0},
	{G3, 504, 0},
	{A2, 756, 0},
	{A2, 252, 0},
	{A2, 252, 0},
	{B2, 252, 0},
	{Cb3, 252, 0},
	{A2, 252, 0},
	{Ab2, 504, 0},
	{Ab2, 252, 0},
	{Ab2, 252, 0},
	{Ab2, 252, 0},
	{Ab2, 252, 0},
	{Ab2, 252, 0},
	{Ab2, 252, 0},
	{F3, 252, 0},
	{F3, 252, 504},
	{C3, 252, 0},
	{C3, 252, 504},
	{Ab2, 252, 0},
	{Ab2, 252, 504},
	{F3, 252, 0},
	{F3, 252, 504},
	{F3, 252, 0},
	{F3, 252, 504},
	{C3, 252, 0},
	{C3, 252, 504},
	{Ab2, 252, 0},
	{Ab2, 252, 504},
	{F3, 252, 0},
	{F3, 252, 504},
	{Ab2, 252, 0},
	{Ab2, 252, 0},
	{Ab2, 252, 0},
	{Ab2, 252, 0},
	{C3, 252, 0},
	{C3, 252, 0},
	{C3, 252, 0},
	{C3, 252, 0},
	{F3, 252, 0},
	{F3, 252, 0},
	{E3, 252, 0},
	{E3, 252, 0},
	{D3, 252, 0},
	{D3, 252, 0},
	{C3, 252, 0},
	{C3, 252, 0},
	{Ab2, 504, 0},
	{F3, 504, 0},
	{A2, 504, 0},
	{E3, 504, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{A2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{B2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{A2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{B2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{A2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{B2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{A2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{B2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{B2, 252, 0},
	{E3, 252, 0},
	{G3, 252, 0},
	{E3, 252, 0},
	{B2, 252, 0},
	{E3, 252, 0},
	{G3, 252, 0},
	{E3, 252, 0},
	{A2, 252, 0},
	{E3, 252, 0},
	{G3, 252, 0},
	{Cb3, 252, 0},
	{A2, 252, 0},
	{E3, 252, 0},
	{G3, 252, 0},
	{Cb3, 252, 0},
	{A2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{B2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{A2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{B2, 252, 0},
	{D3, 252, 0},
	{Fb3, 252, 0},
	{D3, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{E3, 756, 0},
	{E3, 252, 0},
	{E3, 252, 0},
	{Fb3, 252, 0},
	{G3, 504, 0},
	{A2, 756, 0},
	{A2, 252, 0},
	{A2, 252, 0},
	{B2, 252, 0},
	{Cb3, 252, 0},
	{A2, 252, 0},
	{Ab2, 504, 0},
	{Ab2, 252, 0},
	{Ab2, 252, 0},
	{Ab2, 252, 0},
	{Ab2, 252, 0},
	{Ab2, 252, 0},
	{Ab2, 252, 0},
	{F3, 252, 0},
	{F3, 252, 504},
	{C3, 252, 0},
	{C3, 252, 504},
	{Ab2, 252, 0},
	{Ab2, 252, 504},
	{F3, 252, 0},
	{F3, 252, 504},
	{F3, 252, 0},
	{F3, 252, 504},
	{C3, 252, 0},
	{C3, 252, 504},
	{Ab2, 252, 0},
	{Ab2, 252, 504},
	{F3, 252, 0},
	{F3, 252, 504},
	{Ab2, 252, 0},
	{Ab2, 252, 0},
	{Ab2, 252, 0},
	{Ab2, 252, 0},
	{C3, 252, 0},
	{C3, 252, 0},
	{C3, 252, 0},
	{C3, 252, 0},
	{F3, 252, 0},
	{F3, 252, 0},
	{E3, 252, 0},
	{E3, 252, 0},
	{D3, 252, 0},
	{D3, 252, 0},
	{C3, 252, 0},
	{C3, 252, 0},
	{Ab2, 504, 0},
	{F3, 504, 0},
	{A2, 504, 0},
	{E3, 504, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 756, 0},
	{B2, 252, 0},
	{D3, 756, 0},
	{D3, 252, 0},
	{B2, 504, 0},
};


// ------------------------------------------------------------------
// HELPERS
// ------------------------------------------------------------------


void read_midi_event(const int (*track)[3], uint16_t index, int out_values[3]) {
	for (uint8_t i = 0; i < 3; i++) {
		out_values[i] = pgm_read_word(&(track[index][i]));
	}
}

static bool uart_putc(uint8_t c) {
	uint8_t head = tx_head;
	uint8_t next = (uint8_t)((head + 1) & TX_MASK);

	// Full if advancing head hits tail
	if (next == tx_tail) return false;

	tx_buf[head] = c;
	tx_head = next;

	// Kick the transmitter: enable Data Register Empty interrupt
	UCSR0B |= (1 << UDRIE0);
	return true;
}



// ------------------------------------------------------------------
// INITIALIZERS
// ------------------------------------------------------------------


// Overflow = 1ms
void timer1_init(void) {
	TCCR1A = 0x00;
	TCCR1B = (1 << CS11) | (1 << CS10);  // 64
	TCNT1 = 65536 - 250;
	TIMSK1 |= (1 << TOIE1);
}

void uart_init(void) {
	// Baud
	UBRR0H = (uint8_t)(UBRR_VAL >> 8);
	UBRR0L = (uint8_t)(UBRR_VAL & 0xFF);

	// 8N1
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

	// Enable TX (RX optional) — leave UDRE interrupt OFF for now
	UCSR0B = (1 << TXEN0);  // | (1<<RXEN0) if you also receive
}

void uart_init_rx(void) {
	// Baud
	UBRR0H = (uint8_t)(UBRR_VAL >> 8);
	UBRR0L = (uint8_t)(UBRR_VAL & 0xFF);

	// 8N1
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

	// Enable RX and its interrupt (TX config can be elsewhere)
	UCSR0B |= (1 << RXEN0) | (1 << RXCIE0);
}


// ------------------------------------------------------------------
// UTILITY
// ------------------------------------------------------------------

/* ---- Non-blocking: copy up to maxlen bytes into dst; returns count ---- */
uint8_t uart_read_bytes(uint8_t *dst, uint8_t maxlen) {
	uint8_t n = 0;
	while ((n < maxlen) && (rx_head != rx_tail)) {
		dst[n++] = rx_buf[rx_tail];
		rx_tail  = (uint8_t)((rx_tail + 1) & RX_MASK);
	}
	return n;  // 0 means no data right now
}

/* ---- Non-blocking: returns how many bytes are currently buffered ---- */
uint8_t uart_rx_available(void) {
	uint8_t head = rx_head;   // snapshot (atomic on AVR)
	uint8_t tail = rx_tail;
	return (uint8_t)((head - tail) & RX_MASK);
}


int uart_write_str(const char *s) {
	int n = 0;
	while (*s) {
		if (!uart_putc((uint8_t)*s)) break;  // stop if buffer full
		++s; ++n;
	}
	return n;   // if this is < strlen(s), buffer filled; call again later
}


void playA(){
	int values[3];
	read_midi_event(midiA, indexA++, values);
	uint16_t freq = values[0];
	maxCountAon = values[1];
	maxCountAoff = values[2];
	
	if (!freq) return;
	DDRD |= (1 << PORTD6);

	uint8_t presc_bits = 0b0; // Valor por defecto
	uint16_t ocr;

	// Probar todos los prescalers
	const uint16_t presc_list[] = {8, 64, 256, 1024};
	const uint8_t  bits_list[]  = { 0b010, 0b011, 0b100, 0b101 };

	for (uint8_t i=0;i<4;i++) {
		ocr = (F_CPU / (2UL * presc_list[i] * freq)) - 1;
		if (ocr <= 255) { presc_bits = bits_list[i]; break; }
	}

	TCCR0A = (1 << COM0A0) | (1 << WGM01);
	TCCR0B = presc_bits;        // prescaler elegido
	OCR0A  = (uint8_t)ocr;
	
	enableCountAon = 1; // Start playing
}

void stopA(void){
	TCCR0B = 0b0;
	enableCountAoff = 1;
}


void playB(){
	int values[3];
	read_midi_event(midiB, indexB++, values);
	uint16_t freq = values[0];
	maxCountBon = values[1];
	maxCountBoff = values[2];
	
	if (!freq) return;
	DDRB |= (1 << PORTB3);

	uint8_t presc_bits = 0b0; // Valor por defecto
	uint16_t ocr;

	// Probar todos los prescalers
	const uint16_t presc_list[] = {8, 32, 64, 128, 256, 1024};
	const uint8_t  bits_list[]  = {0b010, 0b011, 0b100, 0b101, 0b110, 0b111 };

	for (uint8_t i=0;i<6;i++) {
		ocr = (F_CPU / (2UL * presc_list[i] * freq)) - 1;
		if (ocr <= 255) { presc_bits = bits_list[i]; break; }
	}

	TCCR2A = (1 << COM2A0) | (1 << WGM21);
	TCCR2B = presc_bits;        // prescaler elegido
	OCR2A  = (uint8_t)ocr;
	
	enableCountBon = 1; // Empezar a tocar
}

void stopB(void){
	TCCR2B = 0b0;
	enableCountBoff = 1;
}


// ------------------------------------------------------------------
// WEIRD SHIT
// ------------------------------------------------------------------

typedef enum {
    CMD_NONE = 0,
    CMD_C1,
    CMD_C2,
    CMD_PIANO,
    CMD_UNKNOWN
} cmd_t;

/* Small token buffer (adjust as needed) */
#define CMD_BUF_SZ 16

/* Helper: ASCII upper-case without locale */
static inline char upc(char c) {
    return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
}

/* Return true if c is a delimiter that ends a token */
static inline bool is_delim(char c) {
    return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n') || (c == ',');
}

/* Compare token (already uppercased) to a literal (uppercased here) */
static bool token_eq(const char *tok, uint8_t len, const char *lit) {
    for (uint8_t i = 0; i < len; ++i) {
        if (lit[i] == '\0' || tok[i] != lit[i]) return false;
    }
    return lit[len] == '\0';
}

/* Non-blocking parser:
 * - Feed it repeatedly (e.g., every loop).
 * - Returns one command when a token is completed; otherwise CMD_NONE.
 * - Unrecognized tokens yield CMD_UNKNOWN (so you can notify or ignore).
 */
cmd_t uart_read_command(void) {
    static char    buf[CMD_BUF_SZ];
    static uint8_t len = 0;

    uint8_t tmp[32];
    uint8_t n = uart_read_bytes(tmp, sizeof(tmp));
    for (uint8_t i = 0; i < n; ++i) {
        char c = (char)tmp[i];

        if (is_delim(c)) {
            if (len == 0) continue;          // skip repeated delimiters
            // We have a complete token in buf[0..len-1]
            // Match against commands
            if (token_eq(buf, len, "C1"))    { len = 0; return CMD_C1; }
            if (token_eq(buf, len, "C2"))    { len = 0; return CMD_C2; }
            if (token_eq(buf, len, "PIANO")) { len = 0; return CMD_PIANO; }
            len = 0;
            return CMD_UNKNOWN;
        } else {
            // Accumulate (uppercase); drop overflow safely
            if (len < (CMD_BUF_SZ - 1)) {
                buf[len++] = upc(c);
                buf[len]   = '\0';
            }
            // If overflow, keep reading until a delimiter resets len
        }
    }

    // No completed token this call
    return CMD_NONE;
}


// ------------------------------------------------------------------
// MAIN LOOP
// ------------------------------------------------------------------



int main(void) {
	uart_init();
	uart_init_rx();
	timer1_init();
	sei();
	
	uart_write_str("Hola PIC 2\n");
	//uart_write_str("[C1] \r\n");
	//uart_write_str("[C2] \r\n");
	//uart_write_str("[PIANO] \r\n");
	
	eventAoff = 1;
	eventBoff = 1;
	
	while (1){
		/*
		  cmd_t cmd = uart_read_command();
		  switch (cmd) {
			  case CMD_C1:
			  uart_write_str("[C1!!] \r\n");
			  break;
			  case CMD_C2:
			  uart_write_str("[C2!!] \r\n");
			  break;
			  case CMD_PIANO:
			  uart_write_str("[PIANO!!] \r\n");
			  break;
			  case CMD_UNKNOWN:
			  uart_write_str("[NOENTIENDO!!] \r\n");
			  break;
			  case CMD_NONE:
			  uart_write_str("[NOENTIENDO!!NONE] \r\n");
			  break;
			  
			  default:
			  uart_write_str("[NOENTIENDO!!DEFAULT] \r\n");
			  break;
		  }
		  */
		
		if (eventAon){
			countA = 0;
			enableCountAon = 0;
			eventAon = 0;
			stopA();
		} if (eventAoff){
			countA = 0;
			enableCountAoff = 0;
			eventAoff = 0;
			playA();
		}
		if (eventBon) {
			countB = 0;
			enableCountBon = 0;
			eventBon = 0;
			stopB();
		} if (eventBoff){
			countB = 0;
			enableCountBoff = 0;
			eventBoff = 0;
			playB();
						
		}
	}
}


// ------------------------------------------------------------------
// INTERRUPT SERVICE ROUTINES	
// ------------------------------------------------------------------


ISR(TIMER1_OVF_vect){
	TCNT1 = 65536 - 250;  // 1ms preload
	
	if (enableCountAon) {
		if (++countA > maxCountAon) {
			eventAon = 1;
		}
		
		} else if (enableCountAoff){
		if (++countA > maxCountAoff){
			eventAoff = 1;
		}
	}
	
	if (enableCountBon) {
		if (++countB > maxCountBon) {
			eventBon = 1;
		}
		
		} else if (enableCountBoff){
		if (++countB > maxCountBoff){
			eventBoff = 1;
		}
	}
}

ISR(USART_UDRE_vect) {
	if (tx_head == tx_tail) {
		// Nothing to send: disable UDRE interrupt to stop firing
		UCSR0B &= ~(1 << UDRIE0);
		return;
	}
	UDR0 = tx_buf[tx_tail];
	tx_tail = (uint8_t)((tx_tail + 1) & TX_MASK);
}

/* ---- ISR: push each received byte into the ring buffer ---- */
ISR(USART_RX_vect) {
	uint8_t status = UCSR0A;
	uint8_t data   = UDR0;  // reading UDR0 clears RXC

	// Basic error accounting (frame/parity/overrun)
	if (status & ((1 << FE0) | (1 << DOR0) | (1 << UPE0))) {
		rx_errors++;
		// Still store data to keep stream aligned; comment out to drop on error
	}

	uint8_t next = (uint8_t)((rx_head + 1) & RX_MASK);
	if (next == rx_tail) {
		// Buffer full: drop the byte (or overwrite by advancing tail)
		rx_overruns++;
		// Optionally: rx_tail = (uint8_t)((rx_tail + 1) & RX_MASK); // overwrite oldest
		return;
	}

	rx_buf[rx_head] = data;
	rx_head = next;
}
