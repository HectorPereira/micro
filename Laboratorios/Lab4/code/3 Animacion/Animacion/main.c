#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>

// ------------------------------------------------------------------
// USART - Recepción Asíncrona
// ------------------------------------------------------------------
volatile uint8_t uart_rx = 0;
volatile uint8_t new_data = 0;

void uart_init(uint16_t baud) {
	uint16_t ubrr = (F_CPU / (16UL * baud)) - 1;

	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)(ubrr & 0xFF);

	UCSR0B = (1 << RXEN0) | (1 << RXCIE0);  // RX + Interrupt
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8N1
}

ISR(USART_RX_vect) {
	uart_rx = UDR0;
	new_data = 1;
}



// ------------------------------------------------------------------
// Configuración general
// ------------------------------------------------------------------

#define LED_PIN PORTD6
#define LED_DDR DDRD
#define NUM_LEDS 256

uint8_t leds[NUM_LEDS * 3];  // Datos GRB para cada LED

// ------------------------------------------------------------------
// Prototipos
// ------------------------------------------------------------------

void ws2812_init(void);
void ws2812_send_pixel(uint8_t r, uint8_t g, uint8_t b);
void ws2812_show(void);
void ws2812_fill(uint8_t r, uint8_t g, uint8_t b, uint16_t n);
void ws2812_set_pixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void ws2812_show_all(void);
void ws2812_clear(void);

void send_bit(uint8_t bitVal);
void send_byte(uint8_t byte);
void turn_led(uint8_t led_x, uint8_t led_y);

uint8_t wheel_color(uint8_t pos, uint8_t color);
void anim_rainbow_diagonal(void);
void anim_barber_pole(void);
void draw_raibow_mask(void);



	


// ------------------------------------------------------------------
// MAIN
// ------------------------------------------------------------------
int main(void){
	ws2812_init();
	uart_init(9600);
	sei();                 // habilitar interrupciones globales

	ws2812_show();         // reset inicial
	
	
	while (1)
	{
		if (new_data) {
			new_data = 0;
	
			if (uart_rx == '1') {
				anim_barber_pole();
			}
			else if (uart_rx == '2') {
				anim_rainbow_diagonal();
			}
		}
	}
}


// ------------------------------------------------------------------
// FUNCIONES WS2812
// ------------------------------------------------------------------


uint16_t serpentine_index(uint8_t x, uint8_t y) {
	if (y % 2 == 0) {
		return y * 16 + x;         // fila normal
		} else {
		return y * 16 + (15 - x);  // fila invertida
	}
}

uint8_t wheel_color(uint8_t pos, uint8_t color) {
	if (pos < 85)
	return (pos * 3 * (color == 0)) + ((255 - pos*3) * (color == 2));
	else if (pos < 170)
	return ((pos-85) * 3 * (color == 1)) + ((255 - (pos-85)*3) * (color == 0));
	else
	return ((pos-170) * 3 * (color == 2)) + ((255 - (pos-170)*3) * (color == 1));
}

void anim_rainbow_diagonal(void) {
	while (1)
	{
		for(uint16_t j = 0; j < 255; j++){
			for(uint8_t y = 0; y < 16; y++){
				for(uint8_t x = 0; x < 16; x++){
					if (new_data) return;
					uint16_t index = serpentine_index(15-y, x);

					// --- MUCHOS COLORES EN PANTALLA ---
					uint8_t pos = (x * 9 + y * 6 + j * 3) % 255;

					uint8_t r = wheel_color(pos, 0);
					uint8_t g = wheel_color(pos, 1);
					uint8_t b = wheel_color(pos, 2);

					ws2812_set_pixel(index, r, g, b);
				}
			}
			draw_raibow_mask();
			ws2812_show_all();
		}
	}
}

void draw_raibow_mask(void) {
	const uint8_t r = 0;
	const uint8_t g = 0;
	const uint8_t b = 0;
	
	for (int8_t x = 0; x < 10; x++) {
		ws2812_set_pixel(serpentine_index(x, 0), r, g, b);
	}
	for (int8_t x = 0; x < 7; x++) {
		ws2812_set_pixel(serpentine_index(x, 1), r, g, b);
	}
	for (int8_t x = 0; x < 5; x++) {
		ws2812_set_pixel(serpentine_index(x, 2), r, g, b);
	}
	for (int8_t x = 0; x < 4; x++) {
		ws2812_set_pixel(serpentine_index(x, 3), r, g, b);
	}
	for (int8_t x = 0; x < 3; x++) {
		ws2812_set_pixel(serpentine_index(x, 4), r, g, b);
	}
	for (int8_t x = 0; x < 2; x++) {
		ws2812_set_pixel(serpentine_index(x, 5), r, g, b);
	}
	for (int8_t x = 0; x < 2; x++) {
		ws2812_set_pixel(serpentine_index(x, 6), r, g, b);
	}
	for (int8_t x = 0; x < 1; x++) {
		ws2812_set_pixel(serpentine_index(x, 7), r, g, b);
	}
	for (int8_t x = 0; x < 1; x++) {
		ws2812_set_pixel(serpentine_index(x, 8), r, g, b);
	}
	for (int8_t x = 0; x < 1; x++) {
		ws2812_set_pixel(serpentine_index(x, 9), r, g, b);
	}
	
	for (int8_t x = 0; x < 3; x++) {
		ws2812_set_pixel(serpentine_index(15-x, 7), r, g, b);
	}
	for (int8_t x = 0; x < 5; x++) {
		ws2812_set_pixel(serpentine_index(15-x, 8), r, g, b);
	}
	for (int8_t x = 0; x < 6; x++) {
		ws2812_set_pixel(serpentine_index(15-x, 9), r, g, b);
	}
	for (int8_t x = 0; x < 7; x++) {
		ws2812_set_pixel(serpentine_index(15-x, 10), r, g, b);
	}
	for (int8_t x = 0; x < 7; x++) {
		ws2812_set_pixel(serpentine_index(15-x, 11), r, g, b);
	}
	for (int8_t x = 0; x < 8; x++) {
		ws2812_set_pixel(serpentine_index(15-x, 12), r, g, b);
	}
	for (int8_t x = 0; x < 8; x++) {
		ws2812_set_pixel(serpentine_index(15-x, 13), r, g, b);
	}
	for (int8_t x = 0; x < 8; x++) {
		ws2812_set_pixel(serpentine_index(15-x, 14), r, g, b);
	}
	for (int8_t x = 0; x < 8; x++) {
		ws2812_set_pixel(serpentine_index(15-x, 15), r, g, b);
	}
}



void draw_yellow_frame(int8_t radius) {

	const uint8_t r = 255;
	const uint8_t g = 255;
	const uint8_t b = 0;

	int8_t x_start = 8 - radius;
	int8_t x_end   = 8 + radius;
	
	
	// Curvatura exterior
	
	for (int8_t x = x_start; x < x_start + radius*2+1; x++) {
		ws2812_set_pixel(serpentine_index(x, 1), r, g, b);
	}
	
	for (int8_t x = x_start+2; x < x_start + radius*2-1; x++) {
		ws2812_set_pixel(serpentine_index(x, 0), r, g, b);
	}
	
	for (int8_t x = x_start; x < x_start + radius*2+1; x++) {
		ws2812_set_pixel(serpentine_index(x, 14), r, g, b);
	}
	
	for (int8_t x = x_start+2; x < x_start + radius*2-1; x++) {
		ws2812_set_pixel(serpentine_index(x, 15), r, g, b);
	}
	
	
	// --- Fila superior (y = 0) ---
	for (int8_t x = x_start; x < x_start + 2; x++) {
		if (x >= 0 && x < 16)
		ws2812_set_pixel(serpentine_index(x, 2), r, g, b);
	}
	for (int8_t x = x_end - 1; x <= x_end; x++) {
		if (x >= 0 && x < 16)
		ws2812_set_pixel(serpentine_index(x, 2), r, g, b);
	}
	
	// --- Fila superior (y = 0) ---
	for (int8_t x = x_start; x < x_start + 2; x++) {
		if (x >= 0 && x < 16)
		ws2812_set_pixel(serpentine_index(x, 0), 0, 0, 0);
	}
	for (int8_t x = x_end - 1; x <= x_end; x++) {
		if (x >= 0 && x < 16)
		ws2812_set_pixel(serpentine_index(x, 0), 0, 0, 0);
	}

	// --- Fila inferior (y = 15) ---
	for (int8_t x = x_start; x < x_start + 2; x++) {
		if (x >= 0 && x < 16)
		ws2812_set_pixel(serpentine_index(x, 13), r, g, b);
	}
	for (int8_t x = x_end - 1; x <= x_end; x++) {
		if (x >= 0 && x < 16)
		ws2812_set_pixel(serpentine_index(x, 13), r, g, b);
	}
	
	for (int8_t x = x_start; x < x_start + 2; x++) {
		if (x >= 0 && x < 16)
		ws2812_set_pixel(serpentine_index(x, 15), 0, 0, 0);
	}
	for (int8_t x = x_end - 1; x <= x_end; x++) {
		if (x >= 0 && x < 16)
		ws2812_set_pixel(serpentine_index(x, 15), 0, 0, 0);
	}
}




void anim_barber_pole(void) {

	const int8_t radius = 3;
	const uint8_t stripe_width = 9;
	const uint8_t period = stripe_width * 3;

	uint16_t t = 0;

	while (1) {
		if (new_data) return;
		ws2812_clear();

		for (uint8_t y = 0; y < 16; y++) {
			for (uint8_t x = 0; x < 16; x++) {

				int8_t dx = x - 8;
				if (dx < -radius || dx > radius) {
					continue;
				}

				int8_t adx = dx < 0 ? -dx : dx;
				int8_t dist = radius - adx;
				if (dist < 0) dist = 0;

				uint8_t base = (uint8_t)(40 + dist * 35);
				if (base > 255) base = 255;

				int16_t phase = (int16_t)(dx * 2 + y * 3 + (t / 2));
				while (phase < 0) phase += period;
				uint8_t p = (uint8_t)(phase % period);

				uint8_t r, g, b;

				if (p < stripe_width) {
					r = base;
					g = base / 10;
					b = base / 10;
				}
				else if (p < 2 * stripe_width) {
					r = base;
					g = base;
					b = base;
				}
				else {
					r = base / 10;
					g = base / 5;
					b = base;
				}

				ws2812_set_pixel(serpentine_index(x, y), r, g, b);
			}
		}

		// ??? Marco ajustado al ancho del cilindro ???
		draw_yellow_frame(radius);

		ws2812_show_all();
		t++;
		_delay_ms(30);
	}
}






void ws2812_init(void) {
	LED_DDR |= (1 << LED_PIN); // Configura pin de salida
}

void send_bit(uint8_t bitVal) {
	if (bitVal) {
		PORTD |= (1 << LED_PIN);
		asm volatile (
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t");
		PORTD &= ~(1 << LED_PIN);
		asm volatile ("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
		} else {
		PORTD |= (1 << LED_PIN);
		asm volatile ("nop\n\t""nop\n\t""nop\n\t");
		PORTD &= ~(1 << LED_PIN);
		asm volatile (
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
	}
}


void send_byte(uint8_t byte) {
	cli();
	for (uint8_t i = 0; i < 8; i++) {
		send_bit(byte & 0x80);
		byte <<= 1;
	}
	sei();
}

void ws2812_send_pixel(uint8_t r, uint8_t g, uint8_t b) {
	send_byte(g);
	send_byte(r);
	send_byte(b);
}

void ws2812_show(void) {
	_delay_us(30);  // tiempo de reset (>50us)
}

void ws2812_fill(uint8_t r, uint8_t g, uint8_t b, uint16_t n) {
	cli();
	for (uint16_t i = 0; i < n; i++) {
		ws2812_send_pixel(r, g, b);
	}
	sei();
	ws2812_show();
}

void ws2812_set_pixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
	if (index >= NUM_LEDS) return;
	leds[index * 3 + 0] = g;
	leds[index * 3 + 1] = r;
	leds[index * 3 + 2] = b;
}

void ws2812_show_all(void) {
	cli();
	for (uint16_t i = 0; i < NUM_LEDS; i++) {
		ws2812_send_pixel(leds[i * 3 + 1], leds[i * 3 + 0], leds[i * 3 + 2]);
	}
	sei();
	ws2812_show();
}

void ws2812_clear(void) {
	for (uint16_t i = 0; i < NUM_LEDS * 3; i++)
	leds[i] = 0;
}

// Encender LED en coordenadas (x, y)
void turn_led(uint8_t led_x, uint8_t led_y) {
	uint8_t index = led_y * 8 + led_x;
	ws2812_clear();
	ws2812_set_pixel(index, 0, 0, 0); // rojo
	ws2812_show_all();
}

