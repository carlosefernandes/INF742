#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include "simple_usart/uart.h"

#define IDLE_MODE 0
#define POWER_DOWN_MODE 1
#define POWER_SAVE_MODE 2
#define STANDBY_MODE 3
#define EXT_STANDBY_MODE 4

volatile char* DDRd = (char*)0x02A;
volatile char* PORTd = (char*)0x02B;
volatile char* PINd = (char*)0x029;

volatile long t2_counter = 0;
volatile long active_time = 0;
volatile long begining_sleep = 0;
volatile long sleeping = 0;
volatile long begin_delay = 0;
volatile long delay_size = 250;

long millis() {
    return t2_counter + ((TCNT2 * 16) / 250) ;
}

void delay() {
    begin_delay = millis();
}

void buttons_init() {
    // Setup button 1
    *DDRd = ~(1 << DDD2);           // pin2 as input
    EICRA |= (1 << ISC01);         // set INT0 to trigger on falling logic change
    EIMSK |= (1 << INT0);          // Turns on INT0

}

ISR (INT0_vect)
{
  long begin_timer2 = millis();
  if (begining_sleep > 0) {
	   sleeping += (begin_timer2 - begining_sleep);
  }
  begining_sleep = 0;

  if (begin_delay == 0) {
        printf("\nTime since boot %ld", begin_timer2);
        active_time = (begin_timer2 - sleeping);
        unsigned long  duty_cicle = (active_time*100)/begin_timer2;
        printf("\nActive time %ld", active_time);
        printf("\nEstimated duty cicle %d%%.", duty_cicle);
        delay();
    }
}

void timer2_init()
{
    // set up timer with prescaler = 1024 and CTC mode
    TCCR2B = (1 << WGM12) | (1<<CS22) | (1<<CS21) | (1<<CS20);

    // initialize counter
    TCNT2 = 0;

    // interruption for each 16 milliseconds
    OCR2A = 0xF9;

    // enable compare interrupt
    TIMSK2 |= (1 << OCIE2A);

    sei();
}

ISR (TIMER2_COMPA_vect)
{

    t2_counter = t2_counter + 16;
    long current_time = t2_counter + ((TCNT2 * 16)/250);
    // begining_sleep > 0 means that processor is waking
    if (begining_sleep > 0) {
		    sleeping += (current_time - begining_sleep);
	  }
	  begining_sleep = 0;

    if (current_time > (begin_delay + delay_size)) {
       begin_delay = 0;
    }
}

void sleep_cpu() {
  // Do not sleep if handling debounce
  if (begin_delay == 0) {
      // begining_sleep == 0 means that timer2 and button interruptions were
      // were handled and processor can sleep
      if (begining_sleep == 0) {
	       begining_sleep = millis();
       }

      SMCR |= (1 << SE); // Enable sleep mode
      asm("sleep");      // Make processor sleep
  }
}

void set_sleep_mode(char mode) {
    switch (mode) {
      case IDLE_MODE:
            SMCR &= ~(1 << SM0);
            SMCR &= ~(1 << SM1);
            SMCR &= ~(1 << SM2);
	          break;
      case POWER_DOWN_MODE:
            SMCR &= ~(1 << SM0);
            SMCR |= (1<<SM1);
	          break;
	    case POWER_SAVE_MODE:
            SMCR |= (1<<SM1) | (1<<SM0);
	          break;
	case STANDBY_MODE:
            SMCR &= ~(1 << SM0);
            SMCR |= (1<<SM2) | (1<<SM1);
	    break;
	case EXT_STANDBY_MODE:
            SMCR |= (1<<SM2) | (1<<SM1) | (1<<SM0);
	    break;
    }
}

int main (void)
{
    timer2_init();

    // Initialize UART
    uart_init();
    stdout = &uart_output;
    stdin  = &uart_input;

    buttons_init();

    //Enable global interruptions
    sei();

    while (1) {
        set_sleep_mode(POWER_SAVE_MODE);
 	      sleep_cpu();
    }

    return 0;
}
