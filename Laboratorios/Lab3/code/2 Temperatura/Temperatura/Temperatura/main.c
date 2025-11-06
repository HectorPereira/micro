#define F_CPU 16000000UL
#include <avr/io.h>          
#include <util/delay.h>      
#include <avr/interrupt.h>   
#include <string.h>          
#include <stdint.h>          
#include <stdbool.h>         
#include <stdio.h>           

// ----------------------------------
// Definiciones 
// ----------------------------------

#define BAUD 9600UL
#define UBRR_VALUE ((F_CPU/16/BAUD) - 1)
#define TX_BUFFER_SIZE 128
#define RX_BUFFER_SIZE 128
#define UART_TIMEOUT_MS 500

#define precarger 10000

#define VENT_PORT  PORTB
#define VENT_DDR   DDRB
#define VENT_PIN   PB4   // D12 en Arduino UNO

// macro para setear
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
// macro para resetear
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))

#define T1_PRELOAD   (uint16_t)(65536UL - (F_CPU/1024UL))

// ----------------------------------
// Variables 
// ----------------------------------

char string_to_send[4] = ""; 
uint16_t valor = 0;
uint8_t contolar = 0;

volatile char    serialBuffer[TX_BUFFER_SIZE];
volatile uint8_t serialReadPos  = 0;
volatile uint8_t serialWritePos = 0;

volatile char    rxBuffer[RX_BUFFER_SIZE];
volatile uint8_t rxReadPos  = 0;
volatile uint8_t rxWritePos = 0;

// ----------------------------------
// Prototipos 
// ----------------------------------

// Inicialización general
void Init_pwm(void);        // Configura PWM en OC0A (pin D6)
void Init_adc(void);        // Configura el ADC (canal A1)
void init_timer(void);      // Configura el Timer1 para interrupciones cada 1 s
void medicion(void);        // Marca bandera para tomar una medición
void Init_ventilador(void); // Inicializa el pin del ventilador como salida
void Ventilador_on(void);   // Enciende el ventilador
void Ventilador_off(void);  // Apaga el ventilador


// Comunicación UART / USART
void uart_init(unsigned int ubrr);       // Inicializa UART con el valor UBRR indicado
void uart_send(char c);                  // Envía un carácter por UART
void uart_print(const char *s);          // Envía una cadena de texto
void add_string(char *s, char c);


char Chardos(void);                      // Lee un carácter del buffer RX (no bloqueante)
bool usart_readstring(char *dst, uint8_t cap); // Lee una línea de texto hasta '\n'

// Conversión y utilidades
char Number_to_ascii(uint8_t val);       // Convierte un número (0–9) en carácter ASCII
bool ascii_to_u16_switch(const char *s, uint16_t *out); // Convierte texto numérico a entero de 16 bits

// Lectura analógica
uint16_t adc_read_blocking_adif(void);   // Realiza lectura ADC bloqueante (espera ADIF=1)

// Control de temperatura
void enviar_temperatura(uint16_t tmin, uint16_t tmax); // Envía límites de temperatura por UART
void cambiar_rango(uint16_t *tmin, uint16_t *tmax);    // Permite modificar los límites de temperatura
void pedir_u16_linea(const char *prompt, uint16_t *out,
                            uint16_t vmin, uint16_t vmax); // Solicita un valor numérico por UART y valida el rango


// ----------------------------------
// Programa principal
// ----------------------------------

int main(void) {
	uart_init(UBRR_VALUE);

	// Habilita interrupción por recepción UART
	UCSR0B |= (1 << RXCIE0);

	Init_pwm();
	uart_init(UBRR_VALUE);
	Init_adc();
	init_timer();
	sei();                     // Habilita interrupciones globales
	Init_ventilador();
	Ventilador_off();          // Asegura que el ventilador inicie apagado
	
	float tC;                  // Temperatura actual
	uint16_t tC2 = 25;         // Temperatura mínima
	uint16_t tC3 = 30;         // Temperatura máxima
	uint16_t adc = 0;          // Valor ADC leído
	
	OCR0A = 0;                 // PWM inicial en 0

	uart_print("Si quiere prender el sistema escriba Encender\r\n");
	
	char u[64];
	u[0] = '\0';

	while (1) {
		// Espera hasta recibir el comando "Encender"
		while (strcmp(u, "Encender") != 0) {
			usart_readstring(u, 64);
			if (strcmp(u, "Encender") == 0) {
				enviar_temperatura(tC2, tC3);
			}
		}
	    
		// Si se activa la bandera de medición (cada 1 s)
		if (contolar) {
			string_to_send[0] = '\0';
			uint16_t div = 1;

<<<<<<< HEAD
			adc = adc_read_blocking_adif();     // Lee ADC
			tC = (adc * 500.0f) / 1023.0f;      // Convierte a °C (LM35)
			valor = tC;
=======
		if (tC <= (tC2 + (tC2 / 12))) {
			heater_on = 1;              // por debajo del m�nimo -> encender
			Ventilador_off();
			} 
		else if (tC > tC2 && tC < tC3){
			heater_on = 0;
			Ventilador_off();
		}
		else if (tC > (tC3 - (tC3 / 12))) {
			heater_on = 0; 
			Ventilador_on();             // por encima del m�ximo -> apagar
		}
		
		if (heater_on)  OCR0A = 100;
		else            OCR0A = 0;
		
		uint8_t d_t   = (int8_t)tC - (int8_t)(tC3);
		uint8_t pwm2;
		if(d_t > 0){
			
			if (d_t > 20)       pwm2 = 255;
			else if (d_t > 10)  pwm2 = 125;
			else if (d_t > 5)   pwm2 = 50;
			else                pwm2 = 25;
>>>>>>> test

			// Calcula divisor para conversión a caracteres
			while (valor / div >= 10) {
				div *= 10;
			}


			uart_print("\r\n");
			uart_print(string_to_send);
			uart_print("\r\n");

			contolar = 0;

			// Control de temperatura y ventilador
			static uint8_t heater_on = 0;  // 1 = encendido, 0 = apagado

			if (tC <= tC2) {
				heater_on = 1;             // Por debajo del mínimo -> encender
				Ventilador_off();
			} 
			else if (tC >= tC3) {
				heater_on = 0;             // Por encima del máximo -> apagar
				Ventilador_on();
			}

			// Mantiene el último estado dentro del rango [tC2, tC3]
			OCR0A = heater_on ? 100 : 0;

			// Si recibe '1', permite cambiar el rango de temperatura
			char k = Chardos();
			if (k == '1') {
				cambiar_rango(&tC2, &tC3);
				enviar_temperatura(tC2, tC3);
			}

			uint8_t d_t = tC - tC3;

			if(d_t > 0){
				if(d_t > 20){OCR2A = 255;}
				if(d_t > 10){OCR2A = 125;}
				if(d_t > 5){OCR2A = 50;}
				else{OCR2A = 20;}
			}
		}
	}
}


// ----------------------------------
// Funciones
// ----------------------------------

void Init_pwm(void){
    DDRD |= (1 << DDD6);

	// Fast PWM (modo 3, TOP=255), salida no inversora en OC0A
    TCCR0A = (1 << WGM01) | (1 << WGM00) | (1 << COM0A1); // COM0A1=1, COM0A0=0
    TCCR0B = (1 << CS01) | (1 << CS00); // Prescaler = 64  (? 976 Hz @16MHz)

	// Para PB3 (Para el FAN)
	DDRB |= (1 << DDB3);
    TCCR2A = (1 << WGM21) | (1 << WGM20) | (1 << COM2A1);
    TCCR2B = (1 << CS22);
}
void Init_adc(void){
	ADMUX  = 0b01000001;
	ADCSRA = (1 << ADEN)| (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);   // ADPS=111 ? prescaler 128
	DIDR0  = (1 << ADC0D); // Desabilitado la entrada Digital
}
void init_timer(void) {
	cli();                      // Deshabilita interrupciones globales (opcional)
	TCCR1A = 0;                 // Modo normal (WGM13:0 = 0)
	TCCR1B = 0;
	TCNT1  = T1_PRELOAD;        // Carga inicial para un período de ~1 s
	TIMSK1 = (1 << TOIE1);      // Habilita interrupción por desbordamiento
	TCCR1B = (1 << CS12) | (1 << CS10);  // Prescaler = 1024
	// sei();                   // Habilita interrupciones globales (si se desea aquí)
}
void medicion(void){
	contolar = 1;
}
void Init_ventilador(void) {
	sbi(VENT_DDR, VENT_PIN);   // como salida
	sbi(VENT_PORT, VENT_PIN);  // encender (activo en alto)
}
void Ventilador_on(void)  { sbi(VENT_PORT, VENT_PIN); }
void Ventilador_off(void) { cbi(VENT_PORT, VENT_PIN); }



void uart_init(unsigned int ubrr) {
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = (1<<TXEN0) | (1<<RXEN0) | (1<<RXCIE0);
	UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);
}
void uart_send(char c) {
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = c;
}
void uart_print(const char *s) {
	while (*s) uart_send(*s++);
}
void add_string(char *s, char c) {
	while (*s++);
	*(s - 1) = c;
	*s = '\0';
}



char Chardos(void){
	char ret = '\0';

	if (rxReadPos != rxWritePos){
		ret = rxBuffer[rxReadPos];
		rxReadPos++;

		if (rxReadPos >= RX_BUFFER_SIZE){
			rxReadPos = 0;
		}
	}
	
	return ret;
}
bool usart_readstring(char *dst, uint8_t cap) {
	char c = Chardos();          // Toma 1 carácter si hay; '\0' si no
	if (c == '\0') return false; // No llegó nada todavía
	if (c == '\r') return false; // Ignorar retorno de carro
	if (c == '\n') return true;  // Fin de línea

	// Agrega el carácter recibido si aún hay espacio en el buffer
	if (strlen(dst) < cap - 1) {
		add_string(dst, c);
	}

	return false;
}



char Number_to_ascii(uint8_t val){
	switch (val) {
		case 0: return '0';
		case 1: return '1';
		case 2: return '2';
		case 3: return '3';
		case 4: return '4';
		case 5: return '5';
		case 6: return '6';
		case 7: return '7';
		case 8: return '8';
		case 9: return '9';
		
		default: return '?';
	}
}
bool ascii_to_u16_switch(const char *s, uint16_t *out) {
	uint8_t digs[5];   // Hasta 5 dígitos posibles (0..65535)
	uint8_t n = 0;

	// Leer y validar dígitos (detiene en CR o LF)
	for (; *s && *s != '\r' && *s != '\n'; ++s) {
		char c = *s;
		uint8_t d;

		switch (c) {
			case '0': d = 0; break;
			case '1': d = 1; break;
			case '2': d = 2; break;
			case '3': d = 3; break;
			case '4': d = 4; break;
			case '5': d = 5; break;
			case '6': d = 6; break;
			case '7': d = 7; break;
			case '8': d = 8; break;
			case '9': d = 9; break;
			default: return false;  // Carácter no numérico
		}

		if (n >= 5) return false;  // Demasiados dígitos (posible overflow)
		digs[n++] = d;
	}

	if (n == 0) return false;      // Cadena vacía

	// Recombinar los dígitos: unidades, decenas, centenas, etc.
	uint32_t val = 0;
	uint32_t mult = 1;

	for (int8_t i = (int8_t)n - 1; i >= 0; --i) {
		val += (uint32_t)digs[i] * mult;
		mult *= 10;
	}

	if (val > 65535u) return false;  // Límite de uint16_t superado

	*out = (uint16_t)val;
	return true;
}



// Lectura bloqueante esperando ADIF = 1
uint16_t adc_read_blocking_adif(void) {
	ADCSRA |= (1 << ADIF);      // Limpia el flag previo de conversión
	ADCSRA |= (1 << ADSC);      // Inicia una nueva conversión ADC

	while (!(ADCSRA & (1 << ADIF))) {
		// Espera hasta que la conversión finalice (ADIF = 1)
	}

	uint16_t v = ADC;           // Lee el resultado de la conversión
	ADCSRA |= (1 << ADIF);      // Limpia el flag para la próxima conversión
	return v;
}




void enviar_temperatura(uint16_t tmin, uint16_t tmax) {
	char buf[12];

	// Línea 1: palabra de encabezado
	uart_print("temp\r\n");

	// Línea 2: temperatura mínima
	sprintf(buf, "%u", (unsigned)tmin);
	uart_print(buf);
	uart_print("\r\n");

	// Línea 3: temperatura máxima
	sprintf(buf, "%u", (unsigned)tmax);
	uart_print(buf);
	uart_print("\r\n");
}
void cambiar_rango(uint16_t *tmin, uint16_t *tmax) {
	uint16_t a, b;

	uart_print("\r\n--- CAMBIO DE RANGO ---\r\n");

	pedir_u16_linea("Ingrese tC2 (mínimo, entero °C 0..150): ", &a, 0, 150);
	pedir_u16_linea("Ingrese tC3 (máximo, entero °C 0..150): ", &b, 0, 150);

	if (b <= a) {
		uart_print("tC3 debe ser estrictamente mayor que tC2. Operación cancelada.\r\n");
		return;
	}

	*tmin = a;
	*tmax = b;

	uart_print("Rango actualizado.\r\n");
}
void pedir_u16_linea(const char *prompt, uint16_t *out, uint16_t vmin, uint16_t vmax) {
	char buf[8];
	uint16_t v;

	while (1) {
		uart_print(prompt);
		uart_print("\r\n");  // Salto de línea para legibilidad

		buf[0] = '\0';

		// Espera hasta que usart_readstring() complete la línea ('\n')
		while (!usart_readstring(buf, sizeof(buf))) {
			// Espera activa no bloqueante: usart_readstring consume del buffer RX
		}

		// Valida que el valor ingresado sea numérico y esté dentro del rango
		if (ascii_to_u16_switch(buf, &v) && v >= vmin && v <= vmax) {
			*out = v;
			return;
		}

		uart_print("Valor inválido. Intente nuevamente.\r\n");
	}
}




// ----------------------------------
// ISRs
// ----------------------------------

ISR(USART_RX_vect){
	rxBuffer[rxWritePos] = UDR0;
	rxWritePos++;

	if (rxWritePos >= RX_BUFFER_SIZE){
		rxWritePos = 0;
	}
}
ISR(TIMER1_OVF_vect) {
	TCNT1 = T1_PRELOAD;         
	medicion();                 
}
