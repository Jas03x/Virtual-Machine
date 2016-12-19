#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system.h"

int main(int argc, char* argv[])
{
    if(argc != 3) {
        puts("Error: Expected 3 arguments.");
        return -1;
    }

    FILE* input = fopen(argv[1], "r");
    if(!input) {
        printf("Error: Could not open file [%s] for reading.\n", argv[1]);
        return -1;
    }

    FILE* output = fopen(argv[2], "w");
    if(!output) {
        printf("Error: Could not open file [%s] for writing.\n", argv[2]);
        return -1;
    }

    int code_start;
    fread(&code_start, sizeof(int), 1, input);


    // 2 temporary use integers:
    int i = 0;
    int e = 0;

    printf("{");
    if(code_start > 0) fprintf(output, "{");

    for(; i < code_start; i++) {
        fread(&e, sizeof(int), 1, input);
        printf("%c", (char) e);
        fprintf(output, "%c", (char) e);
    }

    printf("}");
    if(code_start > 0) fprintf(output, "}\n");

    printf("_CODE_:\n");
    fprintf(output, "_CODE_:\n");

    int state = 0;

    // start interpretation:
    while(1) {
        if(fread(&i, sizeof(int), 1, input) != 1) break;

        switch(state)
        {
            case 0: // expecting op-code:
                switch(i)
                {
                    case NOP:
                        printf("NOP ");
                        fprintf(output, "NOP ");
                        state = 0;
                        break;
                    case INT:
                        printf("INT ");
                        fprintf(output, "INT ");
                        state = 1;
                        break;
                    case MOV:
                        printf("MOV ");
                        fprintf(output, "MOV ");
                        state = 3;
                        break;
                    case CPY:
                        printf("CPY ");
                        fprintf(output, "CPY ");
                        state = 4;
                        break;
                    case ADD:
                        printf("ADD ");
                        fprintf(output, "ADD ");
                        state = 4;
                        break;
                    case INC:
                        printf("INC ");
                        fprintf(output, "INC ");
                        state = 3;
                        break;
                    case DEC:
                        printf("DEC ");
                        fprintf(output, "DEC ");
                        state = 3;
                        break;
                    case SUB:
                        printf("SUB ");
                        fprintf(output, "SUB ");
                        state = 4;
                        break;
                    case CMP:
                        printf("CMP ");
                        fprintf(output, "CMP ");
                        state = 4;
                        break;
                    case JMP:
                        printf("JMP ");
                        fprintf(output, "JMP ");
                        state = 1;
                        break;
                    case JEQ:
                        printf("JEQ ");
                        fprintf(output, "JEQ ");
                        state = 1;
                        break;
                    case JLE:
                        printf("JLE ");
                        fprintf(output, "JLE ");
                        state = 1;
                        break;
                    case JGE:
                        printf("JGE ");
                        fprintf(output, "JGE ");
                        state = 1;
                        break;
                    case PUSH:
                        printf("PUSH ");
                        fprintf(output, "PUSH ");
                        state = 2;
                        break;
                    case POP:
                        printf("POP ");
                        fprintf(output, "POP ");
                        state = 2;
                        break;
                    case GET:
                        printf("GET ");
                        fprintf(output, "GET ");
                        state = 4;
                        break;
                    case SET:
                        printf("SET ");
                        fprintf(output, "SET ");
                        state = 4;
                        break;
                }
                break;

            case 1: // expecting integer
                printf("%i\n", i);
                fprintf(output, "%i\n", i);
                state = 0;
                break;

            case 4: // expecting a register followed by a register
            case 3: // expecting a register followed by an integer
            case 2: // expecting register
                switch(i)
                {
                    case EAX: printf("EAX"); fprintf(output, "EAX"); break;
                    case EBX: printf("EBX"); fprintf(output, "EBX"); break;
                    case ECX: printf("ECX"); fprintf(output, "ECX"); break;
                    case EDX: printf("EDX"); fprintf(output, "EDX"); break;
                    case ESP: printf("ESP"); fprintf(output, "ESP"); break;
                    default:
                        puts("Unknown register error in input file.");
                        return -1;
                        break;
                }

                switch(state)
                {
                    case 2:
                        printf("\n");
                        fprintf(output, "\n");
                        state = 0;
                        break;
                    case 3:
                        printf(" ");
                        fprintf(output, " ");
                        state = 1;
                        break;
                    case 4:
                        printf(" ");
                        fprintf(output, " ");
                        state = 2;
                        break;
                }

                break;
        }
    }

    fclose(input);
    fclose(output);

    return 0;
}

