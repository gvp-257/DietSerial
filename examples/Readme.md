# DietSerial Examples

### SimpleUsage

Demonstrates using the default baud rate (9600), `print`ing strings and numbers, and writing  binary blocks of bytes - the `write` function, usually used for non-text data.

Also demonstrates reading a line of text from the Serial Monitor, and getting a floating-point number from the Serial Monitor.

### NonblockingRead

Demonstrates receiving characters one at a time as the loop() function is executed over and over, while doing other work inside the loop function as well.  Receives whatever you type in from the serial monitor.

Note: you cannot use delay() inside loop() or any function called from it, if you use this technique.

### MemoryComparison

Report memory usage of DietSerial versus Arduino's Serial for printing strings and numbers.

With Arduino IDE 2.3.6 and an Uno clone, the numbers I obtained are:

|     bytes:  |   FLASH|  SRAM|
|    :-----:  |-------:|-----:|
|Serial       |    5116|   198|
|DietSerial   |    3650|    18|


Note: Arduino uses 9 bytes of SRAM at the bare minimum, for the millis() and micros() functions.

### Shorthand

Demonstrates convenience functions for printing commonly used characters, like comma, dash, dot, colon, and tab. These are the same as in the library [SendOnlySerial](https://github.com/gvp-257/SendOnlySerial).


### DebuggingExample

Shows the debugging macros `printVar`, `printFloatVar`, and `printReg` (print register). These are the same as in the library [SendOnlySerial](https://github.com/gvp-257/SendOnlySerial).