#define F_CPU 1000000UL
#include <avr/io.h>
#include <util/delay_basic.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/pgmspace.h>


typedef struct {
	char cmd;       // e.g., 'D' = draw (pen down), 'U' = pen up, etc.
	float dx_mm;    // relative X move (mm)
	float dy_mm;    // relative Y move (mm)
} Move;



#include "circle_data_path.h"


// Distance per movement step (mm)

#define PATH_SCALE 1.0f   


// -------------------- PIN MAP --------------------
#define STEP_X PB3
#define DIR_X  PB4
#define EN_X   PB5

#define STEP_Y PC3
#define DIR_Y  PC4
#define EN_Y   PC5

#define STEP_SCALE 18.46f     // steps per mm (calibrated)
#define SOLENOID PC0          // Active-HIGH = pen down

static inline void delay_cycles(uint16_t n){ _delay_loop_2(n); }


// -------------------- INITIALIZATION --------------------
void plotter_init(void) {
    DDRB |= (1<<STEP_X)|(1<<DIR_X)|(1<<EN_X);
    DDRC |= (1<<STEP_Y)|(1<<DIR_Y)|(1<<EN_Y)|(1<<SOLENOID);

    // Enable motor drivers (active-HIGH)
    PORTB |= (1<<EN_X);
    PORTC |= (1<<EN_Y);

    // Pen up
    PORTC &= ~(1<<SOLENOID);
}

// -------------------- PEN CONTROL --------------------
void pen_up(void) {
    PORTC |= (1<<SOLENOID);
}

void pen_down(void) {
    PORTC &= ~(1<<SOLENOID);
}

// -------------------- CORE MOVE FUNCTION --------------------
void move_axis(volatile uint8_t *port_dir, uint8_t dir_bit,
               volatile uint8_t *port_step, uint8_t step_bit,
               uint8_t direction, uint16_t steps)
{
    if(direction) *port_dir |=  (1<<dir_bit);
    else           *port_dir &= ~(1<<dir_bit);

    delay_cycles(1);  // direction setup time

	for (uint16_t i = 0; i < steps; i++) {
		*port_step |=  (1<<step_bit);
		delay_cycles(300);
		*port_step &= ~(1<<step_bit);
		delay_cycles(300);
	}
 }

// -------------------- WRAPPERS WITH POSITION TRACKING --------------------
void move_x(uint8_t dir, float dist_mm) {
    uint16_t steps = (uint16_t)(dist_mm * STEP_SCALE);
    move_axis(&PORTB, DIR_X, &PORTB, STEP_X, dir, steps);


}

void move_y(uint8_t dir, float dist_mm) {
    uint16_t steps = (uint16_t)(dist_mm * STEP_SCALE);
    move_axis(&PORTC, DIR_Y, &PORTC, STEP_Y, dir, steps);


}


void execute_path(const Move *path, uint16_t size) {
	for (uint16_t i = 0; i < size / sizeof(Move); i++) {
		Move m;
		m.cmd   = pgm_read_byte(&path[i].cmd);
		m.dx_mm = pgm_read_float(&path[i].dx_mm);
		m.dy_mm = pgm_read_float(&path[i].dy_mm);

		// Pen command
		if (m.cmd == 'U') {
			pen_up();
			continue;
			} else if (m.cmd == 'D') {
			pen_down();
		}

		if (m.dx_mm > 0.0f)
		move_x(1, m.dx_mm * PATH_SCALE);
		else if (m.dx_mm < 0.0f)
		move_x(0, -m.dx_mm * PATH_SCALE);

		if (m.dy_mm > 0.0f)
		move_y(1, m.dy_mm * PATH_SCALE);
		else if (m.dy_mm < 0.0f)
		move_y(0, -m.dy_mm * PATH_SCALE);

		_delay_ms(10);
	}
}


// -------------------- MAIN TEST --------------------
int main(void) {
	plotter_init();

	while (1) {
		execute_path(circle_data_path, sizeof(circle_data_path));
		_delay_ms(2000);
	}
}