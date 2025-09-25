#include <Arduino.h>
// Non-blocking read:
// demonstrates using loop() to receive a line of text character by character,
// while doing other work inside loop().
//
// NOTE: you cannot use delay() inside loop() or any of the functions called
// loop() if you use this technique, and the technique makes a lot of use 
// of global variables to avoid using delay and friends.
//
// As you can see, this is more fiddly than using Serial.readString or 
// DietSerial's blocking DietSerial.readString(buffer, bufLen) function.
// ============================================================================
#include "DietSerial.h"


// ============================================================================
// Configuration of pins for DoOtherWork()

static const uint8_t LEDPIN = 13;
bool         ledState = false;  // off

const int    ledOffMillis = 650;
const int    ledOnMillis = 2000;

unsigned long ledNextChangeTime;

// Other variables for other work:-
// Number of times the loop() function is executed.
unsigned long loopCount = 0;

// ============================================================================
// Non-blocking "receive string" variables.
// Flag to indicate we are ready to receive character(s) over RX0.
bool    wantChars = false;  // Are we expecting characters over the RX pin?

// Buffer for receiving characters over the serial interface, RX0 pin
char    buffer[61];
size_t  position = 0; // Where to put the next character in the buffer.

// ============================================================================
//    Main program
// ============================================================================

// Flag to finish the program.
bool    finished = false;

void setup()
{
    // Configure the "Other Work"
    pinMode(LEDPIN, OUTPUT);
    digitalWrite(LEDPIN, (ledState? HIGH : LOW));  // initial state.
    // ... set the time to change the led next.
    ledNextChangeTime = millis() + (ledState? ledOnMillis : ledOffMillis);

    DietSerial.begin();
    // begin() defaults to 9600 baud and 90 seconds receive-wait timeout.

    // DietSerial.setTimeout(10);  // seconds

    // Clear any stray characters that have come in before we are ready:
    while (DietSerial.available()) {DietSerial.read();}

    // Prompt for input:
    PrintPrompt();
    wantChars = true;
} // end of setup()

void loop()
{
    if (finished) Terminate();
    ++loopCount;  // count times through loop for printing at the end.

    // check for an incoming character on RX0.
    if (wantChars && DietSerial.available())
        {GetIncomingCharacter();}
    else
    if (wantChars && DietSerial.error())
    { 
        DietSerial.printError(DietSerial.error());
        // handle the error: clear the buffer and try again.
        position = 0;
        for (auto& ch : buffer)  ch = 0;
        PrintPrompt();
    }

    DoOtherWork();
    
    if (!wantChars)
    // We have finished getting the string: print it and quit
    // In normal code we would not terminate.
    {
        PrintReceivedString();
        PrintLoopCount();
        finished = true;
    }
}

// =============================================================================
//    Supporting functions
// =============================================================================

// Example of other work being done while receiving the text over RX0

void DoOtherWork(void)
{
    // Toggle the LED pin
    if (millis() >= ledNextChangeTime)
    {
        ledState = ledState? LOW : HIGH;           // toggle boolean
        ledNextChangeTime = millis() + (ledState? ledOnMillis: ledOffMillis);
        digitalWrite(LEDPIN, ledState? HIGH: LOW); // set the pin high or low accordingly.
    }
}
void PrintPrompt(void)
{
    DietSerial.println
        (F("Type a string to be sent to the Arduino and press Enter."));
}

void GetIncomingCharacter(void)    
{
    char c = DietSerial.read();  // Since available is true, read returns immediately
    if (position < (sizeof(buffer) - 1)) // buffer is full
    {
        if (isNormalChar(c))
        {
            buffer[position] = c;
            ++position;
        }
        else
        {
            buffer[position] = '\0'; // terminate the string with a NUL
            wantChars = false;
        }
    }
    else 
    { 
        // Full up: terminate string.
        buffer[position] = '\0'; // terminate the string with a NUL
        wantChars = false;
    }
}

bool isNormalChar(char c)
{
    // if not end-of-line and not a NUL: is normal.
    switch (c)
    {
        case '\n':
        case '\r':
        case '\0':
            return false;
        default:
            return true;
    };
}

void PrintLoopCount(void)
{
    DietSerial.print(F("Count of times through loop(): "));
    DietSerial.println(loopCount);
    DietSerial.println();
}

void PrintReceivedString()
{
    DietSerial.print(F("Text that was received: "));
    DietSerial.println(buffer);
    DietSerial.println();
}

void Terminate(void)
{
    DietSerial.println(F("That's the end of the Simple Usage example of DietSerial."));
     exit(0);
}
