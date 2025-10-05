#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

#define TRIG_DDR   DDRB
#define TRIG_PORT  PORTB	
#define TRIG_PIN   PB1        

#define ECHO_DDR   DDRB
#define ECHO_PIN   PB0        

#define LED_DDR    DDRD
#define LED_PIN    PD6

static volatile uint16_t icr_start = 0;
static volatile uint16_t icr_end   = 0;
static volatile uint8_t  capturing = 0;      
static volatile uint8_t  done_flag = 0;
static volatile uint32_t ovf_count = 0;

ISR(TIMER1_CAPT_vect){
	uint16_t icr = ICR1;
	if (!capturing) {
		icr_start = icr;
		ovf_count = 0;
		capturing = 1;
		TCCR1B &= ~(1 << ICES1);     
		} else {
		icr_end = icr;
		capturing = 0;
		done_flag = 1;
		TCCR1B |= (1 << ICES1);      
	}
}

ISR(TIMER1_OVF_vect){
	if (capturing) ovf_count++;
}

static inline void hcsr04_trigger_10us(void){
	TRIG_PORT |= (1 << TRIG_PIN);
	_delay_us(12);
	TRIG_PORT &= ~(1 << TRIG_PIN);
}

static inline void hcsr04_init(void){
	TRIG_DDR  |=  (1 << TRIG_PIN);
	TRIG_PORT &= ~(1 << TRIG_PIN);

	ECHO_DDR  &= ~(1 << ECHO_PIN);

	TCCR1A = 0;
	TCCR1B = (1 << ICNC1) | (1 << ICES1) | (1 << CS11);
	TCCR1C = 0;

	TIFR1 = (1 << ICF1) | (1 << TOV1);
	TIMSK1 = (1 << ICIE1) | (1 << TOIE1);

	sei();
}

static uint16_t hcsr04_read_cm_blocking(void){
	done_flag = 0;
	capturing = 0;
	TCCR1B |= (1 << ICES1);        
	TIFR1 = (1 << ICF1) | (1 << TOV1);

	hcsr04_trigger_10us();

	uint16_t wait_us = 40000;
	while (!done_flag && wait_us--) _delay_us(1);
	if (!done_flag) return 0xFFFF;

	uint32_t ticks = (uint32_t)ovf_count * 65536UL + (uint16_t)(icr_end - icr_start);
	uint32_t t_us = ticks >> 1;

	return (uint16_t)((t_us + 29U) / 58U);
}

static inline void pwm_init_timer0_oc0a(void){
	LED_DDR |= (1 << LED_PIN);   
	TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);
	TCCR0B = (1 << CS01) | (1 << CS00);
	OCR0A = 0; 
}

static inline uint8_t map_cm_to_duty(uint16_t cm){
	if (cm == 0xFFFF) return 0;  

	const uint16_t MIN_CM = 5;    
	const uint16_t MAX_CM = 200;   

	if (cm <= MIN_CM) return 255;
	if (cm >= MAX_CM) return 0;

	uint32_t span = (uint32_t)(MAX_CM - MIN_CM);
	uint32_t val  = (uint32_t)(MAX_CM - cm); 
	uint32_t duty = (val * 255U + (span / 2)) / span; 
	return (uint8_t)duty;
}

int main(void){
	hcsr04_init();
	pwm_init_timer0_oc0a();
	uint8_t duty = 0;

	for (;;) {	
		uint16_t d_cm = hcsr04_read_cm_blocking();
		uint8_t target = map_cm_to_duty(d_cm);
		duty += (target-duty)/4;
		OCR0A = duty;
		_delay_ms(70);  
	}
}
