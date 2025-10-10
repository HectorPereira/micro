/*
 * main.c
 *
 * Created: 10/21/2023 6:47:37 PM
 *  Author: Jebus
 *  Funcional ajuste tres espacios para linea 1
 */ 

#define PCF8574	0x27							

#include <xc.h>
#include <avr/io.h>
#include <util/twi.h>
#define F_CPU	16000000UL
#include <util/delay.h>
#include "twi_lcd.h"

int main(void)
{
	twi_init();									
	twi_lcd_init();								
	twi_lcd_cmd(0x80);				
	twi_lcd_clear();	
	twi_lcd_msg("Linea 1!!");
	while (1)
	{
		//--- Send a String to LCD 	
					
	}
}

