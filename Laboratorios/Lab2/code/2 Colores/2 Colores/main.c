// ------------------------------------------------------------------
// LIBRARIES
// ------------------------------------------------------------------


#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>

// -----------------------------------------------------------------
// DEFINITIONS-
// ------------------------------------------------------------------

#define TX_BUF_SZ 64
#define TX_MASK   (TX_BUF_SZ - 1)

#define RX_BUF_SZ 64
#define RX_MASK   (RX_BUF_SZ - 1)

#define RED   PB0
#define GREEN PB1
#define BLUE  PB2

#define TIMER_1_PRELOAD  65400

#define NUM_COLORS (sizeof(color_refs)/sizeof(color_refs[0]))



// ------------------------------------------------------------------
// PROGRAM VARIABLES
// ------------------------------------------------------------------

// Color recognition
typedef struct {
	const char *name;
	uint16_t r, g, b;
} ColorRef;

const ColorRef color_refs[] = {
	{"MORADO",    309, 220, 375},
	{"ROJO",       370, 170, 372},
	{"AMARILLO",    322, 152, 164},
	{"VERDE",     414, 276, 221},
	{"AZUL CLARO", 223, 226, 181},
	{"VIOLETA",    345, 356, 434},
	{"BLANCO",    180, 180, 180},
};

uint16_t sample_rgb[3] = {0,0,0};

// USART 
uint8_t tx_buf[TX_BUF_SZ];
uint8_t tx_head = 0, tx_tail = 0;
uint8_t rx_buf[RX_BUF_SZ];
uint8_t rx_head = 0, rx_tail = 0;

// Event handling
uint8_t event_pending = 0;


// Color recordings
uint8_t COLOR_RED[3] = {0,0,0};
uint8_t COLOR_GREEN[3] = {0,0,0};
uint8_t COLOR_BLUE[3] = {0,0,0};
uint8_t COLOR_ORANGE[3] = {0,0,0};
uint8_t COLOR_PURPLE[3] = {0,0,0};
uint8_t COLOR_CYAN[3] = {0,0,0};
uint8_t COLOR_LIGHTBLUE[3] = {0,0,0};
uint8_t COLOR_WHITE[3] = {0,0,0};
uint8_t COLOR_BLACK[3] = {0,0,0};
	
// ADC
uint16_t adc_result = 0;
uint8_t  adc_done   = 0;

// Deboucing
uint8_t debounce_enable = 0;

// LED state
uint8_t led_state = 0;

	

// ------------------------------------------------------------------
// HELPERS
// ------------------------------------------------------------------


uint8_t usart_rx_available(void) {
	return (uint8_t)((rx_head - rx_tail) & RX_MASK);
}

void utoa_simple(uint16_t value, char *buffer) {
	char temp[6];
	int i = 0, j = 0;

	if (value == 0) {
		buffer[0] = '0';
		buffer[1] = '\0';
		return;
	}

	// Convert digits to temp buffer (reversed)
	while (value > 0 && i < sizeof(temp) - 1) {
		temp[i++] = (value % 10) + '0';
		value /= 10;
	}

	// Reverse digits into final buffer
	while (i > 0) buffer[j++] = temp[--i];
	buffer[j] = '\0';
}




// ------------------------------------------------------------------
// INITIALIZERS
// ------------------------------------------------------------------


// Timer 1
void timer1_init(void) {
	TCCR1A = 0;                    // Normal mode
	TCCR1B = (1 << CS12) | (1 << CS10); // Prescaler 1024
	TCNT1  = TIMER_1_PRELOAD;                // Preload for 1s overflow
	TIMSK1 = (1 << TOIE1);         // Enable overflow interrupt
}

void timer1_disable(void){
	TIMSK1 &= ~(1 << TOIE1);         // Enable overflow interrupt
}


// USART
void usart_init(void) {
	const uint16_t ubrr = (16000000UL / (16UL * 9600)) - 1;
	UBRR0H = ubrr >> 8;
	UBRR0L = ubrr;
	UCSR0A = 0;
	UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);   // <- RX interrupt
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);               // 8N1
}

void adc_init(void) {
	ADMUX  = (1 << REFS0);                           // AVcc ref, ADC0 input
	ADCSRA = (1 << ADEN) | (1 << ADIE)               // Enable ADC + its interrupt
	| (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);    // Prescaler 128
}


void eint_init(void) {
	// 1. Configure PD2 and PD3 as inputs
	DDRD &= ~((1 << PD2) | (1 << PD3));

	// 2. Optionally enable internal pull-ups (if using buttons to GND)
	PORTD |= (1 << PD2) | (1 << PD3);

	MCUCR |= (1 << ISC01) | (1 << ISC00)
	| (1 << ISC11) | (1 << ISC10);

	EIMSK |= (1 << INT0) | (1 << INT1);
}

void eint_disable(void) {
	EIMSK &= ~((1 << INT0) | (1 << INT1));   // Clear bits ? disable both
}


// ------------------------------------------------------------------
// UTILITY
// ------------------------------------------------------------------
const char* identify_color(uint16_t r, uint16_t g, uint16_t b) {
	uint32_t best_dist = 0xFFFFFFFF;
	const char *best_name = "UNKNOWN";

	for (uint8_t i = 0; i < NUM_COLORS; i++) {
		int32_t dr = (int32_t)r - color_refs[i].r;
		int32_t dg = (int32_t)g - color_refs[i].g;
		int32_t db = (int32_t)b - color_refs[i].b;
		uint32_t dist = dr*dr + dg*dg + db*db;

		if (dist < best_dist) {
			best_dist = dist;
			best_name = color_refs[i].name;
		}
	}
	return best_name;
}

// --- Update LED color ---
void rgb_set(uint8_t r, uint8_t g, uint8_t b) {
	PORTB = (PORTB & ~((1 << RED)|(1 << GREEN)|(1 << BLUE))) |
	((r<<RED) | (g<<GREEN) | (b<<BLUE));
}


uint8_t usart_write_try(uint8_t b) {
	uint8_t next = (uint8_t)((tx_head + 1) & TX_MASK);
	if (next == tx_tail) return 0;               // full
	tx_buf[tx_head] = b;
	tx_head = next;
	UCSR0B |= (1 << UDRIE0);                       // kick the ISR
	return 1;
}

// Non-blocking: queues as many chars as fit, returns how many were queued.
uint16_t usart_write_str(const char *s) {
	uint16_t n = 0;
	while (*s && usart_write_try((uint8_t)*s++)) n++;
	return n;
}


uint8_t usart_read_try(uint8_t *b) {
	if (rx_head == rx_tail) return 0;                 // empty
	*b = rx_buf[rx_tail];
	rx_tail = (uint8_t)((rx_tail + 1) & RX_MASK);
	return 1;
}

uint16_t adc_read(uint8_t channel) {
	ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);  // Select ADC channel 0–7
	ADCSRA |= (1 << ADSC);                      // Start conversion
	while (ADCSRA & (1 << ADSC));               // Wait for conversion to finish
	return ADC;                                 // Return 10-bit result
}

void cycle_led(void){
	led_state = (led_state + 1) % 3;
	switch (led_state) {
		case 0: rgb_set(1,0,0); break; // Red
		case 1: rgb_set(0,1,0); break; // Green
		case 2: rgb_set(0,0,1); break; // Blue
	}
}



// ------------------------------------------------------------------
// MAIN
// ------------------------------------------------------------------


int main(void) {
	usart_init();
	adc_init();
	eint_init();
	DDRB |= (1 << PB0) | (1 << PB1) | (1 << PB2);  // RGB pins out
	sei();                                          // global interrupts ON


	while (1) {
		if (event_pending) {
			uint16_t value = adc_read(0);
			sample_rgb[led_state] = value;  // store per LED color


			if (led_state==2) {
				// all 3 have been measured, now identify:
				const char *name = identify_color(sample_rgb[0],
				sample_rgb[1],
				sample_rgb[2]);
				usart_write_str("Color: ");
				usart_write_str(name);
				usart_write_str("\r\n");
			}

			event_pending = 0;
		}
	}
}

// ------------------------------------------------------------------
// ISRs
// ------------------------------------------------------------------

// USART
ISR(USART_UDRE_vect) {
	if (tx_head == tx_tail) {
		UCSR0B &= (uint8_t)~(1 << UDRIE0);
		return;
	}
	UDR0 = tx_buf[tx_tail];
	tx_tail = (uint8_t)((tx_tail + 1) & TX_MASK);
}

ISR(USART_RX_vect) {
	uint8_t d = UDR0;
	uint8_t next = (uint8_t)((rx_head + 1) & RX_MASK);
	if (next != rx_tail) {
		rx_buf[rx_head] = d;
		rx_head = next;
	}
}


// --- Cycle through colors ---
ISR(TIMER1_OVF_vect) {
	if (!adc_done) {             // Only start if not already running
		ADCSRA |= (1 << ADSC);
	}

	TCNT1 = TIMER_1_PRELOAD;     
	if (debounce_enable) {
		eint_init();
		debounce_enable = 0;
	}

	event_pending = 1;
	timer1_disable();
}

// ADC
ISR(ADC_vect) {
	adc_result = ADC;      // read 10-bit result
	adc_done = 1;          // flag that data is ready
}

ISR(INT0_vect) {
	cycle_led();
	eint_disable();
	debounce_enable = 1;
	timer1_init();
}

ISR(INT1_vect) {
	// Code to run when INT1 (PD3) is triggered
	eint_disable();
	debounce_enable = 1;
	timer1_init();
}