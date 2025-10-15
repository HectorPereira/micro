/*
 * Sleep-mode demo for ATmega328P
 * 6 LEDs on PC0..PC5
 *
 * Sequence:
 *  - ON 5s, OFF 5s (awake)
 *  - ON 5s, OFF, ADC Noise Reduction sleep 5s
 *  - ON 5s, OFF, Power-save sleep 5s
 *  - ON 5s, OFF, Power-down sleep 5s
 *
 * Fuse note: keep RSTDISBL=unprogrammed (PC6 is RESET; don’t use it as LED).
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

// ------------------ LEDs on PORTC (PC0..PC5) ------------------
static inline void leds_init(void) {
    DDRC  |= 0b00111111;   // PC0..PC5 outputs
    PORTC &= ~0b00111111;  // start OFF (low)
}
static inline void leds_on(void)  { PORTC |=  0b00111111; }
static inline void leds_off(void) { PORTC &= ~0b00111111; }

// ------------------ Power reduction helpers -------------------
static inline void pr_enable_all(void)   { PRR = 0; }  // everything on
static inline void pr_reduce_for_sleep(void) {
    // Turn off modules you don’t need while sleeping to cut leakage.
    // Keep ADC for ADC Noise Reduction; otherwise switch it off too.
    PRR = _BV(PRTWI) | _BV(PRTIM1) | _BV(PRTIM0) | _BV(PRSPI) | _BV(PRUSART0);
    // Leave Timer2 & ADC on; Timer2 helps Power-save, ADC for ADC NR mode.
}

// ------------------ Watchdog tick (?1 s) ----------------------
volatile uint8_t wdt_ticks;

static void wdt_setup_1s_int(void) {
    cli();
    // Enter timed sequence
    WDTCSR = _BV(WDCE) | _BV(WDE);
    // Set ~1s period (WDP2|WDP1), interrupt only (WDIE), no reset
    WDTCSR = _BV(WDIE) | _BV(WDP2) | _BV(WDP1);  // ~1.0 s
    sei();
}

ISR(WDT_vect) { 
    if (wdt_ticks < 255) wdt_ticks++; 
}

// Sleep for N seconds in the given sleep mode, using the 1 s WDT tick
static void sleep_for_seconds(uint8_t seconds, uint8_t mode) {
    wdt_ticks = 0;
    set_sleep_mode(mode);
    pr_reduce_for_sleep();

    while (wdt_ticks < seconds) {
        sleep_enable();
        // On many AVRs this lowers current further during sleep:
        #ifdef sleep_bod_disable
        sleep_bod_disable();
        #endif
        sleep_cpu();            // Sleep until WDT interrupt
        sleep_disable();        // Always disable after waking
    }

    // Restore peripherals for awake operation
    pr_enable_all();
}

// Simple blocking wait (busy) for exact 5s when awake
static void wait_ms_blocking(uint16_t ms) {
    while (ms--) _delay_ms(1);
}

// ------------------ Main ------------------
int main(void) {
    leds_init();
    pr_enable_all();
    wdt_setup_1s_int();  // we’ll use WDT both for sleeps and as a heartbeat wake

    // 1) Awake: ON 5s, OFF 5s
    leds_on();  wait_ms_blocking(5000);
    leds_off(); wait_ms_blocking(5000);

    // 2) ON 5s, OFF, ADC Noise Reduction sleep 5s
    leds_on();  wait_ms_blocking(5000);
    leds_off();
    sleep_for_seconds(5, SLEEP_MODE_ADC);

    // 3) ON 5s, OFF, Power-save sleep 5s
    leds_on();  wait_ms_blocking(5000);
    leds_off();
    sleep_for_seconds(5, SLEEP_MODE_PWR_SAVE);

    // 4) ON 5s, OFF, Power-down sleep 5s
    leds_on();  wait_ms_blocking(5000);
    leds_off();
    sleep_for_seconds(5, SLEEP_MODE_PWR_DOWN);

    // Finished: leave LEDs OFF and loop forever (awake)
    while (1) { /* idle */ }
}
