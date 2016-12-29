#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system.h"

// debug statement:
//#define DEBUG_MODE

#ifdef DEBUG_MODE
    #define dprintf(format, ...) printf((format), __VA_ARGS__);
#else
    #define dprintf(...)
#endif

static char* RAM;

#define L 0
#define H 1
// NOTE: the high and low registers are _ONLY_ for the lower m16 byte like normal CPUs
typedef union REG32
{
    char m8[2];
    short m16;
    int m32;
}REG32;

static REG32 REGISTERS[REGISTER_COUNT];

#define REG8_OFFSET AL
// 8 bit sub-register pointers
static char* REG8[] =
{
    &REGISTERS[EAX].m8[L],
    &REGISTERS[EAX].m8[H],
    &REGISTERS[EBX].m8[L],
    &REGISTERS[EBX].m8[H],
    &REGISTERS[ECX].m8[L],
    &REGISTERS[ECX].m8[H],
    &REGISTERS[EDX].m8[L],
    &REGISTERS[EDX].m8[H]
};

#define REG16_OFFSET AX
// 16 bit sub-register pointers
static short* REG16[] =
{
    &REGISTERS[EAX].m16,
    &REGISTERS[EBX].m16,
    &REGISTERS[ECX].m16,
    &REGISTERS[EDX].m16
};

int readROM()
{
    FILE* file = fopen("ROM", "r");
    if(!file) return -1;

    fseek(file, 0L, SEEK_END);
    unsigned long size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    if(size > RAM_SIZE) {
        printf("ROM too large! Maximum RAM of %i!\n", RAM_SIZE);
        return -1;
    }

    fread(&REGISTERS[EIP].m32, sizeof(int), 1, file);

    if(fread(RAM, 1, size - 4, file) != size - 4) return -1;
    REGISTERS[ESP].m32 += size;

    fclose(file);
    return 0;
}

static inline char read_b() { return RAM[REGISTERS[EIP].m32++]; }
static inline int read_s(){ return RAM[REGISTERS[EIP].m32++] + (RAM[REGISTERS[EIP].m32++] << 8); }
static inline int read_i(){ return RAM[REGISTERS[EIP].m32++] + (RAM[REGISTERS[EIP].m32++] << 8) + (RAM[REGISTERS[EIP].m32++] << 16) + (RAM[REGISTERS[EIP].m32++] << 24); }

int main()
{
    // allocate the heap
    RAM = (char*) malloc(RAM_SIZE);
    if(!RAM) return -1;
    memset(RAM, 0, RAM_SIZE); // 0 - initalize the memory

    if(readROM() != 0)
    {
        puts("ROM reading exception.");
        return -1;
    }

    int RUNNING = 1;
    char OP, v0, v1;
    while(RUNNING)
    {
        // increment the stack pointer for the next bit
        OP = read_b();

        switch(OP)
        {
            case INT:
                v0 = read_b();
                switch(v0)
                {
                    case EXIT:
                        RUNNING = 0;
                        break;

                    case PRINT_INT:
                        printf("%i", REGISTERS[EAX].m32);
                        break;

                    case PRINT_CHAR:
                        printf("%c", (char) REGISTERS[EAX].m32);
                        break;

                    default:
                        printf("VM Crash: Bad interrupt [%i]\n", v0);
                        return -1;
                }
                break;

            case MOV:
                v0 = RAM[REGISTERS[EIP].m32++];
                switch(v0)
                {
                    case AL: REGISTERS[EAX].m8[L] = read_b(); break;
                    case BL: REGISTERS[EBX].m8[L] = read_b(); break;
                    case CL: REGISTERS[ECX].m8[L] = read_b(); break;
                    case DL: REGISTERS[EDX].m8[L] = read_b(); break;

                    case AH: REGISTERS[EAX].m8[H] = read_b(); break;
                    case BH: REGISTERS[EBX].m8[H] = read_b(); break;
                    case CH: REGISTERS[ECX].m8[H] = read_b(); break;
                    case DH: REGISTERS[EDX].m8[H] = read_b(); break;

                    case AX: REGISTERS[EAX].m16 = read_s(); break;
                    case BX: REGISTERS[EBX].m16 = read_s(); break;
                    case CX: REGISTERS[ECX].m16 = read_s(); break;
                    case DX: REGISTERS[EDX].m16 = read_s(); break;

                    case EAX: REGISTERS[EAX].m32 = read_i(); break;
                    case EBX: REGISTERS[EBX].m32 = read_i(); break;
                    case ECX: REGISTERS[ECX].m32 = read_i(); break;
                    case EDX: REGISTERS[EDX].m32 = read_i(); break;
                    case ESB: REGISTERS[ESB].m32 = read_i(); break;
                    case ESP: REGISTERS[ESP].m32 = read_i(); break;
                    case ESI: REGISTERS[ESI].m32 = read_i(); break;
                    case EDI: REGISTERS[EDI].m32 = read_i(); break;

                    default:
                        printf("VM Crash: Bad move destination register [%i]\n", v0);
                        return -1;
                }
                break;

            case CPY:
                v0 = read_b();
                v1 = read_b();
                switch(v0) {
                    case AL:
                    case BL:
                    case CL:
                    case DL:
                    case AH:
                    case BH:
                    case CH:
                    case DH:
                        if(v1 < AL || v1 > DH) { // check if both registers are the same bit types
                            printf("VM Crash: Invalid copy operation operands: [%i] and [%i].\n", v0, v1);
                            return -1;
                        }
                        *REG8[v0 - REG8_OFFSET] = *REG8[v1 - REG8_OFFSET];
                        break;

                    case AX:
                    case BX:
                    case CX:
                    case DX:
                        if(v1 < AX || v1 > DX) { // check if both registers are the same bit types
                            printf("VM Crash: Invalid copy operation operands: [%i] and [%i].\n", v0, v1);
                            return -1;
                        }
                        *REG16[v0 - REG16_OFFSET] = *REG16[v1 - REG16_OFFSET];
                        break;

                    case EAX:
                    case EBX:
                    case ECX:
                    case EDX:
                    case ESP:
                    case ESI:
                        if(v1 < EAX || v1 > EDI) { // check if both registers are the same bit types
                            printf("VM Crash: Invalid copy operation operands: [%i] and [%i].\n", v0, v1);
                            return -1;
                        }
                        REGISTERS[v0].m32 = REGISTERS[v1].m32;
                        break;

                    default:
                        printf("VM Crash: Bad copy source register [%i].\n", v0);
                        return -1;
                }
                break;

            case INC:
                v0 = RAM[REGISTERS[EIP].m32++];
                switch(v0)
                {
                    case AL:
                    case BL:
                    case CL:
                    case DL:
                    case AH:
                    case BH:
                    case CH:
                    case DH:
                        REG8[v0 - REG8_OFFSET] += read_b(); // 8 bit mode - add 1 byte
                        break;

                    case AX:
                    case BX:
                    case CX:
                    case DX:
                        REG16[v0 - REG16_OFFSET] += read_s(); // 16 bit mode - add 1 short
                        break;

                    case EAX:
                    case EBX:
                    case ECX:
                    case EDX:
                    case ESP:
                    case ESI:
                    case EDI:
                        REGISTERS[v0].m32 += read_i(); // 32 bit mode - add 1 int
                        break;

                    default:
                        printf("VM Crash: Bad increment register [%i]\n", v0);
                        return -1;
                }
                break;
                
            case DEC:
                v0 = read_b();
                switch(v0)
                {
                    case AL:
                    case BL:
                    case CL:
                    case DL:
                    case AH:
                    case BH:
                    case CH:
                    case DH:
                        REG8[v0 - REG8_OFFSET] -= read_b();
                        break;

                    case AX:
                    case BX:
                    case CX:
                    case DX:
                        REG16[v0 - REG16_OFFSET] -= read_s();
                        break;

                    case EAX:
                    case EBX:
                    case ECX:
                    case EDX:
                    case ESP:
                    case ESI:
                    case EDI:
                        REGISTERS[v0].m32 -= read_i();
                        break;

                    default:
                        printf("VM Crash: Bad increment register [%i]\n", v0);
                        return -1;
                }
                break;

            case ADD:
                v0 = read_b();
                v1 = read_b();
                switch(v0) {
                    case AL:
                    case BL:
                    case CL:
                    case DL:
                    case AH:
                    case BH:
                    case CH:
                    case DH:
                        if(v1 < AL || v1 > DH) {
                            printf("VM Crash: Invalid add operation operands: [%i] and [%i].\n", v0, v1);
                            return -1;
                        }
                        *REG8[v0 - REG8_OFFSET] += *REG8[v1 - REG8_OFFSET];
                        break;

                    case AX:
                    case BX:
                    case CX:
                    case DX:
                        if(v1 < AX || v1 > DX) {
                            printf("VM Crash: Invalid add operation operands: [%i] and [%i].\n", v0, v1);
                            return -1;
                        }
                        *REG16[v0 - REG16_OFFSET] += *REG16[v1 - REG16_OFFSET];
                        break;

                    case EAX:
                    case EBX:
                    case ECX:
                    case EDX:
                    case ESP:
                    case ESI:
                    case EDI:
                        if(v1 < EAX || v1 > EDI) {
                            printf("VM Crash: Invalid add operation operands: [%i] and [%i].\n", v0, v1);
                            return -1;
                        }
                        REGISTERS[v0].m32 += REGISTERS[v1].m32;
                        break;

                    default:
                        printf("VM Crash: Bad add source register [%i].\n", v0);
                        return -1;
                }
                break;

            case SUB:
                v0 = read_b();
                v1 = read_b();
                printf("SUB %i %i\n", v0, v1);
                switch(v0) {
                    case AL:
                    case BL:
                    case CL:
                    case DL:
                    case AH:
                    case BH:
                    case CH:
                    case DH:
                        if(v1 < AL || v1 > DH) {
                            printf("VM Crash: Invalid sub operation operands: [%i] and [%i].\n", v0, v1);
                            return -1;
                        }
                        *REG8[v0 - REG8_OFFSET] -= *REG8[v1 - REG8_OFFSET];
                        break;

                    case AX:
                    case BX:
                    case CX:
                    case DX:
                        if(v1 < AX || v1 > DX) {
                            printf("VM Crash: Invalid sub operation operands: [%i] and [%i].\n", v0, v1);
                            return -1;
                        }
                        *REG16[v0 - REG16_OFFSET] -= *REG16[v1 - REG16_OFFSET];
                        break;

                    case EAX:
                    case EBX:
                    case ECX:
                    case EDX:
                    case ESP:
                    case ESI:
                    case EDI:
                        if(v1 < EAX || v1 > EDI) {
                            printf("VM Crash: Invalid sub operation operands: [%i] and [%i].\n", v0, v1);
                            return -1;
                        }
                        REGISTERS[v0].m32 -= REGISTERS[v1].m32;
                        break;

                    default:
                        printf("VM Crash: Bad sub source register [%i].\n", v0);
                        return -1;
                }
                break;

            case JMP:
                REGISTERS[EIP].m32 = read_i();
                break;

            case JEQ:
                v1 = read_i();
                if(REGISTERS[FLG].m32 & EQ_FLAG) REGISTERS[EIP].m32 = v1;
                break;

            case JGE:
                v1 = read_i();
                if(REGISTERS[FLG].m32 & GT_FLAG) REGISTERS[EIP].m32 = v1;
                break;

            case JLE:
                v1 = read_i();
                if(REGISTERS[FLG].m32 & LS_FLAG) REGISTERS[EIP].m32 = v1;
                break;

            case CMP:
                v0 = read_b();
                v1 = read_b();
                REGISTERS[FLG].m32 = 0; // clear the flags
                switch(v0) {
                    case AL:
                    case BL:
                    case CL:
                    case DL:
                    case AH:
                    case BH:
                    case CH:
                    case DH:
                        if(v1 < AL || v1 > DH) {
                            printf("VM Crash: Invalid cmp operation operands: [%i] and [%i].\n", v0, v1);
                            return -1;
                        }
                        v0 = *REG8[v0 - REG8_OFFSET] - *REG8[v1 - REG8_OFFSET];
                        if(v0 == 0) REGISTERS[FLG].m32 |= EQ_FLAG;
                        else if(v0 < 0) REGISTERS[FLG].m32 |= LS_FLAG;
                        else REGISTERS[FLG].m32 |= GT_FLAG;
                        break;

                    case AX:
                    case BX:
                    case CX:
                    case DX:
                        if(v1 < AX || v1 > DX) {
                            printf("VM Crash: Invalid cmp operation operands: [%i] and [%i].\n", v0, v1);
                            return -1;
                        }
                        v0 = *REG16[v0 - REG16_OFFSET] - *REG16[v1 - REG16_OFFSET];
                        if(v0 == 0) REGISTERS[FLG].m32 |= EQ_FLAG;
                        else if(v0 < 0) REGISTERS[FLG].m32 |= LS_FLAG;
                        else REGISTERS[FLG].m32 |= GT_FLAG;
                        break;

                    case EAX:
                    case EBX:
                    case ECX:
                    case EDX:
                    case ESP:
                    case ESI:
                    case EDI:
                        if(v1 < EAX || v1 > EDI) {
                            printf("VM Crash: Invalid cmp operation operands: [%i] and [%i].\n", v0, v1);
                            return -1;
                        }
                        v0 = REGISTERS[v0].m32 - REGISTERS[v1].m32;
                        if(v0 == 0) REGISTERS[FLG].m32 |= EQ_FLAG;
                        else if(v0 < 0) REGISTERS[FLG].m32 |= LS_FLAG;
                        else REGISTERS[FLG].m32 |= GT_FLAG;
                        break;

                    default:
                        printf("VM Crash: Bad cmp source register [%i].\n", v0);
                        return -1;
                }
                break;

            case PUSH:
                v0 = read_b();
                switch(v0)
                {
                    case AL:
                    case BL:
                    case CL:
                    case DL:
                    case AH:
                    case BH:
                    case CH:
                    case DH:
                        RAM[REGISTERS[ESP].m32] = *REG8[v0 - REG8_OFFSET];
                        REGISTERS[ESP].m32 ++;
                        break;

                    case AX:
                    case BX:
                    case CX:
                    case DX:
                        memcpy(&RAM[REGISTERS[ESP].m32], &REG16[v0 - REG16_OFFSET], sizeof(short));
                        REGISTERS[ESP].m32 += 2;
                        break;

                    case EAX:
                    case EBX:
                    case ECX:
                    case EDX:
                    case ESB:
                    case ESP:
                    case ESI:
                    case EDI:
                        memcpy(&RAM[REGISTERS[ESP].m32], &REGISTERS[v0].m32, sizeof(int));
                        REGISTERS[ESP].m32 += 4;
                        break;

                    default:
                        printf("VM Crash: Invalid push register [%i].\n", v0);
                        break;
                }
                break;

            case POP:
                v0 = read_b();
                switch(v0)
                {
                    case AL:
                    case BL:
                    case CL:
                    case DL:
                    case AH:
                    case BH:
                    case CH:
                    case DH:
                        *REG8[v0 - REG8_OFFSET] = RAM[REGISTERS[ESP].m32];
                        REGISTERS[ESP].m32 --;
                        break;

                    case AX:
                    case BX:
                    case CX:
                    case DX:
                        memcpy(&REG16[v0 - REG16_OFFSET], &RAM[REGISTERS[ESP].m32], sizeof(short));
                        REGISTERS[ESP].m32 -= 2;
                        break;

                    case EAX:
                    case EBX:
                    case ECX:
                    case EDX:
                    case ESB:
                    case ESP:
                    case ESI:
                    case EDI:
                        memcpy(&REGISTERS[v0].m32, &RAM[REGISTERS[ESP].m32], sizeof(int));
                        REGISTERS[ESP].m32 -= 4;
                        break;

                    default:
                        printf("VM Crash: Invalid pop register [%i].\n", v0);
                        break;
                }
                break;

            case WRITE:
                v0 = read_b();
                switch(v0)
                {
                    case AL:
                    case BL:
                    case CL:
                    case DL:
                    case AH:
                    case BH:
                    case CH:
                    case DH:
                        RAM[REGISTERS[EDI].m32] = *REG8[v0 - REG8_OFFSET];
                        break;

                    case AX:
                    case BX:
                    case CX:
                    case DX:
                        memcpy(&RAM[REGISTERS[EDI].m32], &REG16[v0 - REG16_OFFSET], sizeof(short));
                        break;

                    case EAX:
                    case EBX:
                    case ECX:
                    case EDX:
                    case ESB:
                    case ESP:
                    case ESI:
                    case EDI:
                        memcpy(&RAM[REGISTERS[EDI].m32], &REGISTERS[v0].m32, sizeof(int));
                        break;

                    default:
                        printf("VM Crash: Invalid set operation register [%i].\n", v0);
                        break;
                }
                break;

            case FETCH:
                v0 = read_b();
                switch(v0)
                {
                    case AL:
                    case BL:
                    case CL:
                    case DL:
                    case AH:
                    case BH:
                    case CH:
                    case DH:
                        *REG8[v0 - REG8_OFFSET] = RAM[REGISTERS[ESI].m32];
                        break;

                    case AX:
                    case BX:
                    case CX:
                    case DX:
                        memcpy(&REG16[v0 - REG16_OFFSET], &RAM[REGISTERS[ESI].m32], sizeof(short));
                        break;

                    case EAX:
                    case EBX:
                    case ECX:
                    case EDX:
                    case ESB:
                    case ESP:
                    case ESI:
                    case EDI:
                        memcpy(&REGISTERS[v0].m32, &RAM[REGISTERS[ESI].m32], sizeof(int));
                        break;

                    default:
                        printf("VM Crash: Invalid get operation register [%i].\n", v0);
                        break;
                }
                break;

            defualt:
                printf("VM Crash: Invalid op-code [%i].\n", v0);
                return -1;
        }
    }

    return 0;
}

