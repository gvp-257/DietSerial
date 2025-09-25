
#include <Arduino.h>
// Memory comparison between DietSerial and Arduino Serial.
// Compile this example in the Arduino IDE (Ctrl-R) and note the program size
// and SRAM usage.
//
// Then comment out the USE_DIETSERIAL 1 line, and uncomment the one below it
// to use Serial instead. Compile again and note the program memory and
// SRAM usage.


//#define USE_DIETSERIAL 1
#define USE_DIETSERIAL 0   // For serial

#if defined(USE_DIETSERIAL)

#if USE_DIETSERIAL
#include "DietSerial.h"
#define SENDER DietSerial

#else  // USE_DIETSERIAL defined as 0 (false)
#define SENDER Serial
#endif // USE_DIETSERIAL == 1

#else  // USE_DIETSERIAL not defined: assume Serial.
#define SENDER Serial
#endif // defined(USE_DIETSERIAL)

#ifndef PRINTFLASHSTRING
#define PRINTFLASHSTRING(x,TEXT) \
do { \
    Serial.println(F(TEXT)); \
} while (0);
#endif

void setup() {
    SENDER.begin(9600);


    PRINTFLASHSTRING(aTestFlashString,"This is a string stored in Flash.");

    SENDER.println();


    PRINTFLASHSTRING(floatString,"Printing a floating point number -1.2345678 with 4 decimal places, and with 7:");

    float floatNumber = -1.2345678;
    SENDER.println(floatNumber);      // printing floats defaults to 4 decimal places
    SENDER.println(floatNumber, 7);   // second parameter is number of decimal places
    SENDER.println();

    // Printing integer type numbers:
    PRINTFLASHSTRING(integerString, "Printing an integer in decimal, hexadecimal and binary:");

    int integer = 21400;
    SENDER.println(integer);   // default format is decimal
    SENDER.println(integer, HEX);
    SENDER.println(integer, BIN);  // binary
    SENDER.println();


    // printing a boolean
    PRINTFLASHSTRING(boolString, "Printing a boolean variable, set to true:");
    bool b = true;
    SENDER.println(b);   // "true"

    for (auto i = 0; i < 60; i++) {SENDER.print('=');}
    SENDER.println(); SENDER.println();

    PRINTFLASHSTRING(InputPrompt, "Enter up to 17 characters: ");
#if USE_DIETSERIAL
    char buffer[20];
    for (auto& c : buffer)  c = 0;   // zero out the buffer

    size_t numberOfChars = DietSerial.readString(buffer, sizeof(buffer));
    if ((numberOfChars > 0) && !(DietSerial.error()))
    {
        DietSerial.print(F("You entered: "));
        DietSerial.println(buffer);
    }
    else
    if (DietSerial.error())
    {
        DietSerial.printError(DietSerial.error());
    }
    else // no error and no text
    {
        DietSerial.println(F("No text was received."));
    }
#else
    Serial.setTimeout(20000);   // default is 1000 milliseconds, need longer.
    String s = Serial.readString();
    Serial.print(F("You entered: "));
    Serial.println(s);
#endif
    PRINTFLASHSTRING(endoftest,"That concludes the memory comparison example for DietSerial.");

    SENDER.flush();
}

void loop() {}
