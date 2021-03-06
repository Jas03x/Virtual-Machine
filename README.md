# Virtual-Machine

A pseudo 32 bit virtual machine just for fun/experimentation. This is mainly a CPU-like 2d graphics/IO machine.

There are 4 parts so far:
  1. Machine: The virtual machine itself
  2. Compiler: This compiler compiles assembly files into code readable by the machine.
  3. Decompiler: This decompiles the machine readable code generated by the compiler into human readable assembly code. (Not updated for the rework yet).
  4. Analyzer: A tool for debugging ROM files.

Very experimental and untested right now. Things will change a lot as this project progresses.

---------------------------------------------------------------------------------------------------------------------------
# System information
This is the system information (registers, assembly operations, etc.). All this can be found in the system.h header.

#####Registers:
These are modeled very similarly to Intel CPUs

  * 32 bit General purpose: EAX, EBX, ECX, EDX
  * 16 bit registers: AX - DX: the 16 bits of the 32 bit general register
  * 8 bit registers: AL - DL: the lower 8 bits of the 16 bit general registers
  * 8 bit registers: AH - DH: the higher 8 bits of the 16 bit general registers
  
  * Special registers (each 32 bit registers):
  * ESB: The stack base pointer
  * ESP: The stack top pointer
  * ESI: The source index pointer
  * EDI: The destination index pointer
  * EIP: The instruction pointer
  * FLG: The flag register
  
#####RAM:
This machine uses 65536 bytes of program memory.

#####Assembly operations:
  * NOP - Do nothing
  * INT i - Interupt with code i
  * MOV r i - Store the value i in r
  * CPY r0 r1 - copy r1's value to r0
  * ADD r0 r1 - add r1's value to r0
  * INC r i - add i to r
  * DEC r i - subtract i from r
  * SUB r0 r1 - substrct r1 from r0
  * CMP r0 r1 - compare the value of r0 to r1
  * JMP i - jump to the the i'th byte
  * JEQ i - if the last CMP operation operands were equal, jump to the i'th byte
  * JLE i - if the last CMP operation's r0 operand was less than r1, jump to the i'th byte
  * JGE i - if the last CMP operation's r0 operand was greater than r1, jump to the i'th byte
  * PUSH r - push r's value onto the stack
  * POP r - pop the stack onto r
  * FETCH r0 r1 - read the stack at r1 and store in r0
  * WRITE r0 r1 - overwrite the stack at r0 with r1
  * CALL i - jump to the i'th location in memory
  * RET - return to the address stored at the stack top
  
#####Interrupt options:
  - 1 = Exit
  - 2 = Print integer
  - 3 = Print character
  - 4 = Read disk (more information below)

---------------------------------------------------------------------------------------------------------------------------
# File I/O:
The virtual hard drive is the “DRIVE” file. All the virtual disk partitions, data, etc are stored in this file only.

---------------------------------------------------------------------------------------------------------------------------
# Examples:
* function.asm - A simple example which uses the call/ret codes and the stack
* program.asm - A simple hello world example
* add_array.asm - A program which reads shorts off the program memory and displays their sum
* read_hd.asm - A program which reads the DRIVE file and prints the first 6 bytes