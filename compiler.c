#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system.h"

#define MEMORY_ERROR() {puts("Memory error"); return -1;}

#define MAX_LINE_LENGTH 256

#define inRange(x, m, M) ((m) <= (x) && (x) <= (M))
#define isNumber(x) (inRange((x), 48, 57))
#define isLetter(x) (inRange((x), 65, 90) || inRange((x), 97, 122))
#define isChar(x) (isNumber((int) (x)) || isLetter((int) (x)) || (int) x == 95 )

typedef struct REF
{
    char* name;
    int offset;
}REF;

static REF* REFERENCES = NULL;
static unsigned int REFERENCE_COUNT = 0;
static unsigned int REFERENCES_SIZE = 16;

int findReference(const char* name)
{
    int i = 0;
    for(; i < REFERENCE_COUNT; i++) {
        if(strcmp(name, REFERENCES[i].name) == 0) {
            return i;
        }
    }

    return -1;
}

int addReference(char* name, int offset)
{
    if(REFERENCE_COUNT >= REFERENCES_SIZE) {
        REFERENCES = (REF*) realloc(REFERENCES, sizeof(REF) * REFERENCES_SIZE * 2);
        if(!REFERENCES) MEMORY_ERROR();

        REFERENCES_SIZE *= 2;
    }

    REF ref;
    unsigned long len = strlen(name);

    ref.name = (char*) malloc(len + 1);
    if(!ref.name) MEMORY_ERROR();
    strcpy(ref.name, name);
    ref.offset = offset;

    REFERENCES[REFERENCE_COUNT] = ref;
    REFERENCE_COUNT ++;
    return 0;
}

// fast conversion method
int toRegister(char* buffer)
{
    if(strlen(buffer) != 3) return -1;
    if(buffer[0] != 'E') return -1;
    switch(buffer[1])
    {
        case 'A':
            if(buffer[2] == 'X') return EAX;
            else return -1;
            break;
        case 'B':
            if(buffer[2] == 'X') return EBX;
            else return -1;
            break;
        case 'C':
            if(buffer[2] == 'X') return ECX;
            else return -1;
            break;
        case 'D':
            if(buffer[2] == 'X') return EDX;
            else return -1;
            break;
        case 'S':
            if(buffer[2] == 'P') return ESP;
            else return -1;
            break;
        default:
            return -1;
            break;
    }

    return -1;
}

int main(int argc, char* argv[])
{
    if(argc != 3) {
        puts("Error: expected 3 arguments.");
        return -1;
    }

    FILE* input = fopen(argv[1], "r");
    if(!input) {
        printf("Could not open input file [%s] for reading.\n", argv[1]);
        return -1;
    }

    FILE* output = fopen(argv[2], "w");
    if(!input) {
        printf("Could not open output file [%s] for writing.\n", argv[1]);
        return -1;
    }

    REFERENCES = (REF*) malloc(sizeof(REF) * REFERENCES_SIZE);
    if(!REFERENCES) MEMORY_ERROR();

    char buffer[MAX_LINE_LENGTH];
    unsigned int index = 0;
    unsigned int state = 0;
    int offset = 0;

    while(1)
    {
        char c = fgetc(input);
        if(feof(input)) break;

        switch(state)
        {
            case 0:
                switch(c)
                {
                    case '\n': // multi possiblibilty case 
                    case '\t':
                    case ' ':
                        if(index > 0) {
                            offset ++;
                            index = 0;
                        }
                        break;
                    case '{':
                        state = 1;
                        break;
                    case ';':
                        state = 2;
                        break;
                    case ':':
                        buffer[index] = '\0'; // finalize the string for the reference name
                        addReference(buffer, offset);
                        index = 0; // reset the index
                        break;
                    default:
                        buffer[index++] = c;
                        break;
                }
            break;

            case 1: // data block
                if(c == '}') state = 0; // reset state, the data block is over
                else {
                    offset ++;
                    if(c == '\\') fgetc(input); // skip the special character
                }
                break;

            case 2:
                if(c == '\n') state = 0;
                break;
        }
    }

    // check final state:
    if(state != 0 && state != 2) {
        if(state == 1) {
            puts("Error: Unterminated data block failure.");
            return -1;
        }
    }

    rewind(input);
    index = 0; // index for the buffer
    offset = 0; // the byte offset
    state = 0; // the parse state
    unsigned int symbol = 0; // the symbol state
    unsigned int line = 1; // the line number
    int i; // a temporary variable
    int len = 0;

    // start by writing the code offset:
    i = findReference("_CODE_");
    if(i == -1) i = 0;
    else i = REFERENCES[i].offset;
    const int code_start = i + 1; // +1 since the first byte states where the program begins

    printf("%i\n", code_start);
    fwrite(&code_start, sizeof(int), 1, output);

    while(1)
    {
        char c = fgetc(input);
        if(feof(input)) break;

        switch(state)
        {
            case 0: // any text
                switch(c)
                {
                    case '\n':
                        line++;
                    case '\t':
                    case ';': // in case the code ends with a comment
                    case ' ':
                        if(index > 0) {
                            buffer[index] = '\0'; // finalize string for processing

                            // we have finished reading a statement, figure out if the input is correct
                            switch(symbol)
                            {
                                case 0: // expecting an opcode
                                    if(strcmp(buffer, "NOP") == 0){       i = NOP;  printf("NOP ");  fwrite(&i, sizeof(int), 1, output); symbol = 0; }
                                    else if(strcmp(buffer, "INT") == 0){  i = INT;  printf("INT ");  fwrite(&i, sizeof(int), 1, output); symbol = 1; }
                                    else if(strcmp(buffer, "MOV") == 0){  i = MOV;  printf("MOV ");  fwrite(&i, sizeof(int), 1, output); symbol = 3; }
                                    else if(strcmp(buffer, "CPY") == 0){  i = CPY;  printf("CPY ");  fwrite(&i, sizeof(int), 1, output); symbol = 4; }
                                    else if(strcmp(buffer, "ADD") == 0){  i = ADD;  printf("ADD ");  fwrite(&i, sizeof(int), 1, output); symbol = 4; }
                                    else if(strcmp(buffer, "INC") == 0){  i = INC;  printf("INC ");  fwrite(&i, sizeof(int), 1, output); symbol = 3; }
                                    else if(strcmp(buffer, "DEC") == 0){  i = DEC;  printf("DEC ");  fwrite(&i, sizeof(int), 1, output); symbol = 3; }
                                    else if(strcmp(buffer, "SUB") == 0){  i = SUB;  printf("SUB ");  fwrite(&i, sizeof(int), 1, output); symbol = 4; }
                                    else if(strcmp(buffer, "CMP") == 0){  i = CMP;  printf("CMP ");  fwrite(&i, sizeof(int), 1, output); symbol = 4; }
                                    else if(strcmp(buffer, "JMP") == 0){  i = JMP;  printf("JMP ");  fwrite(&i, sizeof(int), 1, output); symbol = 1; }
                                    else if(strcmp(buffer, "JEQ") == 0){  i = JEQ;  printf("JEQ ");  fwrite(&i, sizeof(int), 1, output); symbol = 1; }
                                    else if(strcmp(buffer, "JLE") == 0){  i = JLE;  printf("JLE ");  fwrite(&i, sizeof(int), 1, output); symbol = 1; }
                                    else if(strcmp(buffer, "JGE") == 0){  i = JGE;  printf("JGE ");  fwrite(&i, sizeof(int), 1, output); symbol = 1; }
                                    else if(strcmp(buffer, "PUSH") == 0){ i = PUSH; printf("PUSH "); fwrite(&i, sizeof(int), 1, output); symbol = 2; }
                                    else if(strcmp(buffer, "POP") == 0){  i = POP;  printf("POP ");  fwrite(&i, sizeof(int), 1, output); symbol = 2; }
                                    else if(strcmp(buffer, "GET") == 0){  i = GET;  printf("GET ");  fwrite(&i, sizeof(int), 1, output); symbol = 4; }
                                    else if(strcmp(buffer, "SET") == 0){  i = SET;  printf("SET ");  fwrite(&i, sizeof(int), 1, output); symbol = 4; }
                                    else {
                                        printf("Error: Expecting op-code, but got [%s] at line [%i].\n", buffer, line);
                                        return -1;
                                    }
                                    break;

                                case 1: // expecting an integer
                                    if(isLetter(buffer[0])) {
                                        i = findReference(buffer);
                                        if(i == -1) {
                                            printf("Invalid integer or label value [%s] on line %i.\n", buffer, line);
                                            return -1;
                                        }
                                        i = REFERENCES[i].offset + 1;
                                    } else {
                                        len = strlen(buffer);
                                        for(i = 0; i < len; i++) {
                                            if(!isNumber(buffer[i])) {
                                                printf("Invalid integer or label value [%s] on line %i.\n", buffer, line);
                                                return -1;
                                            }
                                        }
                                        i = atoi(buffer);
                                    }

                                    printf(" %i", i);
                                    fwrite(&i, sizeof(int), 1, output);
                                    if(symbol == 1)
                                    {
                                        printf("\n");
                                        symbol = 0;
                                    }
                                    else
                                        symbol = 1;
                                    break;

                                case 4: // expecting a register followed by a register
                                case 3: // expecting a register followed by an integer
                                case 2: // expecting a register
                                    i = toRegister(buffer);
                                    if(i == -1) {
                                        printf("Error: Invalid register [%s] on line [%i].\n", buffer, line);
                                        return -1;
                                    }
                                    switch(i) {
                                        case EAX: printf(" EAX"); break;
                                        case EBX: printf(" EBX"); break;
                                        case ECX: printf(" ECX"); break;
                                        case EDX: printf(" EDX"); break;
                                        case ESP: printf(" ESP"); break;
                                    }
                                    fwrite(&i, sizeof(int), 1, output);
                                    if(symbol == 2)
                                    {
                                        printf("\n");
                                        symbol = 0;
                                    }
                                    else if(symbol == 3)
                                        symbol = 1;
                                    else
                                        symbol = 2;
                                    break;
                            }

                            index = 0;
                        }

                        if(c == ';') state = 2;
                        break;
                    case '{':
                        if(offset >= code_start) {
                            printf("Warning: You should not start data block after code begins. line [%i].\n", line);
                        }
                        state = 1;
                        break;
                    case ':': // skip, we have already processed this
                        index = 0;
                        break;
                    default:
                        buffer[index++] = c;
                        break;
                }
                break;

            case 1:
                if(c == '}'){ printf("\n"); state = 0; }
                else if(c == '\n') line++;
                else {
                    i = (int) c;

                    if(c == '\\')
                    {
                        c = fgetc(input);
                        if(c == 'n') {
                            i = (int) '\n';
                            printf("\\n");
                            fwrite (&i, sizeof(int), 1, output);
                        } else {
                            printf("Unknown escape character [%c] on line [%i].\n", c, line);
                            return -1;
                        }
                    }
                    else
                    {
                        printf("%c", c);
                        fwrite(&i, sizeof(int), 1, output);
                    }
                }
                break;
            case 2:
                if(c == '\n'){ line ++; state = 0; }
                break;
        }
    }

    fclose(input);
    fclose(output);

    puts("Successful termination");
    return 0;
}

