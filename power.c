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
volatile long last_interruption = 0;
volatile long begining_sleep = 0;
volatile long sleeping = 0;

long millis() {
    //printf("TCNT2 %d counter %ld \n", TCNT2, t2_counter);
    return t2_counter + ((TCNT2 * 16) / 250) ;
}

void delay(int amount) {
    long begin_delay = millis();
    while((amount + begin_delay) < millis());
}

void buttons_init() {
    // Setup button 1
    *DDRd = ~(1 << DDD2);           // pin2 as input
    EICRA |= (1 << ISC00);         // set INT0 to trigger on ANY logic change
    EIMSK |= (1 << INT0);          // Turns on INT0

}

ISR (INT0_vect)
{
  long begin_timer2 = millis();
  if ((begin_timer2 - last_interruption) >= 200) {
        printf("Time since boot %ld \n", begin_timer2);
        active_time = begin_timer2 + (begin_timer2 - begining_sleep);
        printf("beging %ld  begining_sleep %ld active_time %ld TCNT2 %d \n", begin_timer2, begining_sleep, active_time, TCNT2);
        long duty_cicle = (float)(active_time / begin_timer2) * 100 ;
        printf("Estimated duty cicle %ld \n", duty_cicle);
        delay(20);
    }
    last_interruption = millis();
}

void timer2_init()
{
#ifdef DEBUG_TIMER2
    //printf("timer2_init \n");
#endif

    // set up timer with prescaler = 1024 and CTC mode
    TCCR2B = /*(1 << WGM12) |*/ (1<<CS22) | (1<<CS21) | (1<<CS20);

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
//#ifdef DEBUG_TIMER1
    //printf("TIMER2 \n");
//#endif
    t2_counter = t2_counter + 16;
    sleeping += 16;
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

void sleep_cpu() {
    begining_sleep = millis();
    TCNT2 = 0;
    sleeping = 0;
    asm("sleep");     // Make processor sleep
    SMCR |= (1 << SE); // Enable sleep mode
}

int main (void)
{
    // Initialize UART
    uart_init();
    stdout = &uart_output;
    stdin  = &uart_input;

    buttons_init();

    timer2_init();

    //Enable global interruptions
    sei();

    while (1) {
        //printf("While\n");
        set_sleep_mode(POWER_SAVE_MODE);
 	      sleep_cpu();
    }

    return 0;
}
