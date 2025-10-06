#ifndef MIDI_STORAGE
#define MIDI_STORAGE

#include <avr/io.h>
#include <xc.h>
#include <avr/pgmspace.h>

// Notas
#define G4 392*4
#define Fb4 370*4
#define E4 330*4
#define D4 294*4
#define A3 220*4
#define Cb4 277*4
#define F4 349*4
#define C4 262*4
#define Ab3 233*4
#define A4 440*4
#define Ab4 466*4
#define B4 494*4
#define A2 110*4
#define D3 147*4
#define Fb3 185*4
#define B2 123*4
#define E3 165*4
#define G3 196*4
#define Cb3 139*4
#define Ab2 117*4
#define F3 175*4
#define C3 131*4
#define G2 98*4

const int midiA[349][3] PROGMEM;
const int midiB[433][3] PROGMEM;

#endif