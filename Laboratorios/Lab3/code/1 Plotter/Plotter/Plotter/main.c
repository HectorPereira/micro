#define F_CPU 1000000UL
#include <avr/io.h>
#include <util/delay_basic.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>


typedef struct {
	char cmd;       // e.g., 'D' = draw (pen down), 'U' = pen up, etc.
	float dx_mm;    // relative X move (mm)
	float dy_mm;    // relative Y move (mm)
} Move;

#define LIMIT_Y_MIN   PD2   // INT0
#define LIMIT_Y_MAX   PD3   // INT1

// Active-low inputs with pull-ups
#define LIMIT_MIN_ACTIVE()   ((PIND & (1<<LIMIT_Y_MIN)) == 0)
#define LIMIT_MAX_ACTIVE()   ((PIND & (1<<LIMIT_Y_MAX)) == 0)

// ---------- Debounce config ----------
#define DEBOUNCE_MS  15      // tune between 8–30 ms as needed

// ---------- State ----------
volatile uint8_t  g_limit_hit = 0;   // set to 1 when a limit is validated
volatile uint8_t  g_limit_src = 0xFF;// 0=min, 1=max
volatile uint32_t g_ms = 0;          // millisecond tick

// Debounce windows for each interrupt line
volatile uint8_t  db0_armed = 0, db1_armed = 0;
volatile uint32_t db0_deadline = 0, db1_deadline = 0;

#include "murcielago_path.h"
#include "circle_data_path.h"
#include "flor_path.h"
// #include "flor_path.h"


// Distance per movement step (mm)



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
	if (g_limit_hit) return;
    if(direction) *port_dir |=  (1<<dir_bit);
    else           *port_dir &= ~(1<<dir_bit);

    delay_cycles(1);  // direction setup time

	for (uint16_t i = 0; i < steps; i++) {
		if (g_limit_hit) break;		
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

static inline void emergency_stop(void){
	pen_up();
	PORTB &= ~(1<<EN_X);
	PORTC &= ~(1<<EN_Y);
}


static void debounce_timer_init(void){
	// Timer0 CTC @ 1 kHz: F_CPU=16 MHz, presc=64 ? tick=4 µs; 250 ticks = 1 ms
	TCCR0A = (1<<WGM01);            // CTC
	TCCR0B = 0;
	OCR0A  = 249;                   // 0..249 = 250 counts
	TIMSK0 = (1<<OCIE0A);           // enable compare A interrupt
	TCCR0B = (1<<CS01) | (1<<CS00); // prescaler /64

}


// ---------------- Limits init ----------------
static void limits_init(void){
	// Inputs + pull-ups
	DDRD  &= ~((1<<LIMIT_Y_MIN) | (1<<LIMIT_Y_MAX));
	PORTD |=  ((1<<LIMIT_Y_MIN) | (1<<LIMIT_Y_MAX));

	// INT0/INT1 on FALLING edge (active-low switch to GND)
	EICRA |= (1<<ISC01) | (1<<ISC11);
	EICRA &= ~((1<<ISC00) | (1<<ISC10));

	EIFR  = (1<<INTF0) | (1<<INTF1);  // clear pending
	EIMSK = (1<<INT0)  | (1<<INT1);   // enable

	// Timer for debounce
	debounce_timer_init();
	sei();
}




void execute_path(const Move *path, uint16_t size, float path_scale) {
	for (uint16_t i = 0; i < size / sizeof(Move); i++) {
		if (g_limit_hit) break;
		Move m;
		m.cmd   = pgm_read_byte(&path[i].cmd);
		m.dx_mm = pgm_read_float(&path[i].dx_mm);
		m.dy_mm = pgm_read_float(&path[i].dy_mm);

		// Pen command
		if (m.cmd == 'U') {
			pen_up();
		}
		else if (m.cmd == 'D') {
			pen_down();
		}

		// Apply scaling to movement
		float dx_scaled = m.dx_mm * path_scale;
		float dy_scaled = m.dy_mm * path_scale;

		if (dx_scaled > 0.0f)
		move_x(1, dx_scaled);
		else if (dx_scaled < 0.0f)
		move_x(0, -dx_scaled);

		if (dy_scaled > 0.0f)
		move_y(1, dy_scaled);
		else if (dy_scaled < 0.0f)
		move_y(0, -dy_scaled);

		_delay_ms(10);
	}
	
	
	if (g_limit_hit){
		emergency_stop();	}
}

void execute_path_mirrored(const Move *path, uint16_t size, float path_scale) {
    // Draw original half
    execute_path(path, size, path_scale);

    // Draw mirrored half (vertical reflection)
    for (int16_t i = (size / sizeof(Move)) - 1; i >= 0; i--) {
		if (g_limit_hit) break;
	    Move m;
	    m.cmd   = pgm_read_byte(&path[i].cmd);
	    m.dx_mm = pgm_read_float(&path[i].dx_mm);
	    m.dy_mm = pgm_read_float(&path[i].dy_mm);

	    if (m.cmd == 'U') {
		    pen_up();
			continue;
		    } else if (m.cmd == 'D') {
		    pen_down();
	    }

	    // Mirror across Y-axis (flip DY)
	    float dx_scaled =  m.dx_mm * path_scale;
	    float dy_scaled = -m.dy_mm * path_scale;

	    if (dx_scaled > 0.0f)
	    move_x(1, dx_scaled);
	    else if (dx_scaled < 0.0f)
	    move_x(0, -dx_scaled);

	    if (dy_scaled > 0.0f)
	    move_y(1, dy_scaled);
	    else if (dy_scaled < 0.0f)
	    move_y(0, -dy_scaled);

	    _delay_ms(10);
    }
	
	
	if (g_limit_hit){
		emergency_stop();
	}
}



int main(void) {
	plotter_init();
	limits_init();
	debounce_timer_init();

	while (1) {
		
		execute_path_mirrored(murcielago_path, sizeof(murcielago_path), 3.0f);       
		
		pen_up();
		move_y(0,50);
		
		execute_path(circle_data_path, sizeof(circle_data_path), 0.05f); 
		
		pen_up();
		move_y(0,50);
		
		execute_path(flor_path, sizeof(flor_path), 3.0f);
		
		pen_up();
		move_y(0,500);
	}
}

// ---------------- ISRs ----------------
ISR(INT0_vect){                   // Y_MIN candidate
	// Mask further INT0 edges and start debounce window
	EIMSK &= ~(1<<INT0);
	db0_armed    = 1;
	db0_deadline = g_ms + DEBOUNCE_MS;
}

ISR(INT1_vect){                   // Y_MAX candidate
	EIMSK &= ~(1<<INT1);
	db1_armed    = 1;
	db1_deadline = g_ms + DEBOUNCE_MS;
}
	
ISR(TIMER0_COMPA_vect){
	g_ms++;

	// ---------- INT0 debounce window ----------
	if (db0_armed && (int32_t)(g_ms - db0_deadline) >= 0){
		// Re-sample pin after debounce time
		if (LIMIT_MIN_ACTIVE()){
			g_limit_hit = 1;
			g_limit_src = 0;
		}
		// Re-enable INT0 regardless (validated or false alarm)
		EIMSK |= (1<<INT0);
		db0_armed = 0;
	}

	// ---------- INT1 debounce window ----------
	if (db1_armed && (int32_t)(g_ms - db1_deadline) >= 0){
		if (LIMIT_MAX_ACTIVE()){
			g_limit_hit = 1;
			g_limit_src = 1;
		}
		EIMSK |= (1<<INT1);
		db1_armed = 0;
	}
}
