#include <Arduino.h>
// Test the convenience functions for printing common characters

#include "DietSerial.h"

// Shorthand for the DietSerial object:
#define ds DietSerial

void setup() {
    ds.begin(9600);

    ds.println(F("Test of convenient single-character print functions in DietSerial."));
    ds.println();

    // Shorthand printing functions:
    ds.print("slash: "); ds.slash();
    ds.print("comma: "); ds.comma();
    ds.print("colon: "); ds.colon();
    ds.print("dash:  "); ds.dash();
    ds.print("minus: "); ds.minus();
    ds.print("dot:   "); ds.dot();
    ds.print("star:  "); ds.star();
    ds.print("dquote:"); ds.dquote();
    ds.print("tab:   "); ds.tab();
    ds.print("dollar:"); ds.dollar();
    ds.print("apos:  "); ds.apos();
    ds.print("lparen:"); ds.lparen();
    ds.print("rparen:"); ds.rparen();
    ds.print("crlf:  "); ds.crlf();
    ds.print("langle:"); ds.langle();
    ds.print("rangle:"); ds.rangle();
    ds.print("at:    "); ds.at();
    ds.print("vbar:  "); ds.vbar();
    ds.print("qmark:");  ds.qmark();

    ds.println(F("End of DietSerial shorthand demonstration."));
    ds.flush();
}

void loop() {}
