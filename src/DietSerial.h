#ifndef DIETSERIAL_LIB_H
#define DIETSERIAL_LIB_H

#if defined (__AVR_ATmega328P__)  || defined (__AVR_ATmega168PA__) \
 || defined (__AVR_ATmega328PB__) || defined (__AVR_ATmega328__)   \
 || defined (__AVR_ATmega88PA__)  || defined (__AVR_ATmega168A__)  \
 || defined (__AVR_ATmega48PA__)
#else
#error “DietSerial only supports boards with AVR ATmega168A/PA/328/P/PB processor.”
#endif
// DietSerial:  low-RAM lib for debugging or logging using ATmega328P's
// hardware USART (USART0).

// GvP, 2025.   MIT licence.
// https://github.com/gvp-257/DietSerial

/*
  Low RAM usage library for sending and receiving data and text to
  Arduino's Serial Monitor, or to another Arduino for logging via the rx0 and
  rx1 pins.  Intende mainly for breadboard arduinos and similar battery operated
  projects.

  Only uses 8N1 data frames (bytes).
  No support for Arduino String objects.
  Don't use it in a sketch together with Arduino's Serial object.

  Possible TODO: extend to ATmega1284P and/or ATmega2560 (Mega).

  References
  ==========
  ATmega88A/PA/168A/PA/328/P datasheet: section 20, USART0.
  https://www.microchip.com/en-us/product/ATMEGA328P

  Arduino Reference for Serial:-
  https://docs.arduino.cc/language-reference/en/functions/communication/serial/

  "MAKE: AVR Programming" by Elliot Williams. Makermedia, Sebastopol, CA, USA;
  2014. Chapter 9 on serial communications using the USART.
*/

#include "DietSerial_macros.h"  // debugging convenience prints.


// Copied from WString.h: help class for Arduino's "F()" macro for printing
// strings stored in flash.
class __FlashStringHelper;

// If Arduino's stream has not been included, add enum for
// parseFloat and parseInt
#ifndef Stream_h
enum LookaheadMode
{
    SKIP_ALL,
    SKIP_NONE,
    SKIP_WHITESPACE
};
#endif

// Receive error code.
typedef uint8_t rxerr_t;
// 0 = no error, 1 = timeout, 2 = garbled byte, 4 = other,
// 8 = buffer too small to hold data.

/* Binary bytes to integer/fp conversion unions for read and write fns. */

typedef union {
    uint8_t b[2];
    int     i;          // AVR8: int is 2 bytes.
} bytepair;

typedef union {
    uint8_t b[4];
    double  d;          // AVR8: double is same size as float, 4 bytes.
    float   f;
    long    l;
} quadbyte;


struct AVR_USART
{
private:
    uint8_t timeout_seconds_;
    rxerr_t receive_err_;

public:
    void begin(const unsigned long baudrate = 9600UL);  // baud rate.
    void end(void);                   // power off USART hardware module.

    // =========================================================================
    // Receive Control and Monitoring

    inline
    unsigned long getTimeout(void)
        {return timeout_seconds_;}

    inline
    void    setTimeout(uint8_t newto = 90)
        {timeout_seconds_ = newto;}

    inline
    rxerr_t error(void)
        {return receive_err_;}

    void    printError(const rxerr_t err);    // print text for the error.

    inline
    bool    available(void)
        {return bit_is_set(UCSR0A, RXC0);}// synonym for hasByte

    inline
    bool    hasByte(void)
        {return bit_is_set(UCSR0A, RXC0);}
        // RXC0 flag is set: there is a received byte in UDR0.
        // Reading UDR0 resets the RXC0 flag.

    inline
    bool    byteOK(const uint8_t b)
    {return ((b != 0x15) && (b != 0x18));}
    // Not NAK or CAN

    // ========================================================================
    // Binary data receiving functions.

    // readByte(): Main "receive a byte" function. Get a byte from the USART.
    // Either one already there, or wait for a new one.
    unsigned char readByte(void);
    unsigned char rxcompleted(void);  // Check and return byte already received
    unsigned char rxwait(void);       // Wait for a byte to appear in receive buffer

    // Multi-byte binary data receive.
    size_t  readBytes(uint8_t* buf, const size_t bufLen);
    // specific number formats
    char    readChar(void);       // char - signed 8bit
    void    read4bytes(quadbyte& qb);
    double  readDouble(void);
    float   readFloat(void);
    int     readInt(void);
    long    readLong(void);


    // =========================================================================
    // Human-readable ASCII receiving functions.

    char read(void);

    // Null-delimited ASCII up to max chars. Incoming data terminated
    // with CR-LF, LF, or null byte.
    // Changes CRLF or LF to null, the C string terminator.
    size_t  readString(char* buf, size_t bufLen);

    // Filter incoming characters according to LookaheadMode and ignore:

    // See if a char is wanted according to LookaheadMode and ignore.
    bool    wantChar(char c,
                     const LookaheadMode mode,
                     const char ignore);

    size_t  readStringExcept(char* buf, const size_t buflen,
                     const LookaheadMode mode = SKIP_ALL,
                     const char ignore = 0x7F);

    // Extract a number out of incoming (or existing) text.

    double  parseFloat(const LookaheadMode mode = SKIP_ALL, const char ignore = 0x7F);
    long    parseInt(const LookaheadMode mode = SKIP_ALL, const char ignore = 0x7F);

    double  parseFloat(char* buf, const size_t bufLen); //from already-received data
    long    parseInt(char* buf, const size_t bufLen);   // already-received data


    // =========================================================================
    // Basic send monitoring functions.

    inline
    void flush(void)                  // Wait for last byte to be sent.
        {loop_until_bit_is_set(UCSR0A, TXC0);}

    inline
    bool isReady(void)                // USART has room for a byte to send.
        {return (bit_is_set(UCSR0A, UDRE0));}

    inline
    bool ready(void)                  // synonym
        { return (bit_is_set(UCSR0A, UDRE0));}


    // =========================================================================
    // Binary data transmitting.

    // Single byte
    void txRaw(const uint8_t b);       // Does not check if the USART has room.
                                       //  - use after isReady() returns true
    void write(const uint8_t b);          // Give the USART a byte to send.
    // multiple bytes
    void write(const uint8_t* buffer, const size_t numBytes);

    // other data types
    void write(const char c);
    void write4bytes(quadbyte& qb);
    void write(const double d);
    void write(const float f);
    void write(const int i);
    void write(const long l);

    // Bytes that are in program memory (flash):-
    void writeP(const uint8_t* buffer, const size_t numBytes);


    // =========================================================================
    // Printing functions: human-readable output.

    // 8-bit special types
    void printBinary(const uint8_t b); // B0010 1100 format: constant length
                                       // better than print(b, 2)
    void printDigit(uint8_t b);        // least signif. 4 bits -> char 0-9,a-f
    void print(const bool b);          // "true", "false"

    void print(const char c);          // aka int8_t, receiver thinks ASCII

    // Numerical types
    void print(const double d , const int decimals = 4);
    void print(const float  f , const int decimals = 4);

    void print(const int    i, const int base = DEC); //base: 2 10 16 bin dec hex
    void print(const long   l, const int base = DEC);

    void print(const uint8_t       ub, const int base = DEC);
    void print(const unsigned int  ui, const int base = DEC);
    void print(const unsigned long ul, const int base = DEC);

    // Strings - C null-delimited arrays only.
    void print(const char*);
    // Strings stored in flash with Arduino's F() macro (see WString.h)
    void print(const __FlashStringHelper *str);

    // println() variants of the above.
    void println(void);

    void println(const bool);
    void println(const char);

    void println(const double d, const int base = DEC);
    void println(const float  f, const int base = DEC);

    void println(const int    i, const int base = DEC);
    void println(const long   l, const int base = DEC);

    void println(const uint8_t       ub, const int base = DEC);
    void println(const unsigned int  ui, const int base = DEC);
    void println(const unsigned long ul, const int base = DEC);

    void println(const char*);
    void println(const __FlashStringHelper *fsh);

    // Strings in flash memory, defined:-
    // static const char infostring[] PROGMEM = "InfoInfoInfo!";
    void printP(const char*);
    void printlnP(const char*);

    // Convenience functions for common characters

    // whitespace
    void tab(void);
    void space(void);
    void crlf(void);

    // characters in formatted numbers and dates
    void comma(void);
    void colon(void);
    void dot(void);
    void dash(void);
    void minus(void); // same as dash
    void plus(void);
    void equals(void);

    void lparen(void);
    void rparen(void);

    void slash(void);
    void star(void);

    void percent(void);
    void dollar(void);

    // punctuation.
    void apos(void);
    void dquote(void);
    void qmark(void);
    void langle(void);
    void rangle(void);
    void at(void);
    void vbar(void);
};

// The object:
extern struct AVR_USART DietSerial;

#endif