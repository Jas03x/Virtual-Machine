#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "system.h"

// debug statement:
//#define DEBUG_MODE

#ifdef DEBUG_MODE
    #define dprintf(format, ...) printf((format), __VA_ARGS__);
#else
    #define dprintf(...)
#endif

static int* RAM;

// User use registers:
static int REGISTERS[REGISTER_COUNT];

// CPU only registers
static int PC = 0; // program counter; the address of the next instruction
static int SP = 0; // stack pointer; the pointer to the top of the stack
// WE WILL NEED SOME STATUS REGISTER OR SOMETHING TOO

static uint8_t FLAGS; // 8 bit flag register

int readROM()
{
    FILE* file = fopen("ROM", "r");
    if(!file) return -1;

    fseek(file, 0L, SEEK_END);
    unsigned long size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    if(size > RAM_SIZE * sizeof(int)) {
        printf("ROM too large! Maximum RAM of %i!\n", RAM_SIZE);
        return -1;
    }

    if(fread(RAM, 1, size, file) != size) return -1;
    SP += size / sizeof(int);

    fclose(file);
    return 0;
}

int main()
{
    // allocate the heap
    RAM = (int*) malloc(RAM_SIZE);
    if(!RAM) return -1;

    if(readROM() != 0)
    {
        puts("ROM reading exception.");
        return -1;
    }

    // START EXECUTION:
    puts("VM STARTING.");

    PC = RAM[0];
    dprintf("Execution starting at %i\n", PC);
    SP = 0;
    int RUNNING = 1;
    while(RUNNING)
    {
        // increment the stack pointer for the next bit
        int OP = RAM[PC++];
        int v0, v1;

        dprintf("%i: ", OP);

        switch(OP)
        {
            case INT:
                v0 = RAM[PC++];
                dprintf("INT %i\n", v0);
                switch(v0)
                {
                    case EXIT:
                        RUNNING = 0;
                        break;

                    case PRINT_INT:
                        printf("%i", REGISTERS[EBX]);
                        break;

                    case PRINT_CHAR:
                        //printf("Try to print %i\n", REGISTERS[EBX]);
                        printf("%c", (char) REGISTERS[EBX]);
                        break;
                }
                break;

            case MOV:
                v0 = RAM[PC++];
                v1 = RAM[PC++];
                dprintf("MOV %i %i\n", v0, v1);
                REGISTERS[v0] = v1;
                break;

            case CPY:
                v0 = RAM[PC++];
                v1 = RAM[PC++];
                dprintf("CPY %i %i\n", v0, v1);
                REGISTERS[v0] = REGISTERS[v1];
                break;

            case INC:
                v0 = RAM[PC++];
                v1 = RAM[PC++];
                dprintf("INC %i %i\n", v0, v1);
                REGISTERS[v0] += v1;
                break;

            case DEC:
                v0 = RAM[PC++];
                v1 = RAM[PC++];
                dprintf("DEC %i %i\n", v0, v1);
                REGISTERS[v0] -= v1;
                break;

            case ADD:
                v0 = RAM[PC++];
                v1 = RAM[PC++];
                dprintf("ADD %i %i\n", v0, v1);
                REGISTERS[v0] += REGISTERS[v1];
                break;

            case SUB:
                v0 = RAM[PC++];
                v1 = RAM[PC++];
                dprintf("SUB %i %i\n", v0, v1);
                REGISTERS[v0] -= REGISTERS[v1];
                break;

            case JMP:
                v0 = RAM[PC++];
                PC = v0;
                dprintf("JMP %i\n", v0);
                break;

            case JEQ:
                v0 = RAM[PC++];
                if(FLAGS & EQ_FLAG) PC = v0;
                dprintf("JEQ %i\n", v0);
                break;

            case JGE:
                v0 = RAM[PC++];
                if(FLAGS & GT_FLAG) PC = v0;
                dprintf("JGE %i\n", v0);
                break;

            case JLE:
                v0 = RAM[PC++];
                if(FLAGS & LS_FLAG) PC = v0;
                dprintf("JLE %i\n", v0);
                break;

            case CMP:
                v0 = RAM[PC++];
                v1 = RAM[PC++];

                FLAGS = 0; // clear the flags
                dprintf("CMP %i %i -> %i vs %i\n", v0, v1, REGISTERS[v0], REGISTERS[v1]);

                if(REGISTERS[v0] == REGISTERS[v1]) FLAGS |= EQ_FLAG;
                else if(REGISTERS[v0] < REGISTERS[v1]) FLAGS |= LS_FLAG;
                else if(REGISTERS[v0] > REGISTERS[v1]) FLAGS |= GT_FLAG;
                break;

            case PUSH:
                v0 = RAM[PC++];
                RAM[SP++] = REGISTERS[v0];
                dprintf("PUSH %i\n", v0);
                break;

            case POP:
                v0 = RAM[PC++];
                REGISTERS[v0] = RAM[SP--];
                dprintf("POP %i\n", v0);
                break;

            case GET:
                v0 = RAM[PC++];
                v1 = RAM[PC++];
                REGISTERS[v0] = RAM[REGISTERS[v1]];
                dprintf("GET %i %i -> get %i\n", v0, v1, RAM[REGISTERS[v1]]);
                break;

            case SET:
                v0 = RAM[PC++];
                v1 = RAM[PC++];
                RAM[REGISTERS[v0]] = REGISTERS[v1];
                dprintf("SET %i %i\n", v0, v1);
                break;

            defualt:
                break;
        }

        // dump information:
        //scanf("%*c");
    }

    puts("Successful termination");
    return 0;
}

