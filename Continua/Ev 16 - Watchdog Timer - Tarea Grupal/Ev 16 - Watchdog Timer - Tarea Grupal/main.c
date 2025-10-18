/*
Implementar un circuito que use al menos 3 modos de sleep 
que encienda 5 leds y apague durante 30 segundos.

Calcular el consumo de corriente y de energía de cada uno 
de los modos (activo y modo sleep) y calcular la batería que necesitase 
para que su circuito permanezca funcionando durante (número de grupo) horas.

Entregar: Código, Github, Video con el consumo de los tres modos, 
Gráficas de Consumo en función del Tiempo.
*/


#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <util/delay.h>

#ifndef _BV
#define _BV(b) (1U<<(b))
#endif

static inline void leds_init(void) {
    DDRC  |= 0b00111111;   
    PORTC &= ~0b00111111;  
}
static inline void leds_on(void)  { PORTC |=  0b00111111; }
static inline void leds_off(void) { PORTC &= ~0b00111111; }

static inline void pr_enable_all(void)   { PRR = 0; }  
static inline void pr_reduce_for_sleep(void) {
    PRR = _BV(PRTWI) | _BV(PRTIM1) | _BV(PRTIM0) | _BV(PRSPI) | _BV(PRUSART0);
}

volatile uint8_t wdt_ticks;

static void wdt_setup_1s_int(void) {
    cli();
    WDTCSR = _BV(WDCE) | _BV(WDE);
    WDTCSR = _BV(WDIE) | _BV(WDP2) | _BV(WDP1);  
    sei();
}

ISR(WDT_vect) { 
    if (wdt_ticks < 255) wdt_ticks++; 
}

static void sleep_for_seconds(uint8_t seconds, uint8_t mode) {
    wdt_ticks = 0;
    set_sleep_mode(mode);
    pr_reduce_for_sleep();

    while (wdt_ticks < seconds) {
        sleep_enable();
        #ifdef sleep_bod_disable
        sleep_bod_disable();
        #endif
        sleep_cpu();            
        sleep_disable();        
    }

    pr_enable_all();
}

static void wait_ms_blocking(uint16_t ms) {
    while (ms--) _delay_ms(1);
}

int main(void) {
    leds_init();
    pr_enable_all();
    wdt_setup_1s_int();  

    leds_on();  wait_ms_blocking(5000);
    leds_off(); wait_ms_blocking(5000);

    leds_on();  wait_ms_blocking(5000);
    leds_off();
    sleep_for_seconds(5, SLEEP_MODE_ADC);

    leds_on();  wait_ms_blocking(5000);
    leds_off();
    sleep_for_seconds(5, SLEEP_MODE_PWR_SAVE);

    leds_on();  wait_ms_blocking(5000);
    leds_off();
    sleep_for_seconds(5, SLEEP_MODE_PWR_DOWN);

    while (1);
}
