# DietSerial.h

A low-RAM alternative to Arduino's Serial for AVR-based Arduinos. Down to 9 bytes of RAM, *vs.* Serial's 175+ bytes RAM. (Arduino uses 9 bytes for the millis() and micros() functions in addition to these numbers.)

## Why Not: Single platform, Watchdog Timer

Arduino is really great at keeping things easy for you. `DietSerial` is a little more "low-level" than `Serial`, and it only works for the ATmega168A/PA/328/P/PB: the Uno R3, Nano, Pro, Pro Mini, and for breadboard Arduinos based on the ATmega328P chip.

Also, `DietSerial` uses the **Watchdog Timer** for receive timeout. If your sketch uses the Watchdog Timer for other purposes then `DietSerial` is not suitable. If you don't know, you're probably OK.

### Alternative library

If your sketch only prints data to the PC, and does not receive data input from the serial interface, consider using [`SendOnlySerial`](https://github.com/gvp-257/SendOnlySerial) instead. It is smaller and uses less RAM.

## Why: Reduce Permanent RAM Usage

`Serial` permanently allocates two 64-byte buffers and 17 or so bytes of bookkeeping variables that permanently take up space in your sketch's RAM. The buffers are used to accumulate incoming bytes and to hold bytes to send out over the hardware serial interface.  The bookkeeping variables are use to male Serial cross-platform, to keep track of the baud rate and data format, to keep track of how full the buffers are, and for timeout functionality.

For receiving data, you allocate a temporary buffer of the size you need, in your sketch as you do for `Serial.readBytes()`. `DietSerial` uses this same method for `readBytes()`, `readString()`, and `readStringExcept()`. DietSerial directly sets the baud rate in `DietSerial.begin()`, with a default value of 9600. It only has one data format, 8N1, so needs no storage for that, and it has no hidden buffers, instead reading directly from the hardware. `DietSerial` uses one byte for the timeout which is in seconds rather than milliseconds, so the maximum timeout is 255, with the default at 90 seconds.

Printing is similar to `Serial.print()` and `Serial.println`. Ther is also a `DietSerial.write()` function for sending arrays and floating-point and integer numers in binary format. and `DietSerial.readBytes()`, `DietSerial.readFloat()`, `DietSerial.readInt()`, `DietSerial.readDouble()`, and `DietSerial.readLong()` for receiving them. Finally there are also `DietSerial.readByte()` and `DietSerial.read()` for single bytes or text characters respectively.

As an alternative to formatting numbers, dates, and so on in text with `snprintf()`, `DietSerial` includes several "convenience" functions for printing character by character:  `DietSerial.colon()`, `DietSerial.slash()`, `DietSerial.dash()`, `Dietserial.dot()`, `DietSerial.percent()`, and so on. In this way the RAM requirement for printing formatted text is minimised.

## Example

Compiling the [MemoryComparison](examples/MemoryComparison) example gives the following results with Arduino IDE 2.3.6 on an Uno:


|        bytes:      |FLASH| SRAM|
|       :------:     |----:|----:|
|Serial (built in)   | 5116|  198|
|DietSerial library  | 3650|   18|

These numbers are for SRAM that is permanently allocated at the start of the sketch, "static" SRAM.

### NOTES

 1. Arduino uses 9 bytes at the bare minimum in both cases, for the `millis()` and `delay()` functions.
 2. `Serial.readString()` only works with Arduino `String` objects, which further inflates its flash and RAM demand.
 3. The `DietSerial.readString()` code includes error-checking while the `Serial.readString()` does not.

## Hardware Compatibility

The `DietSerial` library is compatible with the Arduino Uno, the Nano, the Duemilanove, and the Pro Mini (both 5 volt and 3 volt) boards. It will also work with "breadboard Arduinos" using the AVR ATmega328P microcontroller and with a system clock at 16 MHz, 8 MHz, or 1 MHz.

`DietSerial` uses the RX0 and TX1 "hardware serial" pins, which are also connected to the USB interface on Unos and Nanos, just like Serial.


## Differences from Arduino's Built-in Serial

There are several difference from `Serial`, both obvious and not so obvious.

### Missing (Serial functionality not implemented)

#### No String objects

Arduino's `String` objects are not supported. If you're considering using this library, because you are running out of RAM, Arduino `String` objects use too much RAM anyway.

#### Unimplemented Functions

Some less common functions are not supported - `Serial.find()`, `Serial.findUntil()`, and others. 

#### No Collecting Input Behind The Scenes

Arduino's `Serial` works in the background filling and emptying its buffers, accumulating data received or sending data out while your code can do other things.

In practice, most sketches tend to wait for data to be sent or received anyway, using the `Serial.flush()` function to ensure that the transmission has completed, and `Serial.available()` to check whether data has started to arrive.

`DietSerial` is **blocking**, meaning that multi-byte `print`s or `read`s wait for the serial interface to transmit or receive the data over the cables.  The serial hardware holds one byte in its internal buffer.

#### Different Timeout Numbers

Arduino `Serial` expects timeout to be in milliseconds. `DietSerial.setTimeout()` uses seconds, and has a default of 90 seconds and a maximum of 255 seconds.

`DietSerial.setTimeout(60)` is the same time-out as `Serial.setTimeout(60000)`.

#### Timeout Uses the Watchdog Timer

Under the covers, the time-out set with `DietSerial.setTimeout(60)` is timed by the ATmega328P's watchdog timer. The watchdog timer is not exactly precise.

#### Only 8N1 Data Format and CRLF line endings

`DietSerial` is limited to 8N1 formatted data frames. (8 data bits, no parity, 1 stop bit. 8N1 is by far the most common data frame format.) `DietSerial` uses "CR and LF" line endings when sending data.  It will receive lines ending with just LF as well, though.

### Function Differences

#### `DietSerial.available()`

`DietSerial.available()` returns `true` or `false`, indicating whether the hardware has received a byte ready for use by your sketch. (`Serial.available()` tells you the number of bytes waiting in its buffer.)


#### `DietSerial.read()`

`DietSerial.read()` with empty parentheses returns a `char` - a number between 0 and 127, intended to represent an ASCII character.


        char c = 0;

        c = DietSerial.read();

        if (DietSerial.error() > 0) {
           ; // handle the error - see below for details on error()
        }
        else {
           // do something with the character;
        }

Here is the basic loop for reading data coming in over the serial hardware interface:-


        char myBuffer[20];                // buffer to hold incoming data
        for (auto& b : myBuffer) {b = 0;} // clear the buffer to all zeros
        uint8_t bufPos = 0;               // position in buffer to place received byte
        char c = 0;                       // temporary holder for received character

        while (bufPos < 20) {             // while buffer is not full:

          c = DietSerial.read();          // get the received byte, waiting if necessary
          if (DietSerial.error())         // if hardware error has occurred,
            break;                        // jump out of the while loop.

          if ( (c == 0) || (c == '\n') || (c == '\r'))  //  Ascii NUL or LF or CR
            break;                        // End of the text string: stop reading.

          myBuffer[bufPos] = newByte;     // Place received character into the buffer

          ++bufPos;                       // move ready for next character

        };  // end bufPos loop.


        // Handle errors and/or process myBuffer's contents....

Note that `DietSerial.error()` will return a non-zero code (8, in fact) if the buffer is filled up before we receive a a NUL or CR or LF.

#### `DietSerial.readByte()`

This works as for read-with-empty-parentheses above, but returns a byte, with a value between 0 and 255 decimal. It treats NUL and CR and LF as ordinary data - the numbers 0, 14, and 10 respectively.

#### `DietSerial.readString(buffer, buflen)`

The "basic loop" above is what is done inside `DietSerial.readString(myBuffer, 20)`. Here is a small example using that, and showing error handling:-


        char myBuffer[20];             // as above
        for (auto& c : myBuffer) c = 0;

        size_t charsReceived = 0;         // size_t is usual for counting array elements.
        const size_t MinimumChars = 5;    // We expect to receive at least 5 characters

        charsReceived = DietSerial.readString(myBuffer, 20);

        // charsReceived does not count the ending NUL character, as is normal with
        // C's strlen() function.

        // Handle errors and/or process myBuffer's contents.
        if (DietSerial.error()) {
           DietSerial.printError(DietSerial.error()); // prints text explaining the error
        }
        else
        if ( charsReceived < MinimumChars) {
            // handle unexpectedly short text.
        }
        else {
           processBuffer(myBuffer, charsReceived);
        }


#### `DietSerial.readString()`: Receive Text

This reads and accumulates incoming data until an end-of-line character or a NUL character is received. It replaces the end-of-line  CR and LF with a `null` byte, so that the receiving buffer holds a C-style string, an array of characters ending with a null byte.

`DietSerial.readString(myBuffer, buflen)` returns the number of bytes received and updates `DietSerial.error()` in the same way as `DietSerial.read(buffer, buflen)` above. The difference is that bytesReceived may be less than the full length of ther supplied buffer. bytesReceived does not include the terminating null byte.

#### `DietSerial.parseInt()` and `DietSerial.parseFloat()`

`DietSerial.parseInt()` returns a `long`. `DietSerial.parseFloat()` returns a `double`. Yes, the names are misleading.

As well as versions that work directly with incoming data, both of these functions have versions that work on already received data:-


    // We are expecting the other end to send us 3 or more characters representing a
    // floating-point number, followed by CRLF and/or LF, like 1.9<CR><LF>
    // Possibly with other characters in there as well, like "distance: 16.5 cm<CR><LF>"

    // ----------------------------------------------------
    // 1. Directly receive a floating-point number as text.
    //    (Don't send data this way, if you can just use write(floatingpt);
    //    i.e., send four binary bytes instead of potentially 18 text bytes.
    //    But some third-party modules send text.)

    double d1 = DietSerial.parseFloat();  // Receives a line of text and extracts a
                                          // floating-point number from it. Takes time.

    // process the number if text was received and represented a valid number.

    if ((! DietSerial.error()) && (! isNan(d1)))  // isNan comes from <math.h>: is not a number
    {
        processInput((float)d1);   // processInput() expects a float argument.
    }

    // ----------------------------------------------------
    // 2. Getting a floating-point number from a pre-existing string.

    // Assume we have already received a line of text into myBuffer, like so:
    uint8_t myBuffer[20]; for (auto& b : myBuffer) b = 0;

    size_t bytesGot = DietSerial.readString(myBuffer, 20);

    // Then we can parse out a floating-point number this way:-
    double d2 = NAN;  // not-a-number for doubles

    if (!DietSerial.error() && bytesGot >= 3) {
      d2 = DietSerial.parseFloat(myBuffer);
    }

    // if received a valid double, process it.
    if (! isNAN(d2)) {
      doMyThing((float)d2);    // doMyThing() expects a float, not a double.
    }

### Additions

#### Printing: debugging functions and macros

Two functions and three macros that may be useful for debugging your code.

`DietSerial.printBinary(byte b)`: print a single byte in fixed length binary format, in two groups of four bits:-

    DietSerial.printBinary(0xc6);       // prints "0b1100 0110"  via serial.

`DietSerial.printDigit(byte b)`: prints the low four bits of b in ASCII:-

    DietSerial.printDigit(0xf5);        // prints "5".
    DietSerial.printDigit((0xf5 >>4);   // prints "f": the byte is right-shifted four bits.


#### Convenience Functions

`DietSerial` has several "convenience" functions for printing common characters: `DietSerial.comma()`, `DietSerial.dot()`, `DietSerial.colon()`, `DietSerial.dash()`, `DietSerial.percent()`, `DietSerial.tab()`, `DietSerial.CRLF()` for line endings, and so on.


#### Debugging Macros

These are usable unless `NDEBUG` is defined. (`NDEBUG` is a C standard macro for disabling `assert()` macros when compiling your code for deployment.)

Use these macros "bare", i.e. without putting `DietSerial.` in front:-

`printVar(integer-variable)`:

    int count = 73;
    printVar(count);    // prints "count    73   0x49"

`printFloatVar(float-variable)`: prints a line with the name of the variable and its contents, in decimal with four decimal places.

`printReg(REGMACRO)`: prints a line with the name and contents of the given ATmega register (or other byte variable), in binary, hexadecimal, and decimal.

    printReg(UBRR0L);  // prints: "UBRR0L  0b1100 1111      0xcf      207"

With `NDEBUG` #defined, these macros do nothing. You may need to `#undef NDEBUG` to use them where required.


## DietSerial Functions - Input and Output

All functions are members of the `DietSerial` object.  That is, write `DietSerial.begin();`, not just `begin();` in your code.

### INPUT

|Function                     |Remarks                                                                                                                                                                                                                                                                                                                                                           |
|-----------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
|`begin(BAUDRATE)`            |Sets the baud rate for sending and receiving, and the default timeout duration (90 seconds) for receiving. The default baud rate, with  an "empty" `begin()`, is 9600. Recommended baud rates, if the default is too slow, are "round" numbers, e.g. 100000, 125000, but not 115200.|
|`end()`                      |Disables the ATmega's internal serial hardware module and powers it off.|
|`setTimeOut(_seconds)`       |Sets the number of seconds that `read()` functions should  wait for input before giving up and setting the "receive timed out" error code, inspectable with `DietSerial.error()`. Allowed values: `0` to `255`. The default is `90` (90 seconds). The timeout is per each character: successfully receiving a character resets the timer to zero, and it starts counting up to the timeout value again.|
|`available()`                |Returns `true` or `false`, whether a byte has been received by the hardware ready to be read by your code. If `available()` is `true`, `byte b = read();` returns immediately. Otherwise, `read()` will block, waiting for a byte to appear over the wire. All multi-byte `readXxx()`  and `parseXxx()` functions block after the first character.|
|`hasByte()`                  |A synonym for `available()`. |
|`error()`                    |Returns the status of the last character receive attempt. `0` means no error, non-zero means an error occurred. See `printError()` for descriptions.|
|`read()`                     |Returns a single byte : `uint8_t ch = DietSerial.read();`. Returns a `NAK` `0x15`, "receive unsuccessful", if the timeout expires, or a `CAN`, `0x18`, "discard character", if a transmission error was detected. Sets the error code which can be inspected with `error()` and described using `printError(DietSerial.error())`.  |
|`read(buffer, buflen)`       |`size_t stringSize = DietSerial.read(buffer, buflen);`.  Reads a line of text terminated with CR and LF, or just LF, into the supplied `char` array `buffer`. Replaces the CR-LF or LF at the end with a `NUL` (decimal 0) character. Returns the length of the string read, not including the terminating `NUL`. If no CR or LF is received after `buflen - 1` characters are received, `read(buffer, buflen)` sets an error code, "buffer too small" - check it with `DietSerial.error()` - and replaces the last character with a `NUL`. `read(buffer, buflen)` sets error codes for other errors also.|
|`readString(buffer, buflen)` |Synonym for `read(buffer, buflen)`. Makes it explicit that you are expecting a line of text from the serial input. Error codes as described under `read(buffer, buflen)` above.    |
|`readBytes(buffer, nbrBytes)`|Read exactly `nbrBytes` bytes of data from serial input and store them in the supplied array `buffer`.                                                   |
|`readChar()`, `readInt()`, `readLong()`, `readFloat()`, `readDouble()`| Receives 1, 2, 4, 4, or 4 binary bytes respectively, pastes them together as required, and returns the value as the specified data type.  `char c = readChar();`, `int i = readInt();`, etc.|
|`parseInt()`                 |For numbers sent as text. Expects a sequence of digit characters, possibly with a '-' in front. Reads the incoming characters until a non-digit occurs and returns a `long int` (`int32_t`). Returns 0 if an error occurred: use `error()` to check for errors.|
|`parseInt(buffer)`           |Returns an integer from a sequence of digit characters in the `NUL`-terminated string in the `char` array `buffer`, which may have been read in with `readString()`. Returns `0` if there was an error; use `error()` to check for successful reading of the text if `0` is a possibly correct value.|
|`parseFloat()`               |Expects to read in a sequence of characters representing a floating-point number in "natural" format, e.g. -0.0012345. Returns a `double` with the floating-point value if successful. Returns `NAN` and sets a non-zero error code if there was an error.|
|`parseFloat(buffer)`      | As for `parseInt(buffer)`. If successful, returns a `double` being the number specified in the NULL-terminated string of characters in `buffer`.                                                                                           |
|`printError(DietSerial.error())`|Prints text describing the error code returned from `read()`, `readBytes()`, `readLine()`, `parseInt()`, or `parseFloat()` to the serial output. Error codes are `0`: no error, `1`: read timed out, `2`: data is garbled, discard the byte or bytes, `4`: other error, for example zero bytes before `<CR>` or `<LF>` in `readstring()`, `8`: supplied buffer is too small. |


### OUTPUT

|Function              |Remarks                                                                                 |
|----------------------|----------------------------------------------------------------------------------------|
|`begin(BAUDRATE)`     |The default is 9600.                                                                    |
|`end()`               |Disables the hardware and turns it off, saving a few microamps                          |
|`flush()`             |Flush waits for the last byte to be transmitted by the USART hardware.                  |
|`print()`, `println()`|Print most types of data in readable format.                                            |
|`printBinary()`       |Print a byte as a fixed length string of form "0b0011 1010".                            |
|`printDigit()`        |Print the lower 4 bits of the given byte as a single hexadecimal character 0-9,a-f.     |
|`printP()`, `printlnP()`|Print named strings stored in program memory (flash). `printP(promptText);` works with `promptText` defined as `static char promptText[] PROGMEM = "Type something please: ";`.  Useful if you want to print the same string in several places in your code.|
|`write()`             |send individual characters(`write(c)`), or blocks of bytes (`write(array, sizeOfArray)`) without making them readable. There are also versions for `int`, `long`, `float`, and `double` variables, and the `unsigned` variants `unsigned int` and  `unsigned long`: `write(integerVar)`, `write(floatVar)`, etc. These send the variables as fixed-length binary: `write(floatVar)` will send 4 bytes, ready to read at the other end with `float f2 = readFloat();`.|


All the above functions are members of the `DietSerial` object. Use `DietSerial.begin();`, and so on.

You might like to define shorthand macros and/or reference variables to save on typing and clutter in your code:

      #define DS DietSerial       // Now you can use DS.begin();, DS.println(); etc.

      auto& DSref = DietSerial;   // DSref is a reference to DietSerial; now you can use DSref.begin();  etc.

      #define flashString(stringName, stringValue) static const char stringName[] PROGMEM = stringValue
      // Now you can use:
      // flashString(infoString1, "InfoInfoInfo");
      // DietSerial.printlnP(infoString1);


### Floating-Point Uses More RAM

If you print floating-point numbers in readable format, `DietSerial` uses an extra 2kb-ish of flash memory and 20-ish bytes of RAM, in the AVR-libC standard library function `dtostrf()` for formatting floating-point numbers.

## Limitations on Sending

Besides those listed above? :-)  You have to wait.

Because Arduino claims the "USART Data Register Empty" interrupt for its Serial object, all of DietSerial's functions are blocking, meaning that your program waits while they do their thing.

`print()` and `printP()`, `println` and `printlnP()` mostly wait for the hardware to trundle the bits and bytes out over the wire, and they will return to your code only when the last byte has been handed off to the ATmega's internal hardware serial module for transmission. `flush()` waits for the hardware to tell us that that last byte has been sent.

## INSTALLATION

Click the green "Code" button, and choose "Download zip".  Unzip the downloaded zip file into your Arduino "libraries" folder inside your sketchbook folder.  If using the Arduino IDE, search for DietSerial in the library manager.

## USAGE

Include the file at the top of your sketch:

    #include "DietSerial.h"

Then use the functions and macros described above, seasoning to taste.

## TODO

as at 2025-09-24  GvP.

### Maybe

Document functions more thoroughly.

Measure flash and RAM consumption more rigorously.


### Unlikely

Adapt to support the ATmega2560 and/or ATmega1284P microcontrollers.

Adapt to support the ATtiny44/84 or ATtiny45/85 microcontrollers.

Support other parities, stop bits, and error checking.

Non-blocking transmit functions, controlled with a "WANT" define, for use outside the Arduino environment.
