
// DietSerial:  low-RAM Serial replacement using ATmega328P's hardware USART (USART0).

// GvP, 2025.   MIT licence.
// https://github.com/gvp-257/DietSerial
#include <stdlib.h>        // For itoa, ltoa, dtostrf, atol, atof.
#include <ctype.h>         // isdigit, isspace
#include <math.h>          // For NAN.

#include <avr/io.h>        // register name and bit name macros.

#include "DietSerial.h"    // includes DietSerial_macros.h as well

#include "WDTSecondTimer.h"     // used in rxwait for timeout.



// =============================================================================
// Main USART class begins.

void AVR_USART::begin(const unsigned long baudRequested)
{
    timeout_seconds_ = 90;  // default timeout 90 seconds

    unsigned long baud = baudRequested;

    if (baud < 300 ) {baud = 300;}  // 16 MHz system clock can't go below 300.

    // turn on the peripheral and configure it for 8N1 and selected BAUD.
    PRR    &= ~(1<<PRUSART0);

    // Data size 8 bits, Async mode, no parity and 1 stop bit.
    UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);
    // UMSEL01, UMSEL00 = 0 -> Asynchronous UART mode
    // UPM01, UPM00 = 0 -> No parity; USBS = 0 -> 1 stop bit.
    // UCPOL0 = 0 -> normal polarity.

    uint16_t baudreg = ((F_CPU / 4 / baud) - 1) / 2;

    if ((baudreg > 4095) || (baud == 57600 && F_CPU == 16000000UL))
    {
        UCSR0A &= ~(1<<U2X0);   // not USE_2X
        baudreg = ((F_CPU / 8 / baud) -1) / 2; // re-calculate registers.

    }
    else {UCSR0A |= (1<<U2X0);} // USE_2X except in the case above.

    UBRR0H = (uint8_t)(baudreg >> 8);
    UBRR0L = (uint8_t)baudreg;

    //Enable transmit and receive; disable interrupts.
    UCSR0B &= ~((1<<RXCIE0) | (1<<TXCIE0) | (1<<UDRIE0) | (1<<UCSZ02));
    UCSR0B |= (1<<RXEN0) | (1<<TXEN0);
}

void AVR_USART::end(void)
{
    UCSR0B = 0;             // turn off RXEN0 and TXEN0.
    PRR   |= 1<<PRUSART0;   // turn off the USART's clock.
}

// =============================================================================
// Basic receiving functions.  Blocking with timeout.
// Error codes are placed in receive_err_ for examination by error() - inline fn.

void    AVR_USART::printError(rxerr_t err)
{
    if (err) print(F("DietSerial: "));
    if (err == 1) print(F("error 1, Receive timed out."));
    if (err == 2) print(F("error 2, Data is garbled. Discard the character."));
    if (err == 4) print(F("error 4, Other type of error."));
    if (err == 8) print(F("error 8, The supplied buffer is too small. (CR-LF not received.)"));
}

// Main receive-a-byte function.
// ============================

uint8_t AVR_USART::readByte(void)
{
    receive_err_ = 0;        // Optimism: Assume no error.
    // If there was a data overrun, clear the USART's FIFO first.
    if (UCSR0A & (1<<DOR0))
    {
        do
            {volatile uint8_t dummy __attribute__((unused)) = UDR0;}
        while (UCSR0A & (1<<RXC0));
        UCSR0A &= ~(1<<DOR0);   // Reset data overrun flag.
    }

    // If there is a byte ready, return it.
    // (Else) wait for a character to arrive.
    if (UCSR0A & (1<<RXC0))
        return rxcompleted();
    /*Else*/ return rxwait();
}

uint8_t AVR_USART::rxcompleted()
{
    if ((UCSR0A & (1 << FE0)) || (UCSR0A & (1<<UPE0)))
        {receive_err_ = 2;}  // frame or  error: discard char.
    return UDR0;
}

uint8_t AVR_USART::rxwait()
{
    WDTSecondTimer rxTimer;  // RAII: exiting the function stops the timer.

    // Attempt at least 1 receive: do {} while()  not while() {}
    rxTimer.reset();
    do
    {
        if (UCSR0A & (1<<RXC0))
        {
            return rxcompleted();
        }
    }
    while (rxTimer.tick() < timeout_seconds_);
    receive_err_ = 1;     // timed out
    return 0x15;          // ASCII control character NAK, receive unsuccessful
}

// =============================================
// Multi-byte binary receive functions
// Array, numeric types int, long, float, double

size_t AVR_USART::readBytes(uint8_t* buf , const size_t bufLen)
{
    if (bufLen == 0) {receive_err_ = 8; return 0;} // error: buffer too short
    size_t i = 0;
    while (i < bufLen)
    {
        uint8_t newByte = readByte();
        if (receive_err_) return i;
        buf[i++] = newByte;
    };
    return i;
}

int AVR_USART::readInt()
{
    bytepair bp;
    bp.b[0] = readByte();
    if (receive_err_) return 0;
    bp.b[1] = readByte();
    if (receive_err_) return 0;
    return bp.i;
}

void AVR_USART::read4bytes(quadbyte& qb)
{
    for (auto& e : qb.b)
    {
        e = readByte();
        if (receive_err_) return;
    }
}

double AVR_USART::readDouble()
{
    quadbyte qb;
    qb.d = NAN;
    read4bytes(qb);
    return qb.d;
}

float   AVR_USART::readFloat()
{
    quadbyte qb ;
    qb.f = NAN;
    read4bytes(qb);
    return qb.f;
}

long   AVR_USART::readLong()
{
    quadbyte qb;
    qb.l  = 0;
    read4bytes(qb);
    return qb.l;
}


// Read ASCII.
// ==========

char    AVR_USART::read()
{
    return (char)readByte();
}

// Read a line of text (or null-terminated string). Return strlen.
size_t AVR_USART::readString(char* buf, const size_t bufLen)
{
    if (bufLen == 0) {receive_err_ = 8; return 0;} // error: buffer too short

    // Read and save incoming characters until end of line or end of string.
    receive_err_ = 0;
    size_t i = 0;
    while (i < bufLen)
    {
        char c = (char)readByte();
        if ((receive_err_ == 0) && (c < 0))
        {
            receive_err_ = 2;  // expecting an ASCII char 0..127
        }
        if (receive_err_) return i;
        if (c == '\r')   // first of \r\n end-of-line pair
        {
            c = '\0';
            readByte();  // clear the '\n'
        }
        else if (c == '\n')
        {
            c = '\0';
        }
        buf[i] = c;
        ++i;
        if (c == '\0')   return i;  // end of line or null in input.
    }

    // We are here only if i == bufLen. Ensure the last character is a null.
    buf[--i] = '\0';
    receive_err_ = 8;  // buffer too small.
    return ++i;
}

// Read until a null byte (end of string), or one of CR or LF,
// filtering characters according to LookaheadMode and the extra 'ignore' char.
// Filter out the LF if have received a CR before it.
// Place unfiltered chars in supplied array buf.  Return strlen.
// Purpose is extracting representations of numbers out of the incoming
// data.

// Do we want this character?
bool AVR_USART::wantChar(const char c,
                         const LookaheadMode mode,
                         const char unwanted)
{
    if (mode == SKIP_ALL)  // only use digits, +, -, ..
    {
        switch (c)
        {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
            case '-': case '+': case '.':
                return (c != unwanted);
            default:
                return false;
        };
    }
    if (mode == SKIP_WHITESPACE)
    {
        switch (c)
        {
            case ' ':
            case '\t':
            case '\v':
                return false;
            default:
                return (c != unwanted);
        };
    }
    // mode == SKIP_NONE.
    return (c != unwanted);
}

size_t AVR_USART::readStringExcept(char* buf, const size_t bufLen,
                               const LookaheadMode mode, const char ignore)
{
    if (bufLen == 0)
        {receive_err_ = 8; return 8;} // error: buffer too small

    char unwanted =  ignore;
    if  (unwanted == 0)     unwanted = (uint8_t)0x7f;
    if  (unwanted == '\r')  unwanted = (uint8_t)0x7f;
    if  (unwanted == '\n')  unwanted = (uint8_t)0x7f;

    // read and save incoming chars until end of line or end of string.
    receive_err_ = 0;

    size_t i = 0;
    char c = 0;

    while (((c = read()) != 0) && (i < bufLen))
    {
        if ((receive_err_) || (c < 0x20))  // is not a text char or space
            { buf[i] = 0; return ((i>0)? --i : i);}       // CR or LF or null
        if (wantChar(c, mode, unwanted))
            { buf[i] = c; ++i; }
    }

    if (i >= (bufLen-1))
    {
        if (i >= bufLen) {i = bufLen - 1;}
        buf[i] = 0;
        receive_err_ = 8;  // buffer too small.
        return i;
    }
    else if (i == 0) {buf[i] = 0; return i;}
    else {buf[i] = 0; return --i;}
}

// extracting numbers from incoming lines of text.

double AVR_USART::parseFloat(const LookaheadMode mode, const char ignore)
{
    char buf[20];
    for (auto& b : buf) b = '\0';
    size_t len = readStringExcept(buf, 20, mode, ignore);

    if ((len == 0) || (receive_err_))
        return NAN;
    return atof(buf);
}

long   AVR_USART::parseInt(const LookaheadMode mode, const char ignore)
{
    char buf[20];
    for (auto& b : buf) b = '\0';
    size_t len = readStringExcept(buf, 20, mode, ignore);
    if ((len == 0) || (receive_err_))
        return 0;
    return atol(buf);
}

// Extract numbers from already-received text in buffer buf.
// Warning: modifies the supplied buffer!!
double AVR_USART::parseFloat(char* buf, const size_t bufLen)
{
    LookaheadMode mode = SKIP_ALL;
    char ignore = 0x7F;
    int j = -1;
    for (size_t i = 0; i < bufLen; i++)
    {
        if (buf[i] == '\0') break; // end of string.

        if (wantChar(buf[i], mode, ignore))
        {
            ++j;
            if (j < (int)i) buf[j] = buf[i];  // j can never be greater than i.
        }
    }

    if (j > 0)
    {
        // null-terminate the filtered string
        ++j;
        if (j > (int)(bufLen - 1)){j = bufLen - 1;}
        buf[j] = '\0';
        return atof(buf);
    }
    else return NAN;
}

long   AVR_USART::parseInt(char* buf, const size_t bufLen)
{
    LookaheadMode mode = SKIP_ALL;
    char ignore = 0x7F;

    int j = -1;
    for (int i = 0; i < (int)bufLen; i++)
    {
        if (buf[i] == '\0') break; // end of string.

        if (wantChar(buf[i], mode, ignore))
        {
            ++j;
            if (j < i) buf[j] = buf[i];
        }
    }
    if (j > 0)
    {
        // null-terminate the filtered string
        ++j;
        if (j > (int)(bufLen - 1)){j = bufLen - 1;}
        buf[j] = '\0';
        return atol(buf);
    }
    else       return 0;
}

//==============================================================================
//
// Transmit.
//

void AVR_USART::txRaw(const uint8_t b)
    {UDR0 = b;}

void AVR_USART::write(const uint8_t b)
{
    loop_until_bit_is_set(UCSR0A, UDRE0); // wait till prev tx complete.
    UDR0 = b;
}

void AVR_USART::write(const uint8_t* buf, const size_t numBytes)
{
    if (!(buf) || (numBytes == 0)) return;
    for (size_t i = 0; i < numBytes; i++)
    {
        loop_until_bit_is_set(UCSR0A, UDRE0);
        UDR0 = buf[i];
    }
}

// Binary data types
void AVR_USART::write(const char c)
{
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = (uint8_t)c;
}

void AVR_USART::write4bytes(quadbyte& qb)
{
    write(qb.b[0]);
    write(qb.b[1]);
    write(qb.b[2]);
    write(qb.b[3]);
}
void AVR_USART::write(const double d)
{
    write4bytes((quadbyte&)d);
}

void AVR_USART::write(const float f)
{
    write4bytes((quadbyte&)f);
}

void AVR_USART::write(const int i)
{
    bytepair& bp = (bytepair&)i;
    write(bp.b[0]);
    write(bp.b[1]);
}

void AVR_USART::write(const long l)
{
  write4bytes((quadbyte&)l);
}

// Bytes are in program memory (flash):-
void AVR_USART::writeP(const uint8_t* data, const size_t numBytes)
{
    for (size_t i = 0; i < numBytes; i++) {write(pgm_read_byte(&data[i]));}
}


// Transmitting text strings.

void AVR_USART::print(const char* string)
// warning: Assumes string is properly terminated with a null 0 byte.
{
    if (!string) return;
    register int i = 0;
    while (string[i]) {write((uint8_t)string[i++]);}
}

// warning: Assumes string is properly terminated with a null 0 byte
// and has been declared with PROGMEM.
void AVR_USART::printP(const char * s_)
{
    if (!s_) return;
    size_t  i = 0;
    uint8_t c;
    while ((c = pgm_read_byte(&s_[i++])) != '\0') {write(c);}
}


//Numerical types.

// Print a byte as binary, fixed length, format: 0b0011 1011
// (Using itoa() with base 2 gives variable length results.)
void AVR_USART::printBinary(const uint8_t b)
// transmit a binary representation of the byte.
{
    write('0'); write('b');
    write(((b & 0x80))? '1':'0');
    write(((b & 0x40))? '1':'0');
    write(((b & 0x20))? '1':'0');
    write(((b & 0x10))? '1':'0');
    write(' ');
    write(((b & 0x08))? '1':'0');
    write(((b & 0x04))? '1':'0');
    write(((b & 0x02))? '1':'0');
    write(((b & 0x01))? '1':'0');
}

// Least significant four bits 0 - F (hex)
void AVR_USART::printDigit(uint8_t d)
    {d &= 0x0f; write((d < 10)? (d + '0'): (d - 10 + 'a'));}


// 8-bit special types
void AVR_USART::print(const bool b)
    {if (b) print("true"); else print("false");}

void AVR_USART::print(const char c)
    {write(c);}


// Floating-point types
void AVR_USART::print(const double df, const int decimals)
{
    char buf[30] = {0};
    dtostrf(df, 5, decimals, buf);
    print(buf);
}

void AVR_USART::print(const float f, const int decimals)
{
    print((double)f, decimals);
}

// integer types
void AVR_USART::print(const int i, const int base)
{
    print((long)i, base);
}

void AVR_USART::print(const long l, const int base)
{
    char buf[20] = {0};
    ltoa(l, buf, base);
    print(buf);
}


void AVR_USART::print(const uint8_t ub, const int base)
{
    char buf[10] = {0};
    utoa((unsigned int)ub, buf, base);
    print(buf);
}

void AVR_USART::print(const unsigned int ui, const int base)
{
    print((unsigned long)ui, base);
}

void AVR_USART::print(const unsigned long ul, const int base)
{
    char buf[20] = {0};
    ultoa(ul, buf, base);
    print(buf);
}


// Strings stored in flash with Arduino's F() macro: see WString.h
void AVR_USART::print(const __FlashStringHelper *fsh)
{
    if (!fsh) return;
    const char * buf = reinterpret_cast<const char *>(fsh);
    printP(buf);
}

// -------------
// println()

void AVR_USART::println(void)
    {print('\r'); print('\n');}

void AVR_USART::println(const bool b)
    {print((bool)b); println();}

void AVR_USART::println(const char c)
    {print(c); println();}

void AVR_USART::println(const uint8_t ub, const int base)
    {print(ub, base);println();}

void AVR_USART::println(const char* string)
    {print(string); println();}

void AVR_USART::println(const double d, const int decimals)
    {print(d, decimals); println();}

void AVR_USART::println(const float f, const int decimals)
    {print(f, decimals); println();}

void AVR_USART::println(const int i, const int base)
    {print(i, base); println();}

void AVR_USART::println(const long l, const int base)
    {print(l, base); println();}

void AVR_USART::println(const unsigned int ui, const int base)
    {print(ui, base); println();}

void AVR_USART::println(const unsigned long ul, const int base)
    {print(ul, base); println();}



void AVR_USART::printlnP(const char* fsbuf)
    {printP(fsbuf); println();}

void AVR_USART::println(const __FlashStringHelper *fsh)
    {print(fsh); println();}


// convenience functions for common characters

// whitespace
void AVR_USART::tab()     {write('\t');}
void AVR_USART::crlf()    {write('\r');write('\n');}
void AVR_USART::space()   {write(' ');}

// numbers and dates
void AVR_USART::colon()   {write(':');}
void AVR_USART::comma()   {write(',');}
void AVR_USART::dash()    {write('-');}
void AVR_USART::dot()     {write('.');}

void AVR_USART::minus()   {write('-');}
void AVR_USART::plus()    {write('+');}
void AVR_USART::equals()  {write('=');}

void AVR_USART::slash()   {write('/');}
void AVR_USART::star()    {write('*');}

void AVR_USART::lparen()  {write('(');}
void AVR_USART::rparen()  {write(')');}

void AVR_USART::langle()  {write('<');}
void AVR_USART::rangle()  {write('>');}

// punctuation
void AVR_USART::dollar()  {write('$');}
void AVR_USART::percent() {write('%');}

void AVR_USART::apos()    {write('\'');}
void AVR_USART::dquote()  {write('"');}
void AVR_USART::qmark()   {write('?');}
void AVR_USART::at()      {write('@');}
void AVR_USART::vbar()    {write('|');}


// The object:
struct AVR_USART DietSerial;
