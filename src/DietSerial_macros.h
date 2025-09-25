#ifndef DIETSERIAL_LIB_MACROS_H
#define DIETSERIAL_LIB_MACROS_H

#include <math.h>  // for using NAN  and isNAN() in use code

// convenience macros for using strings stored in flash.
#ifndef PRINTFLASHSTRING
#define PRINTFLASHSTRING(name, value) \
do { \
    static const char name[] PROGMEM = value; \
    DietSerial.printlnP(name); \
} while (0)

// Can only name a given string once though. So:-
#define REPRINTFLASHSTRING(name) \
do { \
    DietSerial.printlnP(name); \
} while (0)
#endif


// Possibly useful debugging macros:-
#ifndef NDEBUG

#ifndef printReg
#define printReg(r) do { \
DietSerial.print(#r); DietSerial.print('\t'); DietSerial.printBinary(r); \
DietSerial.print("\t0x"); DietSerial.print(r, HEX); \
DietSerial.print('\t'); DietSerial.print(r, DEC); \
DietSerial.println(); \
} while(0)

// printReg(ADCSRA) gives a line:  ADCSRA  B1000 0111    0x87     135

#endif

#ifndef printVar
#define printVar(x) do { \
DietSerial.print(#x); \
DietSerial.print('\t'); \
DietSerial.print(x, DEC); \
DietSerial.print("\t0x"); \
DietSerial.print(x, HEX); \
DietSerial.println(); \
} while(0)
#endif

// int arrowcount = 22; printVar(arrowcount) gives: arrowcount      22     0x16

// printVar works for floating-points format as well as integers.
// Just twice in decimal with 10 and 16 decimals.
// So, printFloatVar (decimal only):

#ifndef printFloatVar
#define printFloatVar(x) do {\
DietSerial.print(#x); \
DietSerial.print('\t'); \
DietSerial.print(x, 6); \
DietSerial.println(); \
} while(0)
#endif

#else
#ifndef printReg
#define printReg
#endif
#ifndef printVar
#define printVar
#endif
#ifndef printFloatVar
#define printFloatVar
#endif
#endif //NDEBUG.

// Arduino F macro.
#ifndef F
#include <avr/pgmspace.h>
class __FlashStringHelper;
#define F(string_literal) (reinterpret_cast<const __FlashStringHelper *>(PSTR(string_literal)))
#endif

//Number printing formats. These are defined in Arduino's "Print.h" header file.
// If that didn't get included we need them here.
#ifndef DEC
#define DEC 10
#endif
#ifndef HEX
#define HEX 16
#endif
#ifndef BIN
#define BIN 2
#endif

#endif
