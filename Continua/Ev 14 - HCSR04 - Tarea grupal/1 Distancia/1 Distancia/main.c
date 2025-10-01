#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

/* ---------------- Pins ---------------- */
#define TRIG_DDR   DDRB
#define TRIG_PORT  PORTB
#define TRIG_PIN   PB1        // Arduino D9 (TRIG)

#define ECHO_DDR   DDRB
#define ECHO_PIN   PB0        // Arduino D8 (ICP1 ECHO)

// LED on OC0A (PD6 / D6)
#define LED_DDR    DDRD
#define LED_PIN    PD6

/* ------------ Globals (shared with ISRs) ------------ */
static volatile uint16_t icr_start = 0;
static volatile uint16_t icr_end   = 0;
static volatile uint8_t  capturing = 0;      // 0=idle, 1=between edges
static volatile uint8_t  done_flag = 0;
static volatile uint32_t ovf_count = 0;

/* ------------- Timer1 / ICP1 ISRs ------------- */
ISR(TIMER1_CAPT_vect)
{
	uint16_t icr = ICR1;

	if (!capturing) {
		icr_start = icr;
		ovf_count = 0;
		capturing = 1;
		TCCR1B &= ~(1 << ICES1);     // next: falling edge
		} else {
		icr_end = icr;
		capturing = 0;
		done_flag = 1;
		TCCR1B |= (1 << ICES1);      // re-arm for rising
	}
}

ISR(TIMER1_OVF_vect)
{
	if (capturing) ovf_count++;
}

/* ------------- HC-SR04 helpers ------------- */
static inline void hcsr04_trigger_10us(void)
{
	TRIG_PORT |= (1 << TRIG_PIN);
	_delay_us(12);
	TRIG_PORT &= ~(1 << TRIG_PIN);
}

static inline void hcsr04_init(void)
{
	// TRIG as output low
	TRIG_DDR  |=  (1 << TRIG_PIN);
	TRIG_PORT &= ~(1 << TRIG_PIN);

	// ECHO (ICP1) as input
	ECHO_DDR  &= ~(1 << ECHO_PIN);

	// ---- Timer1: input capture timebase ----
	TCCR1A = 0;
	// ICNC1=1 (noise cancel), ICES1=1 (rising first), CS11=1 (prescaler /8 -> 0.5 us/tick)
	TCCR1B = (1 << ICNC1) | (1 << ICES1) | (1 << CS11);
	TCCR1C = 0;

	// Clear pending flags
	TIFR1 = (1 << ICF1) | (1 << TOV1);

	// Enable interrupts
	TIMSK1 = (1 << ICIE1) | (1 << TOIE1);

	sei();
}

/* Blocking read: returns distance in cm, or 0xFFFF on timeout */
static uint16_t hcsr04_read_cm_blocking(void)
{
	done_flag = 0;
	capturing = 0;
	TCCR1B |= (1 << ICES1);        // rising first
	TIFR1 = (1 << ICF1) | (1 << TOV1);

	hcsr04_trigger_10us();

	// ~40 ms timeout window
	uint16_t wait_us = 40000;
	while (!done_flag && wait_us--) _delay_us(1);
	if (!done_flag) return 0xFFFF;

	// ticks = (end - start) + 65536*overflows
	uint32_t ticks = (uint32_t)ovf_count * 65536UL + (uint16_t)(icr_end - icr_start);

	// 0.5 us per tick -> t_us = ticks/2
	uint32_t t_us = ticks >> 1;

	// distance_cm ? t_us / 58 (rounded)
	return (uint16_t)((t_us + 29U) / 58U);
}

/* ------------- Timer0 PWM on OC0A (PD6) ------------- */
static inline void pwm_init_timer0_oc0a(void)
{
	LED_DDR |= (1 << LED_PIN);   // PD6 as output

	// Fast PWM, TOP=0xFF; non-inverting on OC0A
	// COM0A1=1, WGM01=1, WGM00=1
	TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);

	// Prescaler /64 -> ~976 Hz PWM (quiet, flicker-free)
	// CS01=1, CS00=1
	TCCR0B = (1 << CS01) | (1 << CS00);

	OCR0A = 0; // start off
}

/* Map distance [MIN_CM..MAX_CM] -> duty [255..0] (close = bright) */
static inline uint8_t map_cm_to_duty(uint16_t cm)
{
	if (cm == 0xFFFF) return 0;  // no echo -> LED off

	const uint16_t MIN_CM = 5;     // clamp near limit
	const uint16_t MAX_CM = 200;   // far limit

	if (cm <= MIN_CM) return 255;
	if (cm >= MAX_CM) return 0;

	uint32_t span = (uint32_t)(MAX_CM - MIN_CM);
	uint32_t val  = (uint32_t)(MAX_CM - cm); // invert so close -> big value
	uint32_t duty = (val * 255U + (span / 2)) / span; // round
	return (uint8_t)duty;
}

/* ------------------ Example main ------------------ */
int main(void)
{
	hcsr04_init();
	pwm_init_timer0_oc0a();

	// Optional: simple slew to reduce visible flicker
	uint8_t duty = 0;

	for (;;) {
		uint16_t d_cm = hcsr04_read_cm_blocking();
		uint8_t target = map_cm_to_duty(d_cm);

		// Slew 1 step per loop toward target (optional)
		if (duty < target) duty = duty + 5;
		else if (duty > target) duty = duty - 5;

		OCR0A = duty;

		_delay_ms(10);  // re-trigger interval per sensor guidelines
	}
}
