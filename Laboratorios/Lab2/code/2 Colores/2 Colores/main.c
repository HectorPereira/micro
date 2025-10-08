/*
Problema B
	Sistema de Selecci�n de Colores con ATmega328P, Fotocelda, Tira de
	LEDs WS2812 y Servomotor

Descripci�n General del Proyecto
	El sistema consiste en un selector de colores basado en un microcontrolador
	ATmega328P, una fotocelda, una tira de LEDs WS2812 y un servomotor. La
	idea principal es detectar el color presente en una hoja de referencia utilizando
	la fotocelda, posicionar un servomotor en un �ngulo determinado en funci�n del
	color identificado y mostrar dicho color en la tira de LEDs WS2812.

Componentes Utilizados
	? Microcontrolador ATmega328P
	? Fotocelda (Sensor de Luz)
	? Tira de LEDs WS2812
	? Servomotor
	? Comunicaci�n Serial

Comunicaci�n Serial
	? Valor de la fotocelda
	? Color detectado
	? Valor del color establecido
	? Diferencia entre valor establecido y valor de lectura

Proceso de Funcionamiento
	El microcontrolador se encargar� de leer los valores de la fotocelda (sensor de
	luz), realizar el procesamiento para identificar el color, controlar el servomotor
	para moverlo al �ngulo correspondiente al color detectado y encender la tira de
	LEDs WS2812 mostrando el mismo color identificado.
	Este sistema se configurar� para detectar un n�mero limitado de colores (los
	presentes en la hoja de referencia). Cada color tendr� un valor ADC
	(Analog-to-Digital Conversion) preestablecido para asegurar la correcta
	identificaci�n y su representaci�n precisa en la tira de LEDs.

 */ 

#include <xc.h>

int main(void)
{
    while(1)
    {
        //TODO:: Please write your application code 
    }
}