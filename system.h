#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// definitions
#define RAM_SIZE 2048 // 2048 integers for ram

// OP-CODES:
#define NOP   0  // no operation
#define INT   1  // interupt
#define MOV   2  // move value to register
#define CPY   3  // copy register to register
#define ADD   4  // add
#define INC   5  // increment
#define DEC   6  // decrement
#define SUB   7  // subtract
#define CMP   8  // compare
#define JMP   9  // jump
#define JEQ  10  // jump if equal
#define JLE  11  // jump if less than
#define JGE  12  // jump if greater than
#define PUSH 13  // push onto the stack
#define POP  14  // pop the stack
#define GET  15  // access at
#define SET  16  // set at

// register indices
#define EAX 0 // accumulator
#define EBX 1 // other 
#define ECX 2 // other
#define EDX 3 // program counter
#define ESP 4 // stack pointer
#define REGISTER_COUNT 5

// flags
#define EQ_FLAG 1 // EQUAL FLAG
#define GT_FLAG 2 // GREATER FLAG
#define LS_FLAG 4 // LESS FLAG

// interupt options:
#define EXIT 1
#define PRINT_INT  2
#define PRINT_CHAR 3

#endif
