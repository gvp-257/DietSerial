#include <Arduino.h>
// ============================================================================
// Simple Usage: Using DietSerial like Arduino Serial.
// Shows "print" and "write" functions, and the "printBinary" function
// Then shows receiving a string with "readString", and a floating-point number
// with "parseFloat".
// ============================================================================
#include "DietSerial.h"

// ============================================================================
// Error and diagnostic messages
// ============================================================================
static const char ErrorHeader[] PROGMEM =
    "Error occurred in a DietSerial receive function: ";
static const char ErrorNoData[] PROGMEM =
    "No data was received, other than CR and/or LF.";
static const char ErrorNoFloat[] PROGMEM =
    "No floating-point number was detected in what you typed.";

// ============================================================================
//    Main program
// ============================================================================

// Saving 9 bytes of RAM:
// If you do not need any of millis(), micros(), delay(), tone(), analogWrite()
// or delayMicroseconds(), you can replace setup() and loop() with main() as
// follows:-

////void setup() {
int main(void) {

    DietSerial.begin(); // begin() defaults to 9600.

    DietSerial.print(F("Free RAM before DietSerial.begin(): "));

    // Print strings (named and un-named) stored in RAM, with a newline after it.

    // Uncomment this line and see the increase in ram usage after compiling.
    //PrintStringFromRAM();


    PrintStringFromFlash();

    PrintNamedFlashString();

    DietSerial.println(F("Now some other data types besides strings.:-\n"));

    PrintBoolean();

    PrintByteAsFixedLengthBinary();

    PrintInteger();
    PrintFloat();

    // Binary writes
    Write4BytesFromRAM();
    DietSerial.println(); DietSerial.println();

    // A block of 8 bytes stored in flash memory.
    WriteBlockOfBytesFromFlash();

    //===============================================================
    // Reading from serial monitor.
    // The Arduino IDE serial monitor only sends characters after you press enter.
    // Other devices can send characters at random times.

    DietSerial.setTimeout(20);  // seconds. The default is 90.

    GetAndPrintString();
    // Prompt for a floating-point number to be entered and print it
    GetAndPrintFloat();

    Terminate();

//// } // end of setup.
//// void loop() {
//// replace void loop() with this "for (;;)" line:
//    for (;;) {
//    }

} // end of main()

// =============================================================================
//    Supporting Functions
// =============================================================================

void PrintBoolean(void)
{    
    DietSerial.println(F("Printing a boolean variable, set to true:"));
    bool tf = true;
    DietSerial.println(tf);   // "true"
}

void PrintStringFromRAM(void)
{
    char SRAMstring[] = 
        "This is a 'simple' string, stored in RAM.";
        // 42 bytes of RAM.  Reclaimed when this function finishes.
    DietSerial.print(SRAMstring);  // print the text above without a new-line
    DietSerial.println();          // print the new-line.
    DietSerial.println("Another constant text string in RAM.");
    DietSerial.println();          // Two println()s - double-spacing.
}

void PrintStringFromFlash(void)
{
    // Print a line with a string that is stored in flash memory via Arduino's F() macro:
    DietSerial.println(
        F("This is a string stored in flash for printing with the F() macro.\n"));
        // Zero bytes of RAM.  '\n' is newline: with println, get two lines.
}

void PrintNamedFlashString(void)
{    
    // Print a Named String stored in Flash memory. 
    // (This saves flash compared to using F() 
    //  when you want to print the same text twice or more.)

    static const char aTestFlashString[] PROGMEM = 
        "This is a Named String stored in Flash, printed with printlnP.\n";
    
    DietSerial.printlnP(aTestFlashString);
}

void PrintByteAsFixedLengthBinary(void)
{
    DietSerial.println
        (F("Printing a byte in fixed-length binary format (decimal value 118):"));

    int8_t singlebyte = 118;
    DietSerial.printBinary(singlebyte);
    DietSerial.println();
}

void PrintFloat(void)
{
    DietSerial.println
        (F("A floating point number -1.2345678, printed with 4 decimal places, and with 7:"));

    float floatNumber = -1.2345678;
    DietSerial.println(floatNumber);      // printing floats defaults to 4 decimal places
    DietSerial.println(floatNumber, 7);   // second parameter is number of decimal places
    DietSerial.println();
}

void PrintInteger(void)
{
    DietSerial.println
    F(("Printing an integer in decimal, hexadecimal and binary:"));

    int integer = 21400;
    DietSerial.println(integer);   // default format is decimal
    DietSerial.println(integer, HEX);
    DietSerial.println(integer, BIN);  // binary
    DietSerial.println();
}

void GetAndPrintString(void)
    {
        DietSerial.println
            (F("Type a string to be sent to the Arduino and press Enter."));

        // Define a buffer to hold the text received from the serial monitor
        char buffer[40];
        // fill the receive buffer with null bytes to ensure we get a null terminated string.
        for (auto& ch : buffer) ch = '\0';

        int bytesReceived = 
            DietSerial.readString(buffer, sizeof(buffer));

        if (DietSerial.error())
        {
            DietSerial.printlnP(ErrorHeader);
            DietSerial.printError(DietSerial.error());
        }
        else if (bytesReceived == 0)
        {
            DietSerial.printlnP(ErrorHeader);
            DietSerial.printlnP(ErrorNoData);
        }
        else // no error and some text received
        {
            DietSerial.println(F(" The string received is:"));
            DietSerial.println(buffer);
        }
        DietSerial.println();
    }

void GetAndPrintFloat(void)
{
    // Getting a floating-point number directly from the incoming data stream.
        DietSerial.println
            (F("Type a string containing a floating-point number and press Enter."));
        
        double d = DietSerial.parseFloat();

        if (DietSerial.error())
        {
            DietSerial.printlnP(ErrorHeader);
            DietSerial.printError(DietSerial.error());
        }
        else if (isnan(d))  // if d is not-a-number
        {
            DietSerial.printlnP(ErrorHeader);
            DietSerial.printlnP(ErrorNoFloat);
        }
        else // no error 
        {
            DietSerial.println(F(" The number received is (to 4 decimal places):"));
            DietSerial.println(d, 4);
        }
    
}

void Write4BytesFromRAM(void)
{
    // Using write(block, blocksize):
    DietSerial.println
        (F("Binary write() of a block of 4 bytes in SRAM, " 
           "all = 'A' = decimal 65, hex 0x41:"));

    byte ramBytes[4];
    for (auto& rB :ramBytes) rB = 'A';  // modern C++ style for loop.
    DietSerial.write(ramBytes, sizeof(ramBytes));
    DietSerial.println();
}

void WriteBlockOfBytesFromFlash(void)
{
    DietSerial.println
        (F("Bytes stored in flash: DietSerial.writeP(block, sizeof(block)):"));

    static const uint8_t listOfConstants[8] PROGMEM = 
        {0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47};

    DietSerial.writeP(listOfConstants, sizeof(listOfConstants));
    DietSerial.println();
}

void Terminate(void)
{
    DietSerial.println(F("That's the end of the Simple Usage example of DietSerial."));
    DietSerial.flush();
    exit(0);
}
