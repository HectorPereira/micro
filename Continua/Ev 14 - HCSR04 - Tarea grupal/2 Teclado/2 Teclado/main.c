/*
 * main.c
 *
 * Created: 10/3/2025 6:13:36 PM
 *  Author: isacm
 */ 

#include <xc.h>

#define LO_NIBBLE(x)   ((uint8_t)((x) & 0x0F))
#define HI_NIBBLE(x)   ((uint8_t)(((x) >> 4) & 0x0F)) //Para cargar los nibbles

#define a 0b00110010
#define b 0b00101000
#define c 0b00001100
#define d 0b00000001  
#define LCD_RS PB5
#define LCD_E  PB4

void setearLCD(void){
	PORTB &= ~(1 << LCD_RS); // para cargar instrucciones
	
	
	PORTB = (PORTB & 0xF0) | LO_NIBBLE(x);
	// Pulso a E
	PORTB |=  (1 << LCD_E);
	_delay_us(1);
	PORTB &= ~(1 << LCD_E);
	
	PORTB = (PORTB & 0xF0) | HI_NIBBLE(x);
	
	// Pulso a E
	PORTB |=  (1 << LCD_E);
	_delay_us(1);
	PORTB &= ~(1 << LCD_E);
	
	
}
char keypad[4][4] = {
	{ '1', '2', '3', 'A' },
	{ '4', '5', '6', 'B' },
	{ '7', '8', '9', 'C' },
	{ '*', '0', '#', 'D' }
};


int main(void)
{
	
	DDRB |= 0b00111111;
	PORTB &= ~(1 << LCD_RS);
	
	
	setearLCD(a)
	setearLCD(b)
	setearLCD(c)
	setearLCD(d)
	
    while(1)
    {
        //TODO:: Please write your application code 
    }
}
