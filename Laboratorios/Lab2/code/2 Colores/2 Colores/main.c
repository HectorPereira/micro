/*
Problema B
	Sistema de Selección de Colores con ATmega328P, Fotocelda, Tira de
	LEDs WS2812 y Servomotor

Descripción General del Proyecto
	El sistema consiste en un selector de colores basado en un microcontrolador
	ATmega328P, una fotocelda, una tira de LEDs WS2812 y un servomotor. La
	idea principal es detectar el color presente en una hoja de referencia utilizando
	la fotocelda, posicionar un servomotor en un ángulo determinado en función del
	color identificado y mostrar dicho color en la tira de LEDs WS2812.

Componentes Utilizados
	? Microcontrolador ATmega328P
	? Fotocelda (Sensor de Luz)
	? Tira de LEDs WS2812
	? Servomotor
	? Comunicación Serial

Comunicación Serial
	? Valor de la fotocelda
	? Color detectado
	? Valor del color establecido
	? Diferencia entre valor establecido y valor de lectura

Proceso de Funcionamiento
	El microcontrolador se encargará de leer los valores de la fotocelda (sensor de
	luz), realizar el procesamiento para identificar el color, controlar el servomotor
	para moverlo al ángulo correspondiente al color detectado y encender la tira de
	LEDs WS2812 mostrando el mismo color identificado.
	Este sistema se configurará para detectar un número limitado de colores (los
	presentes en la hoja de referencia). Cada color tendrá un valor ADC
	(Analog-to-Digital Conversion) preestablecido para asegurar la correcta
	identificación y su representación precisa en la tira de LEDs.

 */ 

#include <xc.h>

int main(void)
{
    while(1)
    {
        //TODO:: Please write your application code 
    }
}