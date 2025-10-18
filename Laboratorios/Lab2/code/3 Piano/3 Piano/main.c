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
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#ifndef _BV
#define _BV(bit) (1U << (bit))
#endif

// ------------------------------------------------------------------
// DEFINITIONS
// ------------------------------------------------------------------



// Midi A y B
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

// MIDI C
#define E5 659
#define B4 494
#define A4 440
#define E4 330
#define E3 165
#define B3 247
#define A3 220
#define D3 147
#define G5 784
#define Gb3 208
#define Fb3 185
#define G3 196
#define C4 262
#define D4 294
#define Fb4 370
#define G4 392
#define C5 523
#define D5 587
#define Fb5 740
#define B5 988
#define Db4 311
#define Db5 622
#define Cb4 277
#define Gb4 415
#define Gb5 831
#define Cb5 554
#define F4 349
#define Ab3 233
#define Ab4 466
#define F5 698
#define A5 880
#define Ab5 932
#define C6 1047
#define Db3 156
#define F3 175
#define A2 110
#define F2 87
#define G2 98
#define E2 82
#define D6 1175
#define E6 1319
#define C3 131

// Octavas principales
#define C4  262
#define D4  294
#define E4  330
#define F4  349
#define G4  392
#define A4  440
#define B4  494
#define C5  523


#define TX_BUF_SZ 128
#define TX_MASK   (TX_BUF_SZ - 1)

#define RX_BUF_SZ 128
#define RX_MASK   (RX_BUF_SZ - 1)

// ------------------------------------------------------------------
// SRAM VARIABLES
// ------------------------------------------------------------------

// USART ---------------------------------------

uint8_t tx_buf[TX_BUF_SZ];
uint8_t tx_head = 0, tx_tail = 0;

uint8_t rx_buf[RX_BUF_SZ];
uint8_t rx_head = 0, rx_tail = 0;


// STATES ---------------------------------------
uint8_t mode = 0; // 0 = Piano, 1 = Song
uint8_t song = 0; // 0 = Doom, 1 = Still alive

// BUTTONS -------------------------------------
uint8_t prevC = 0b00111111;
uint8_t prevD = 0b00110000;

uint8_t debounce_ms = 0;
uint8_t debounce_active = 0;

const uint16_t NOTE_TABLE[8] = { C4, D4, E4, F4, G4, A4, B4, C5 };

// MIDI ---------------------------------------
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
const int midiC[827][3] PROGMEM = {
	{E5, 94, 0},
	{B4, 94, 0},
	{A4, 94, 0},
	{E4, 94, 0},
	{A4, 94, 0},
	{B4, 94, 0},
	{E5, 94, 0},
	{B4, 94, 0},
	{A4, 94, 0},
	{E4, 94, 0},
	{A4, 94, 0},
	{B4, 94, 0},
	{E5, 94, 0},
	{B4, 94, 0},
	{A4, 94, 0},
	{E4, 94, 0},
	{E3, 188, 0},
	{E3, 188, 0},
	{A4, 188, 0},
	{E3, 188, 0},
	{Gb4, 188, 0},
	{E4, 188, 0},
	{E3, 188, 0},
	{A4, 188, 0},
	{E3, 188, 0},
	{Gb4, 188, 0},
	{E3, 188, 0},
	{E3, 188, 0},
	{E3, 188, 0},
	{Gb4, 188, 0},
	{E3, 188, 0},
	{Gb4, 375, 0},
	{Fb4, 188, 0},
	{Fb4, 375, 0},
	{Fb4, 1500, 750},
	{E3, 188, 0},
	{E3, 188, 0},
	{A4, 188, 0},
	{E3, 188, 0},
	{Gb4, 188, 0},
	{E4, 188, 0},
	{E3, 188, 0},
	{A4, 188, 0},
	{E3, 188, 0},
	{Gb4, 188, 0},
	{E3, 188, 0},
	{E3, 188, 0},
	{E3, 188, 0},
	{Gb4, 188, 0},
	{E3, 188, 0},
	{Gb4, 375, 0},
	{Fb4, 188, 0},
	{Fb4, 375, 0},
	{Fb4, 1500, 0},
	{D3, 188, 0},
	{Fb4, 188, 0},
	{D3, 188, 0},
	{Fb4, 188, 327},
	{D3, 188, 0},
	{D3, 188, 0},
	{E3, 188, 0},
	{E3, 188, 0},
	{Gb3, 188, 0},
	{E3, 188, 0},
	{D3, 188, 0},
	{B3, 375, 0},
	{B3, 188, 0},
	{A3, 375, 0},
	{Gb3, 188, 0},
	{B3, 375, 0},
	{B3, 188, 0},
	{B3, 188, 0},
	{A3, 188, 0},
	{A3, 188, 0},
	{E4, 938, 0},
	{B3, 188, 0},
	{A3, 188, 0},
	{A3, 188, 0},
	{E4, 375, 0},
	{E3, 188, 0},
	{Fb3, 188, 0},
	{G3, 938, 0},
	{A3, 750, 0},
	{B3, 563, 0},
	{C4, 750, 0},
	{D4, 375, 0},
	{D4, 188, 0},
	{C4, 375, 0},
	{B3, 188, 0},
	{A3, 188, 0},
	{G3, 375, 0},
	{B4, 93, 0},
	{Fb4, 95, 0},
	{E4, 94, 0},
	{B3, 94, 0},
	{E4, 93, 0},
	{Fb4, 95, 0},
	{B4, 94, 0},
	{Fb4, 94, 0},
	{E4, 93, 0},
	{B3, 95, 0},
	{E4, 94, 0},
	{Fb4, 94, 0},
	{B4, 93, 0},
	{Fb4, 95, 0},
	{E4, 94, 0},
	{B3, 94, 0},
	{D3, 188, 0},
	{D3, 188, 0},
	{E3, 188, 0},
	{E3, 188, 0},
	{Gb3, 188, 0},
	{E3, 188, 0},
	{D3, 188, 0},
	{B3, 375, 0},
	{B3, 188, 0},
	{A3, 375, 0},
	{Gb3, 188, 0},
	{B3, 375, 0},
	{B3, 188, 0},
	{B3, 188, 0},
	{A3, 188, 0},
	{A3, 188, 0},
	{E4, 938, 0},
	{B4, 188, 0},
	{A4, 188, 0},
	{A4, 188, 0},
	{E5, 375, 0},
	{E4, 188, 0},
	{Fb4, 188, 0},
	{G4, 938, 0},
	{A4, 750, 0},
	{B4, 375, 0},
	{C5, 375, 0},
	{D5, 375, 0},
	{C5, 188, 0},
	{C5, 188, 0},
	{Fb4, 93, 0},
	{B4, 95, 0},
	{Fb5, 94, 0},
	{Fb5, 95, 0},
	{B5, 94, 0},
	{Fb5, 94, 0},
	{E5, 93, 0},
	{Fb5, 95, 0},
	{E5, 94, 0},
	{B4, 94, 0},
	{Fb4, 93, 0},
	{Db4, 95, 0},
	{B3, 94, 0},
	{Db4, 94, 0},
	{Fb4, 93, 0},
	{B4, 95, 0},
	{Db5, 94, 0},
	{Fb5, 94, 0},
	{Db5, 93, 0},
	{Fb5, 95, 0},
	{B5, 94, 0},
	{Fb5, 94, 0},
	{Db5, 93, 0},
	{Fb5, 95, 0},
	{Db5, 94, 0},
	{B4, 94, 0},
	{Fb4, 93, 0},
	{Db4, 95, 0},
	{B3, 188, 0},
	{Cb4, 938, 0},
	{Gb3, 188, 0},
	{Gb3, 188, 0},
	{Cb4, 188, 0},
	{B3, 375, 0},
	{A3, 375, 0},
	{Gb3, 375, 0},
	{Fb3, 375, 0},
	{A3, 1125, 0},
	{Gb3, 188, 0},
	{Gb3, 563, 0},
	{E5, 93, 0},
	{B4, 95, 0},
	{Gb4, 94, 0},
	{E4, 94, 0},
	{Gb5, 93, 0},
	{E5, 95, 0},
	{B4, 94, 0},
	{Gb4, 94, 0},
	{B5, 93, 0},
	{Gb5, 95, 0},
	{E5, 94, 0},
	{B4, 94, 0},
	{Fb4, 375, 0},
	{E3, 375, 0},
	{A3, 188, 0},
	{Gb3, 188, 0},
	{E3, 188, 0},
	{A3, 375, 0},
	{Gb3, 188, 0},
	{E3, 188, 0},
	{B3, 375, 0},
	{A3, 750, 188},
	{E4, 375, 0},
	{A4, 188, 0},
	{Gb4, 188, 0},
	{E4, 188, 0},
	{A4, 375, 0},
	{Gb4, 188, 188},
	{E4, 188, 0},
	{A4, 375, 0},
	{B4, 188, 0},
	{B4, 563, 0},
	{B3, 93, 0},
	{Cb4, 95, 0},
	{Db4, 94, 0},
	{E4, 94, 0},
	{Fb4, 93, 0},
	{Gb4, 95, 0},
	{A4, 94, 0},
	{B4, 94, 0},
	{Cb5, 93, 0},
	{Db5, 95, 0},
	{E5, 188, 188},
	{Fb4, 188, 0},
	{A4, 188, 0},
	{E5, 188, 0},
	{B4, 188, 0},
	{D5, 188, 0},
	{Fb4, 188, 0},
	{A4, 188, 0},
	{D4, 563, 0},
	{E4, 750, 188},
	{D4, 188, 188},
	{D4, 188, 0},
	{E4, 938, 375},
	{G4, 375, 0},
	{G4, 188, 0},
	{E4, 188, 188},
	{Fb4, 188, 0},
	{Fb4, 188, 188},
	{Fb4, 188, 188},
	{E4, 188, 0},
	{E4, 188, 188},
	{D4, 188, 0},
	{G4, 375, 0},
	{G4, 188, 0},
	{Fb4, 188, 188},
	{D4, 188, 0},
	{E4, 188, 188},
	{E4, 563, 0},
	{D5, 750, 188},
	{Cb4, 188, 0},
	{Cb4, 188, 0},
	{A3, 188, 0},
	{E3, 375, 93},
	{D3, 95, 0},
	{E3, 94, 0},
	{Fb3, 94, 0},
	{G3, 93, 0},
	{A3, 95, 0},
	{B3, 94, 0},
	{Cb4, 94, 0},
	{D4, 563, 0},
	{E4, 750, 188},
	{D4, 188, 188},
	{D4, 188, 0},
	{E4, 938, 375},
	{G4, 375, 0},
	{G4, 188, 0},
	{E4, 188, 188},
	{Fb4, 188, 0},
	{Fb4, 188, 188},
	{Fb4, 375, 0},
	{E4, 188, 0},
	{E4, 188, 188},
	{D4, 188, 0},
	{D4, 188, 375},
	{B3, 188, 0},
	{Cb4, 188, 0},
	{D4, 188, 0},
	{F4, 375, 0},
	{F4, 375, 0},
	{F4, 375, 0},
	{F4, 375, 0},
	{E4, 93, 95},
	{D4, 94, 94},
	{B3, 563, 0},
	{G3, 188, 0},
	{D3, 93, 0},
	{E3, 95, 0},
	{Fb3, 94, 0},
	{G3, 94, 0},
	{A3, 93, 0},
	{B3, 95, 0},
	{Cb4, 750, 0},
	{E4, 94, 0},
	{Gb4, 94, 0},
	{A4, 93, 0},
	{Cb5, 95, 0},
	{E5, 94, 0},
	{Cb5, 94, 0},
	{A4, 93, 0},
	{E4, 95, 0},
	{Cb4, 94, 0},
	{A3, 94, 0},
	{Ab3, 93, 0},
	{D4, 95, 0},
	{F4, 94, 0},
	{A4, 94, 0},
	{Ab4, 93, 0},
	{D5, 95, 0},
	{F5, 94, 0},
	{A5, 94, 0},
	{Ab5, 93, 0},
	{A5, 95, 0},
	{F5, 94, 0},
	{D5, 94, 0},
	{Ab4, 93, 0},
	{A4, 95, 0},
	{C4, 94, 0},
	{E4, 94, 0},
	{G4, 93, 0},
	{B4, 95, 0},
	{C5, 94, 0},
	{E5, 94, 0},
	{G5, 93, 0},
	{B5, 95, 0},
	{C6, 94, 0},
	{B5, 94, 0},
	{G5, 93, 0},
	{E5, 95, 0},
	{C5, 94, 0},
	{B4, 94, 0},
	{G4, 375, 0},
	{Db3, 563, 0},
	{F3, 94, 0},
	{G3, 94, 0},
	{Ab3, 93, 0},
	{Db4, 95, 0},
	{F4, 94, 0},
	{G4, 94, 0},
	{Ab4, 93, 0},
	{Db5, 95, 0},
	{F5, 563, 0},
	{A5, 1125, 0},
	{A2, 2438, 0},
	{F2, 375, 0},
	{G2, 2625, 0},
	{E2, 375, 0},
	{F2, 1500, 0},
	{G2, 1125, 0},
	{E2, 563, 0},
	{D3, 188, 0},
	{D3, 188, 0},
	{E3, 188, 0},
	{E3, 188, 0},
	{Gb3, 188, 0},
	{E3, 188, 0},
	{D3, 188, 0},
	{B3, 375, 0},
	{B3, 188, 0},
	{A3, 375, 0},
	{Gb3, 188, 0},
	{B3, 375, 0},
	{B3, 188, 0},
	{B3, 188, 0},
	{A3, 188, 0},
	{A3, 188, 0},
	{E4, 938, 0},
	{B3, 188, 0},
	{A3, 188, 0},
	{A3, 188, 0},
	{E4, 375, 0},
	{E3, 188, 0},
	{Fb3, 188, 0},
	{G3, 938, 0},
	{A3, 750, 0},
	{B3, 563, 0},
	{D5, 375, 0},
	{D5, 188, 0},
	{C5, 188, 0},
	{C5, 188, 0},
	{B4, 188, 0},
	{A4, 188, 0},
	{G4, 188, 0},
	{B4, 188, 0},
	{Db4, 1125, 0},
	{B4, 93, 0},
	{D5, 95, 0},
	{E5, 94, 0},
	{G5, 94, 0},
	{A5, 563, 0},
	{C6, 563, 0},
	{D6, 375, 0},
	{D6, 563, 0},
	{Fb5, 375, 0},
	{Fb5, 188, 0},
	{D5, 375, 0},
	{C4, 94, 0},
	{D4, 95, 0},
	{E4, 94, 0},
	{Fb4, 93, 0},
	{G4, 94, 0},
	{A4, 95, 0},
	{C5, 94, 0},
	{D5, 93, 0},
	{E5, 94, 0},
	{Fb5, 95, 0},
	{G5, 94, 0},
	{A5, 93, 0},
	{D6, 94, 0},
	{A5, 95, 0},
	{Fb5, 94, 0},
	{D5, 93, 0},
	{A5, 94, 0},
	{Fb5, 95, 0},
	{D5, 94, 0},
	{A4, 93, 0},
	{Fb4, 94, 0},
	{D4, 95, 0},
	{A3, 562, 0},
	{D5, 375, 0},
	{E5, 94, 0},
	{D5, 95, 0},
	{B4, 94, 0},
	{A4, 93, 0},
	{G4, 94, 0},
	{A4, 95, 0},
	{Ab4, 94, 0},
	{B4, 93, 0},
	{Ab4, 94, 0},
	{A4, 95, 0},
	{G4, 94, 0},
	{E4, 93, 0},
	{A4, 94, 0},
	{G4, 95, 0},
	{E4, 94, 0},
	{D4, 93, 0},
	{B3, 94, 0},
	{Ab3, 95, 0},
	{A3, 94, 0},
	{G3, 93, 0},
	{E3, 94, 0},
	{G3, 95, 0},
	{A3, 94, 0},
	{Ab3, 93, 0},
	{B3, 94, 0},
	{D4, 95, 0},
	{E4, 94, 0},
	{G4, 93, 0},
	{Fb4, 1125, 0},
	{E4, 375, 0},
	{Db4, 750, 0},
	{Fb5, 750, 0},
	{Cb5, 938, 0},
	{Gb4, 187, 0},
	{Gb4, 188, 0},
	{Cb5, 187, 0},
	{B4, 375, 0},
	{A4, 375, 0},
	{Gb4, 375, 0},
	{Fb4, 375, 0},
	{A4, 1125, 0},
	{Gb4, 188, 0},
	{Gb4, 562, 0},
	{E6, 375, 0},
	{D3, 375, 0},
	{E3, 375, 0},
	{A3, 188, 0},
	{Gb3, 187, 0},
	{E3, 188, 0},
	{A3, 375, 0},
	{Gb3, 187, 0},
	{E3, 188, 0},
	{G5, 1312, 0},
	{E4, 375, 0},
	{A4, 188, 0},
	{Gb4, 187, 0},
	{E4, 188, 0},
	{A4, 750, 0},
	{Gb4, 187, 0},
	{A4, 188, 0},
	{B4, 375, 0},
	{B4, 562, 0},
	{B3, 95, 0},
	{Cb4, 94, 0},
	{Db4, 94, 0},
	{E4, 93, 0},
	{Fb4, 95, 0},
	{Gb4, 94, 0},
	{A4, 94, 0},
	{B4, 93, 0},
	{Cb5, 95, 0},
	{Db5, 94, 0},
	{E5, 187, 188},
	{Fb4, 187, 0},
	{A4, 188, 0},
	{E5, 187, 0},
	{B4, 188, 0},
	{D5, 187, 0},
	{Fb4, 188, 0},
	{A4, 187, 0},
	{D4, 563, 0},
	{E4, 750, 187},
	{D4, 188, 187},
	{D4, 188, 0},
	{E4, 937, 375},
	{G4, 375, 0},
	{G4, 188, 0},
	{E4, 375, 0},
	{Fb4, 187, 0},
	{Fb4, 375, 0},
	{Fb4, 375, 0},
	{E4, 188, 0},
	{D4, 47, 0},
	{E4, 47, 281},
	{D4, 94, 93},
	{D4, 845, 94},
	{G4, 375, 0},
	{G4, 187, 0},
	{Fb4, 188, 187},
	{D4, 188, 0},
	{E4, 187, 188},
	{E4, 1687, 0},
	{Cb4, 95, 0},
	{D4, 94, 0},
	{B3, 95, 0},
	{Cb4, 94, 0},
	{D4, 94, 0},
	{E4, 93, 0},
	{E4, 94, 0},
	{Fb4, 93, 0},
	{G4, 95, 0},
	{A4, 94, 0},
	{B4, 94, 0},
	{Cb5, 93, 0},
	{D5, 563, 0},
	{E5, 750, 187},
	{D5, 188, 187},
	{D5, 188, 0},
	{E5, 187, 0},
	{A5, 1125, 0},
	{A5, 375, 0},
	{A5, 563, 0},
	{G5, 187, 0},
	{Fb5, 563, 0},
	{D5, 187, 0},
	{A4, 188, 0},
	{G4, 187, 0},
	{Fb4, 375, 0},
	{G3, 95, 0},
	{A3, 94, 0},
	{B3, 94, 0},
	{Fb4, 93, 0},
	{G4, 95, 0},
	{A4, 94, 0},
	{D5, 94, 0},
	{Fb5, 93, 0},
	{G5, 95, 0},
	{Fb5, 94, 0},
	{D5, 94, 0},
	{A4, 93, 0},
	{G4, 95, 0},
	{Fb4, 94, 0},
	{G4, 94, 0},
	{A4, 93, 0},
	{E5, 1500, 0},
	{D4, 563, 0},
	{E4, 750, 187},
	{D4, 188, 187},
	{D4, 188, 0},
	{E4, 937, 375},
	{G4, 375, 188},
	{E4, 187, 188},
	{Fb4, 187, 0},
	{Fb4, 188, 187},
	{E4, 563, 0},
	{D4, 1312, 188},
	{G4, 375, 0},
	{G4, 187, 0},
	{Fb4, 188, 187},
	{D4, 188, 0},
	{E4, 187, 188},
	{E4, 1312, 188},
	{D3, 94, 0},
	{E3, 93, 0},
	{Fb3, 95, 0},
	{G3, 94, 0},
	{A3, 94, 0},
	{B3, 93, 0},
	{Cb4, 95, 0},
	{D4, 94, 0},
	{E4, 94, 0},
	{Fb4, 93, 0},
	{G4, 95, 0},
	{A4, 94, 0},
	{B4, 94, 0},
	{Cb5, 93, 0},
	{D5, 95, 0},
	{E5, 94, 0},
	{Fb5, 94, 0},
	{G5, 93, 0},
	{E5, 375, 0},
	{A5, 188, 0},
	{E5, 375, 0},
	{A5, 187, 0},
	{E5, 375, 0},
	{A5, 188, 0},
	{E5, 375, 0},
	{E5, 94, 0},
	{D5, 93, 0},
	{B4, 750, 0},
	{B3, 95, 0},
	{D4, 94, 0},
	{E4, 94, 0},
	{Fb4, 93, 0},
	{A4, 95, 0},
	{B4, 94, 0},
	{E4, 94, 0},
	{Fb4, 93, 0},
	{A4, 95, 0},
	{B4, 94, 0},
	{D5, 94, 0},
	{E5, 93, 0},
	{A4, 95, 0},
	{B4, 94, 0},
	{D5, 94, 0},
	{E5, 93, 0},
	{Fb5, 95, 0},
	{A5, 94, 0},
	{Fb5, 94, 0},
	{E5, 93, 0},
	{D5, 95, 0},
	{E5, 94, 0},
	{Fb5, 94, 0},
	{B5, 93, 0},
	{Fb5, 95, 0},
	{E5, 94, 0},
	{D5, 94, 0},
	{E5, 93, 0},
	{Fb5, 95, 0},
	{B5, 94, 0},
	{D6, 562, 0},
	{B5, 95, 0},
	{G5, 94, 0},
	{D5, 94, 0},
	{B4, 93, 0},
	{G5, 95, 0},
	{D5, 94, 0},
	{B4, 94, 0},
	{G4, 93, 0},
	{D5, 95, 0},
	{B4, 94, 0},
	{G4, 94, 0},
	{D4, 93, 0},
	{F5, 95, 0},
	{D5, 94, 0},
	{Ab4, 94, 0},
	{F4, 93, 0},
	{Ab4, 95, 0},
	{D5, 94, 0},
	{F5, 94, 0},
	{D5, 93, 0},
	{Ab5, 95, 0},
	{F5, 94, 0},
	{D5, 94, 0},
	{Ab4, 93, 0},
	{D5, 95, 0},
	{F5, 94, 0},
	{Ab5, 94, 0},
	{F5, 93, 0},
	{Ab5, 1500, 0},
	{C3, 1313, 0},
	{A2, 1687, 0},
	{G5, 1313, 0},
	{G5, 1312, 0},
	{D5, 95, 0},
	{A4, 94, 0},
	{G4, 94, 0},
	{D4, 93, 0},
	{G4, 95, 0},
	{A4, 94, 0},
	{D5, 94, 0},
	{A4, 93, 0},
	{G4, 95, 0},
	{D4, 94, 0},
	{G4, 94, 0},
	{A4, 93, 0},
	{D5, 95, 0},
	{A4, 94, 0},
	{G4, 94, 0},
	{D4, 93, 0},
	{D4, 1125, 0},
};

// ------------------------------------------------------------------
// HELPERS
// ------------------------------------------------------------------


void read_midi_event(const int (*track)[3], uint16_t index, int out_values[3]) {
	for (uint8_t i = 0; i < 3; i++) {
		out_values[i] = pgm_read_word(&(track[index][i]));
	}
}

uint8_t usart_rx_available(void) {
	return (uint8_t)((rx_head - rx_tail) & RX_MASK);
}




// ------------------------------------------------------------------
// INITIALIZERS
// ------------------------------------------------------------------


// USART
void usart_init_9600(void) {
	const uint16_t ubrr = (16000000UL / (16UL * 9600)) - 1;
	UBRR0H = ubrr >> 8;
	UBRR0L = ubrr;
	UCSR0A = 0;
	UCSR0B = _BV(TXEN0) | _BV(RXEN0) | _BV(RXCIE0);   // <- RX interrupt
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);               // 8N1
}

// Escribir byte al buffer
uint8_t usart_write_try(uint8_t b) {
	uint8_t next = (uint8_t)((tx_head + 1) & TX_MASK);
	if (next == tx_tail) return 0;               // buffer lleno
	tx_buf[tx_head] = b;
	tx_head = next;
	UCSR0B |= _BV(UDRIE0);                       // kick the ISR
	return 1;
}

// Escribir string al buffer
uint16_t usart_write_str(const char *s) {
	uint16_t n = 0;
	while (*s && usart_write_try((uint8_t)*s++)) n++;
	return n;
}

// Leer byte del buffer de recepcion
uint8_t usart_read_try(uint8_t *b) {
	if (rx_head == rx_tail) return 0;                
	*b = rx_buf[rx_tail];
	rx_tail = (uint8_t)((rx_tail + 1) & RX_MASK);
	return 1;
}


// Overflow = 1ms
void timer1_init(void) {
	TCCR1A = 0x00;
	TCCR1B = (1 << CS11) | (1 << CS10);  // 64
	TCNT1 = 65536 - 250;
	TIMSK1 |= (1 << TOIE1);
}

// Botones del piano con sus interrupciones
void init_piano_buttons(void) {
	DDRC &= ~((1 << PORTC0) | (1 << PORTC1) | (1 << PORTC2) |
	(1 << PORTC3) | (1 << PORTC4) | (1 << PORTC5)); 
	PORTC |= (1 << PORTC0) | (1 << PORTC1) | (1 << PORTC2) |
	(1 << PORTC3) | (1 << PORTC4) | (1 << PORTC5);   

	DDRD &= ~((1 << PORTD4) | (1 << PORTD5)); 
	PORTD |=  (1 << PORTD4) | (1 << PORTD5);  

	DDRB  |= (1 << PORTB5);  

	PCICR = (1 << PCIE1) | (1 << PCIE2);

	PCMSK1 = (1 << PCINT8) | (1 << PCINT9) | (1 << PCINT10) |
	(1 << PCINT11) | (1 << PCINT12) | (1 << PCINT13);

	PCMSK2 = (1 << PCINT20) | (1 << PCINT21);
}
// ------------------------------------------------------------------
// UTILITY
// ------------------------------------------------------------------


// Playing frequencies --------------------------
// Reproducir frecuencia en buzzer 1
void playFrequencyA(uint16_t freq) {
	if (!freq) return;  

	DDRD |= (1 << PORTD6);
	
	// Elegir prescaler adecuado para esa frecuencia

	uint8_t presc_bits = 0;     
	uint16_t ocr = 0;

	const uint16_t presc_list[] = {8, 64, 256, 1024};
	const uint8_t  bits_list[]  = {0b010, 0b011, 0b100, 0b101};

	for (uint8_t i = 0; i < 4; i++) {
		ocr = (F_CPU / (2UL * presc_list[i] * freq)) - 1;
		if (ocr <= 255) {
			presc_bits = bits_list[i];
			break;
		}
	}

	TCCR0A = (1 << COM0A0) | (1 << WGM01);
	TCCR0B = presc_bits;          
	OCR0A  = (uint8_t)ocr;        
}

// Reproducir frecuencia en buzzer 2
void playFrequencyB(uint16_t freq) {
	if (!freq) return;

	DDRB |= (1 << PORTB3);

	uint8_t presc_bits = 0;
	uint16_t ocr = 0;

	const uint16_t presc_list[] = {8, 32, 64, 128, 256, 1024};
	const uint8_t  bits_list[]  = {0b010, 0b011, 0b100, 0b101, 0b110, 0b111};

	for (uint8_t i = 0; i < 6; i++) {
		ocr = (F_CPU / (2UL * presc_list[i] * freq)) - 1;
		if (ocr <= 255) {
			presc_bits = bits_list[i];
			break;
		}
	}

	TCCR2A = (1 << COM2A0) | (1 << WGM21);
	TCCR2B = presc_bits;
	OCR2A  = (uint8_t)ocr;
}

// Dejar de reproducir frencuencia buzzer 1
void stopFrequencyA(void) {
	TCCR0A = 0;
	TCCR0B = 0;
	DDRD  |=  (1 << PORTD6);
	PORTD &= ~(1 << PORTD6);
}

// Dejar de reproducir frencuencia buzzer 2
void stopFrequencyB(void) {
	TCCR2A = 0;
	TCCR2B = 0;
	DDRB  |=  (1 << PORTB3);
	PORTB &= ~(1 << PORTB3);
}



// Tracks --------------------------------------
// Reproducir melodia o track en buzzer 1
void playTrackA(void) {
	int values[3];
	if (song == 0){
		read_midi_event(midiC, indexA++, values);
	} else {
		read_midi_event(midiA, indexA++, values);
	}

	uint16_t freq = values[0];
	maxCountAon  = values[1];
	maxCountAoff = values[2];

	playFrequencyA(freq);       
	enableCountAon = 1;         
}

// Reproducir melodia o track en buzzer 2
void playTrackB(void) {
	int values[3];
	read_midi_event(midiB, indexB++, values);

	uint16_t freq = values[0];
	maxCountBon  = values[1];
	maxCountBoff = values[2];
	
	playFrequencyB(freq);
	enableCountBon = 1;  
}

// Buttons -------------------------------------

// Debouncing de botones
void startDebounceTimer(void) {
	debounce_ms = 0;
	debounce_active = 1;
	PCICR &= ~((1 << PCIE1) | (1 << PCIE2));
}

// Ccambio de estado de boton
void handleButtonChange(uint8_t PORT) {
	uint8_t current, changed, mask, pressed, released;

	if (PORT == 0) {
		current  = PINC & 0b00111111;
		changed  = current ^ prevC;
		pressed  = changed & prevC;
		released = changed & ~prevC;

		for (uint8_t i = 0; i < 6; i++) {
			mask = (1 << i);
			if (pressed & mask)
			playFrequencyB(NOTE_TABLE[i+2]);
			else if (released & mask)
			stopFrequencyB();
		}
		prevC = current;
	}
	else if (PORT == 1) {
		current  = PIND & 0b00110000;
		changed  = current ^ prevD;
		pressed  = changed & prevD;
		released = changed & ~prevD;

		for (uint8_t i = 4; i <= 5; i++) {
			mask = (1 << (i));
			if (pressed & mask)
			playFrequencyB(NOTE_TABLE[i-4]);
			else if (released & mask)
			stopFrequencyB();
		}
		prevD = current;
	}
}


// USART -------------------------------------
// Manejo de cambio de estados de usart
void handleUSART(uint8_t character){
	if (character == '1'){
		mode = 1;
		eventAoff = 1;
		eventBoff = 0;
		
		song = 0;
		
		eventAon = 0; // Encender track A
		indexA = 0; // Posicion track A
		countA = 0; // Conteo de overflow de notas de track A
		maxCountAon = 0; // Maximo conteo de overflow encendido en A
		maxCountAoff = 0; // Maximo conteo de overflow apagado en A
		enableCountAon = 0; // Habilitar conteo de encendido en A
		enableCountAoff = 0; // Habilidad conteo de apagado en A
		
		eventBon = 0; // Encender track B
		indexB = 0; // Posicion track B
		countB = 0; // Conteo de overflow de notas de track B
		maxCountBon = 0; // Maximo conteo de overflow encendido en B
		maxCountBoff = 0; // Maximo conteo de overflow apagado en B
		enableCountBon = 0; // Habilitar conteo de encendido en B
		enableCountBoff = 0; // Habilidad conteo de apagado en B
		
		PCICR &= ~((1 << PCIE1) | (1 << PCIE2));
		stopFrequencyB();
		
	} else if (character == '2'){
		mode = 1;
		eventAoff = 1;
		eventBoff = 1;
		
		song = 1;
		
		eventAon = 0; // Encender track A
		indexA = 0; // Posicion track A
		countA = 0; // Conteo de overflow de notas de track A
		maxCountAon = 0; // Maximo conteo de overflow encendido en A
		maxCountAoff = 0; // Maximo conteo de overflow apagado en A
		enableCountAon = 0; // Habilitar conteo de encendido en A
		enableCountAoff = 0; // Habilidad conteo de apagado en A
		
		eventBon = 0; // Encender track B
		indexB = 0; // Posicion track B
		countB = 0; // Conteo de overflow de notas de track B
		maxCountBon = 0; // Maximo conteo de overflow encendido en B
		maxCountBoff = 0; // Maximo conteo de overflow apagado en B
		enableCountBon = 0; // Habilitar conteo de encendido en B
		enableCountBoff = 0; // Habilidad conteo de apagado en B
		
		PCICR &= ~((1 << PCIE1) | (1 << PCIE2));
		stopFrequencyA();

	} else if (character == 'P'){
		mode = 0;
		eventAoff = 0;
		eventBoff = 0;
				
		eventAon = 0; // Encender track A
		indexA = 0; // Posicion track A
		countA = 0; // Conteo de overflow de notas de track A
		maxCountAon = 0; // Maximo conteo de overflow encendido en A
		maxCountAoff = 0; // Maximo conteo de overflow apagado en A
		enableCountAon = 0; // Habilitar conteo de encendido en A
		enableCountAoff = 0; // Habilidad conteo de apagado en A
				
		eventBon = 0; // Encender track B
		indexB = 0; // Posicion track B
		countB = 0; // Conteo de overflow de notas de track B
		maxCountBon = 0; // Maximo conteo de overflow encendido en B
		maxCountBoff = 0; // Maximo conteo de overflow apagado en B
		enableCountBon = 0; // Habilitar conteo de encendido en B
		enableCountBoff = 0; // Habilidad conteo de apagado en B
		
		startDebounceTimer();
		stopFrequencyA();
		stopFrequencyB();
	}
}






// ------------------------------------------------------------------
// MAIN LOOP
// ------------------------------------------------------------------

// Esqueletors
void piano_mode(void);
void song_mode(void);

// Programa principal
int main(void) {
	timer1_init();
	usart_init_9600();
	init_piano_buttons();
	sei();
	
	usart_write_str("Elija una opcion:\r\n");
	usart_write_str("[1] Dragon Ball - Cha-La Head-Cha-La\r\n");
	usart_write_str("[2] Portal - Still alive\r\n");
	usart_write_str("[P] Modo piano\r\n");

	
	while (1){
		if (mode == 0){
			piano_mode();
		} else if (mode == 1){
			song_mode();
		}
	    uint8_t c;
	    if (usart_read_try(&c)) {
		    handleUSART(c);
	    }
	}
}


// ------------------------------------------------------------------
// STATES
// ------------------------------------------------------------------

// Pq si
void piano_mode(void){
	return;
}


// Modo cancioncita
void song_mode(void){
	
	// Track A
	if (eventAon){
		countA = 0;
		enableCountAon = 0;
		eventAon = 0;
		
		stopFrequencyA();
		enableCountAoff = 1;
		
	} else if (eventAoff){
		countA = 0;
		enableCountAoff = 0;
		eventAoff = 0;
		playTrackA();
	}
	
	// Track B
	if (eventBon) {
		countB = 0;
		enableCountBon = 0;
		eventBon = 0;
		
		stopFrequencyB();
		enableCountBoff = 1;
		
	} else if (eventBoff){
		countB = 0;
		enableCountBoff = 0;
		eventBoff = 0;
		playTrackB();
		
	}
}

// ------------------------------------------------------------------
// INTERRUPT SERVICE ROUTINES	
// ------------------------------------------------------------------

ISR(TIMER1_OVF_vect){
	TCNT1 = 65536 - 250;  // 1ms preload
	
	// Debouncing
	if (debounce_active) {  // 1 ms debounce
		PCICR |= (1 << PCIE1) | (1 << PCIE2);
		debounce_active = 0;
	}
	
	// Note playing
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

// Debounce timer
ISR(TIMER0_OVF_vect) {
	PCICR |= (1 << PCIE1) | (1 << PCIE2);
	TCCR0B = 0;
}

// Piano buttons
ISR(PCINT1_vect) {   
	handleButtonChange(0);
	PCICR &= ~((1 << PCIE1) | (1 << PCIE2));
	startDebounceTimer();
}

ISR(PCINT2_vect) {   
	handleButtonChange(1);
	PCICR &= ~((1 << PCIE1) | (1 << PCIE2));
	startDebounceTimer();
}

// USART
ISR(USART_UDRE_vect) {
	if (tx_head == tx_tail) {                    
		UCSR0B &= (uint8_t)~_BV(UDRIE0);         
		return;
	}
	UDR0 = tx_buf[tx_tail];
	tx_tail = (uint8_t)((tx_tail + 1) & TX_MASK);
}

ISR(USART_RX_vect) {
	uint8_t d = UDR0;
	uint8_t next = (uint8_t)((rx_head + 1) & RX_MASK);
	if (next != rx_tail) {                
		rx_buf[rx_head] = d;
		rx_head = next;
	}
}