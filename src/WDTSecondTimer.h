#ifndef WDT_SECOND_TIMER_H
#define WDT_SECOND_TIMER_H

// Use the WDT to 'tick' at one second intervals.
// the WDT is not calibrated so the time is approximate.

// GvP 2025-09. MIT licence.
// =============================================================================
#include <avr/io.h>         // register and register bit name definitions
#include <avr/interrupt.h>  // cli(), sei(), ISR(WDT_vect)


//Watchdog timer reset macro not in avr/interrupt.h.
#ifndef wdt_reset
#define wdt_reset() __asm__ __volatile__ ("wdr")
#endif

volatile uint8_t WDTSecondsElapsed;

ISR(WDT_vect)
{
    wdt_reset();
    ++WDTSecondsElapsed;
}


struct WDTSecondTimer
{
public:
    WDTSecondTimer() {begin();}
    ~WDTSecondTimer() {end();}

    void begin(void)
    {
        // 1. pause interrupts, reset wd timer internal counter
        // 2. clear interrupt flag by writing 1 to it. WDIF
        // 3. wdt change enable.
        // 4. set prescale bits for 1 sec timeout, and WDIE only, *not* WDE.
        // 5. re-enable interrupts. (needed in arduino for millis().)
        // 6. reset elapsed seconds.
        cli();
        wdt_reset();
        WDTCSR |= (1<<WDIF);
        WDTCSR |= (1<<WDCE) | (1<<WDE);  // enable changing WDE and/or WDP3..0
        // Disable reset, enable interrupt and set prescale to ~ 1 second.
        // We are not using interrupts, but we can't have both WDE (system reset)
        // and WDIE off, so turn on WDIE, because we don't want to reset the
        // Arduino every second, so MUST have WDE off.
        // WDP2..0 == binary 110 = 6 decimal gives a 2^(6+1) * 1024 ish divider,
        // 128K.
        // NOTE: Must set WDCE bit to 0 when changing WDE or WDP2..0
        WDTCSR  = (0<<WDCE) |(0<<WDE) | (1<<WDIE) | (1<< WDP2) | (1<<WDP1);
        sei();
        WDTSecondsElapsed = 0;
    }

    inline uint8_t tick() {return WDTSecondsElapsed;}

    void reset(void)
    {
        wdt_reset();
        WDTSecondsElapsed = 0;
    }

    void end(void)
    {
        cli();
        wdt_reset();
        WDTCSR |= (1<<WDIF);
        WDTCSR |= (1<<WDCE) | (1<<WDE);  // enable changing WDE and/or WDP3..0
        WDTCSR  = 0x00;                   // WDE and WDIE off -> WDT is off.
        sei();
    }

};

#endif
