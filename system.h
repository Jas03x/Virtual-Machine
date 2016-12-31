#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// definitions
#define RAM_SIZE 65536 // 65536 bytes of ram

// OP-CODES:
#define NOP     0  // no operation
#define INT     1  // interupt
#define MOV     2  // move value to register
#define CPY     3  // copy register to register
#define ADD     4  // add
#define INC     5  // increment
#define DEC     6  // decrement
#define SUB     7  // subtract
#define CMP     8  // compare
#define JMP     9  // jump
#define JEQ    10  // jump if equal
#define JLE    11  // jump if less than
#define JGE    12  // jump if greater than
#define PUSH   13  // push onto the stack
#define POP    14  // pop the stack
#define FETCH  15  // access at
#define WRITE  16  // set at
#define CALL   17  // call
#define RET    18  // return to the last call's location

// Instruction opcode specifications:
// NOP   - NA
// INT   - byte
// MOV   - byte [byte, short, or int depending on register]
// CPY   - byte byte
// ADD   - byte byte
// INC   - byte [byte, short, or int depending on register]
// DEC   - byte [byte, short, or int depending on register]
// SUB   - byte byte
// CMP   - byte byte
// JMP   - int
// JEQ   - int
// JLE   - int
// JGE   - int
// PUSH  - byte
// POP   - byte
// FETCH - byte
// WRITE - byte
// CALL  - int
// RET   - NA

// Instruction explanations:
// NOP - do nothing
// INT - interrupt execution and make the machine do a built-in operation
// MOV - move the specified value to the register
// CPY - copy a register onto another register
// ADD - add a register to another
// SUB - subtract a register from another
// INC - increment a register by a vlue
// DEC - decrement a register by a value
// CMP - compare two registers
// JMP - jump to the specified location unconditionally
// JEQ - jump to the specified location if the last comparison was true
// JLE - jump to the specified location if the last comparison set the less flag
// JGE - jump to the specified location if the last comparison set the greater flag
// PUSH - push the registers value onto the stack
// POP - pop the registers value
// FETCH - get the value from the stack onto the registers (uses the ESI register)
// WRITE - write the register's value onto the stack (uses the EDI register)
// CALL - jump to the location in memory and then continue executing from this function call
// RET - jump to the next address in the stack

// ROM setup:
// Code segment integer (the byte at which the code begins)
// Data (if any)....
// ------------------------------
// Code...
// ...

// note: 8 bit and 16 bit support exists for chars and shorts
// due to this, you can have all these data types:
// byte, short, int, float

// general 8 bit registers (breakdown of 16 bit registers)
#define AL 10 // CONTINUES FROM ESI
#define AH 11
#define BL 12
#define BH 13
#define CL 14
#define CH 15
#define DL 16
#define DH 17

// general 16 bit registers
#define AX 18
#define BX 19
#define CX 20
#define DX 21

// general 32 bit registers
#define EAX 0 // accumulator
#define EBX 1 // other
#define ECX 2 // other
#define EDX 3 // program counter

// special registers
// These are all full 32 bit integers since they are indices
#define ESB 4 // stack base pointer; points to the bottom of the stack
#define ESP 5 // stack pointer; points to the top of the stack
#define ESI 6 // stack source index
#define EDI 7 // stack destination index
#define EIP 8 // program counter; read only register
#define FLG 9 // flag register
#define REGISTER_COUNT 10

// flags
#define EQ_FLAG 1 // EQUAL FLAG
#define GT_FLAG 2 // GREATER FLAG
#define LS_FLAG 4 // LESS FLAG

// interupt options:
#define EXIT 1
#define PRINT_INT  2
#define PRINT_CHAR 3
#define READ_DISK  4

// NOTE: This explains the read disk interrupt:
// This interrupt will read the disk and push the data onto the stack
// The following registers are used by this interrupt:
// EAX - Mark the start of the location of the hard drive to read
// EBX - Mark the end of the location of the hard drive to read

// assembly data types:
// Strings - byte data: "..."
// Defines: #def name value
// Arrays: <_type_>[e0, e1, ....] where the elements are seperated by commas
// 
// Byte - 8 bits
// Short - 16 bits
// Integer - 32 bits
// Float - 32 bits
//
// Example of an array of shorts:
// my_array: <short>[1, 2, 3, 4]
//

#endif
