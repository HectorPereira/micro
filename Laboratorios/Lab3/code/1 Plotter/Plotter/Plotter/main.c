#define F_CPU 1000000UL
#include <avr/io.h>
#include <util/delay_basic.h>
#include <util/delay.h>

// -------------------- PIN MAP --------------------
#define STEP_X PB3
#define DIR_X  PB4
#define EN_X   PB5

#define STEP_Y PC3
#define DIR_Y  PC4
#define EN_Y   PC5

#define SOLENOID PC0     // active-HIGH = pen down

// delay helper
static inline void delay_cycles(uint16_t n){ _delay_loop_2(n); }

// -------------------- INITIALIZATION --------------------
void plotter_init(void) {
	DDRB |= (1<<STEP_X)|(1<<DIR_X)|(1<<EN_X);
	DDRC |= (1<<STEP_Y)|(1<<DIR_Y)|(1<<EN_Y)|(1<<SOLENOID);

	// Enable drivers (active-HIGH)
	PORTB |=  (1<<EN_X);
	PORTC |=  (1<<EN_Y);

	// Start with pen up
	PORTC &= ~(1<<SOLENOID);
}

// -------------------- CORE MOVE FUNCTION --------------------
void move_axis(volatile uint8_t *port_dir, uint8_t dir_bit,
volatile uint8_t *port_step, uint8_t step_bit,
uint8_t direction, uint16_t steps)
{
	if(direction) *port_dir |=  (1<<dir_bit);
	else           *port_dir &= ~(1<<dir_bit);

	delay_cycles(50);  // small setup time

	for(uint16_t i = 0; i < steps; i++) {
		*port_step |=  (1<<step_bit);
		delay_cycles(300);     // ~1.2 ms high
		*port_step &= ~(1<<step_bit);
		delay_cycles(300);     // ~1.2 ms low
	}
}

// Wrappers for readability
void move_x(uint8_t dir, uint16_t steps) {
	move_axis(&PORTB, DIR_X, &PORTB, STEP_X, dir, steps);
}

void move_y(uint8_t dir, uint16_t steps) {
	move_axis(&PORTC, DIR_Y, &PORTC, STEP_Y, dir, steps);
}

// -------------------- SOLENOID CONTROL --------------------
void pen_down(void) {
	PORTC &= ~(1<<SOLENOID);  // deactivate solenoid
	_delay_ms(100);           // short stabilization delay
}

void pen_up(void) {
	PORTC |= (1<<SOLENOID);   // activate solenoid
	_delay_ms(100);
}

// -------------------- MAIN TEST --------------------
int main(void) {
	plotter_init();

	while(1) {
		pen_down();   // lower pen to draw
		move_x(1, 200);
		move_y(1, 200);
		move_x(0, 200);
		move_y(0, 200);

		pen_up();     // lift pen
		_delay_ms(1000);
	}
}
