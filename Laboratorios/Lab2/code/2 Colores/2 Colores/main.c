// ------------------------------------------------------------------
// LIBRARIES
// ------------------------------------------------------------------

#include <avr/io.h>
#include <avr/interrupt.h>

// -----------------------------------------------------------------
// DEFINITIONS
// ------------------------------------------------------------------

#define F_CPU 16000000UL

// USART
#define TX_BUF_SZ 128
#define TX_MASK   (TX_BUF_SZ - 1)
#define RX_BUF_SZ 128
#define RX_MASK   (RX_BUF_SZ - 1)
#define BAUD_RATE 9600



// ------------------------------------------------------------------
// PROGRAM VARIABLES
// ------------------------------------------------------------------

// Color mapping
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


// USART
uint8_t tx_buf[TX_BUF_SZ];
uint8_t tx_head = 0, tx_tail = 0;

uint8_t rx_buf[RX_BUF_SZ];
uint8_t rx_head = 0, rx_tail = 0;

// ------------------------------------------------------------------
// HELPERS
// ------------------------------------------------------------------

// Retorna la cantidad de elementos en el buffer de RX
uint8_t usart_rx_available(void) {
	return (uint8_t)((rx_head - rx_tail) & RX_MASK);
}

// ------------------------------------------------------------------
// INITIALIZERS
// ------------------------------------------------------------------

void usart_init(void) {
	const uint16_t ubrr = (16000000UL / (16UL * BAUD_RATE)) - 1;
	UBRR0H = ubrr >> 8;
	UBRR0L = ubrr;
	UCSR0A = 0;
	UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);   // <- RX interrupt
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);               // 8N1
}


// ------------------------------------------------------------------
// UTILITY
// ------------------------------------------------------------------



uint8_t usart_write_try(uint8_t b) {
	uint8_t next = (uint8_t)((tx_head + 1) & TX_MASK);
	if (next == tx_tail) return 0;               // full
	tx_buf[tx_head] = b;
	tx_head = next;
	UCSR0B |= (1 << UDRIE0);                       // kick the ISR
	return 1;
}

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

// Stores string in 'dest'
uint8_t usart_read_str(char *dest, uint8_t max_len) {
	uint8_t count = 0;
	while (usart_rx_available() && count < (max_len - 1)) {
		uint8_t c;
		usart_read_try(&c);
		if (c == '\n' || c == '\r') {
			break;
		}
		dest[count++] = c;
	}
	
	dest[count] = '\0';
	return count;
}



// ------------------------------------------------------------------
// MAIN
// ------------------------------------------------------------------


int main(void) {
	usart_init();
	sei();
	while (1) {
	
	}
}

// ------------------------------------------------------------------
// ISRs
// ------------------------------------------------------------------

ISR(USART_UDRE_vect) {
	if (tx_head == tx_tail) {
		UCSR0B &= (uint8_t)~_BV(UDRIE0);
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
	
	// uint8_t data = usart_read_try();
	// ... check for command character
}
