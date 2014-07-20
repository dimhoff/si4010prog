Cypress FX2 C2 Bus Master Implementation
----------------------------------------
This directory contains code to use a Cypress EZ-USB FX2 micro controller as a
C2 bus interface. This code only implements the low level basic functions of a
C2 master. The software is controlled through the FX2 USB interface.

To utilize the C2 Bus master from the host PC use the functions provided in
../../src/c2_hw_fx2.c.

Wiring
------
You have to make the following connections between the SI4010 and the FX2:

GND		-> GND
GPIO5(/C2CLK)	-> IOB.1
GPIO7(/C2DAT)	-> IOB.0