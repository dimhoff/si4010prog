SI4010 Programmer/Debugger
--------------------------
si4010prog is a tool to program and debug the Silicon Labs SI4010 micro
controller.

C2 Bus Master
-------------
The si4010prog program requires some sort of C2 Bus master hardware to interact
with the C2 bus. Currently a C2 Bus master implementation for the Cypress FX2
micro controller is used by default. Although a driver for Linux 2.4/2.6 is also
available that uses the LPT port as C2 bus master.

See the drivers/ directory for the available bus masters.

Known Issues
------------
- Currently there is no way to check if the micro controller is running. 
- Issuing a halt command when micro controller already halted cause protocol
  break. This can only be fixed by doing a 'reset'.

Usage Examples
--------------
- Identify device

        # si4010prog identify
        Device ID: 0x24; Revision ID: 0x02

- Load program into xram and run:

        # si4010prog reset prg:test.ihx run
        Resetting SI4010
        Programming CODE memory using "test.ihx"
        Resuming SI4010 MCU

- Halt running process, get program counter, and dump 16 byte at address 0x08

        # si4010prog halt getpc dram:0x8,16
        Halting SI4010 MCU
        Getting program counter:
        0x01f1
        Dumping 16 bytes of RAM at 0x08:
          0x0000 01daa1a2a3000000 0000000000000000                                      ................

