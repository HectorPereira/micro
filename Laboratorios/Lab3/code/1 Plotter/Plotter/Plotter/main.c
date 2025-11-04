#define F_CPU 1000000UL
#include <avr/io.h>
#include <util/delay_basic.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

typedef struct {
	char cmd;       // 'U' = levantar lápiz, 'D' = bajar lápiz
	float dx_mm;    // desplazamiento en X (mm)
	float dy_mm;    // desplazamiento en Y (mm)
} Move;

// Pines de los finales de carrera
#define LIMIT_Y_MIN   PD2
#define LIMIT_Y_MAX   PD3

#define LIMIT_MIN_ACTIVE()   ((PIND & (1<<LIMIT_Y_MIN)) == 0)
#define LIMIT_MAX_ACTIVE()   ((PIND & (1<<LIMIT_Y_MAX)) == 0)

#define DEBOUNCE_MS  15  // tiempo antirrebote


// Variables globales
volatile uint8_t  g_limit_hit = 0;     // se activa si se toca un límite
volatile uint8_t  g_limit_src = 0xFF;  // guarda cuál límite se activó
volatile uint32_t g_ms = 0;            // contador de milisegundos

volatile uint8_t  db0_armed = 0, db1_armed = 0;   // banderas de antirrebote
volatile uint32_t db0_deadline = 0, db1_deadline = 0;

#include "path_data.h"


// Pines de motores y solenoide
#define STEP_X PB3
#define DIR_X  PB4
#define EN_X   PB5
#define STEP_Y PC3
#define DIR_Y  PC4
#define EN_Y   PC5
#define STEP_SCALE 18.46f
#define SOLENOID PC0

// Pequeña función de retardo por ciclos
static inline void delay_cycles(uint16_t n){ _delay_loop_2(n); }

// Inicialización de pines del plotter
void plotter_init(void) {
	DDRB |= (1<<STEP_X)|(1<<DIR_X)|(1<<EN_X);
	DDRC |= (1<<STEP_Y)|(1<<DIR_Y)|(1<<EN_Y)|(1<<SOLENOID);
	PORTB |= (1<<EN_X);
	PORTC |= (1<<EN_Y);
	PORTC &= ~(1<<SOLENOID);
}

// Lápiz arriba
void pen_up(void) {
	PORTC |= (1<<SOLENOID);
}

// Lápiz abajo
void pen_down(void) {
	PORTC &= ~(1<<SOLENOID);
}

// Movimiento básico de un eje
void move_axis(volatile uint8_t *port_dir, uint8_t dir_bit,
volatile uint8_t *port_step, uint8_t step_bit,
uint8_t direction, uint16_t steps)
{
	if (g_limit_hit) return; // detener si hay emergencia

	if(direction) *port_dir |=  (1<<dir_bit);
	else           *port_dir &= ~(1<<dir_bit);
	delay_cycles(1);

	for (uint16_t i = 0; i < steps; i++) {
		if (g_limit_hit) break;
		*port_step |=  (1<<step_bit);
		delay_cycles(300);
		*port_step &= ~(1<<step_bit);
		delay_cycles(300);
	}
}

// Movimiento en eje X
void move_x(uint8_t dir, float dist_mm) {
	uint16_t steps = (uint16_t)(dist_mm * STEP_SCALE);
	move_axis(&PORTB, DIR_X, &PORTB, STEP_X, dir, steps);
}

// Movimiento en eje Y
void move_y(uint8_t dir, float dist_mm) {
	uint16_t steps = (uint16_t)(dist_mm * STEP_SCALE);
	move_axis(&PORTC, DIR_Y, &PORTC, STEP_Y, dir, steps);
}

// Apagado de emergencia
static inline void emergency_stop(void){
	pen_up();
	PORTB &= ~(1<<EN_X);
	PORTC &= ~(1<<EN_Y);
}

// Inicializa timer para antirrebote
static void debounce_timer_init(void){
	TCCR0A = (1<<WGM01);
	TCCR0B = 0;
	OCR0A  = 249;
	TIMSK0 = (1<<OCIE0A);
	TCCR0B = (1<<CS01) | (1<<CS00);
}


// Configura interrupciones de los finales de carrera
static void limits_init(void){
	DDRD  &= ~((1<<LIMIT_Y_MIN) | (1<<LIMIT_Y_MAX));
	PORTD |=  ((1<<LIMIT_Y_MIN) | (1<<LIMIT_Y_MAX));
	EICRA |= (1<<ISC01) | (1<<ISC11);
	EICRA &= ~((1<<ISC00) | (1<<ISC10));
	EIFR  = (1<<INTF0) | (1<<INTF1);
	EIMSK = (1<<INT0)  | (1<<INT1);
	debounce_timer_init();
	sei();
}


// Ejecuta un camino normal
void execute_path(const Move *path, uint16_t size, float path_scale) {
	for (uint16_t i = 0; i < size / sizeof(Move); i++) {
		if (g_limit_hit) break;
		Move m;
		m.cmd   = pgm_read_byte(&path[i].cmd);
		m.dx_mm = pgm_read_float(&path[i].dx_mm);
		m.dy_mm = pgm_read_float(&path[i].dy_mm);
		if (m.cmd == 'U') {
			pen_up();
		}
		else if (m.cmd == 'D') {
			pen_down();
		}
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
		emergency_stop();
	}
}

// Ejecuta un camino y luego su reflejo vertical
void execute_path_mirrored(const Move *path, uint16_t size, float path_scale) {
	execute_path(path, size, path_scale);
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
		move_x(0,40);
		execute_path(circle_data_path, sizeof(circle_data_path), 0.04f);
		
		pen_up();
		move_y(1,20);
		move_x(0,40);
		execute_path(flor_path, sizeof(flor_path), 3.0f);
		
		pen_up();
		move_x(1,90);
		move_y(1,60);
		execute_path(cross_data_path, sizeof(cross_data_path), 0.5f);
		
		pen_up();
		move_x(0,40);
		execute_path(triangle_data_path, sizeof(triangle_data_path), 0.5f);
		
		pen_up();
		move_x(0,500);
	}
}
// Interrupción por límite inferior
ISR(INT0_vect){
	EIMSK &= ~(1<<INT0);
	db0_armed    = 1;
	db0_deadline = g_ms + DEBOUNCE_MS;
}

// Interrupción por límite superior
ISR(INT1_vect){
	EIMSK &= ~(1<<INT1);
	db1_armed    = 1;
	db1_deadline = g_ms + DEBOUNCE_MS;
}

// Interrupción del timer para antirrebote
ISR(TIMER0_COMPA_vect){
	g_ms++;
	if (db0_armed && (int32_t)(g_ms - db0_deadline) >= 0){
		if (LIMIT_MIN_ACTIVE()){
			g_limit_hit = 1;
			g_limit_src = 0;
		}
		EIMSK |= (1<<INT0);
		db0_armed = 0;
	}
	if (db1_armed && (int32_t)(g_ms - db1_deadline) >= 0){
		if (LIMIT_MAX_ACTIVE()){
			g_limit_hit = 1;
			g_limit_src = 1;
		}
		EIMSK |= (1<<INT1);
		db1_armed = 0;
	}
}
