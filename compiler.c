#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "system.h"

#define MAX_LINE_LENGTH 256

// some basic helper function macros:
#define inRange(x, m, M) ((m) <= (x) && (x) <= (M))
#define isNumber(x) (inRange((x), 48, 57))
#define isLetter(x) (inRange((x), 65, 90) || inRange((x), 97, 122))
#define isChar(x) (isNumber((int) (x)) || isLetter((int) (x)) || (int) x == 95 )

// convert a string into the integer representation
// return -1 if the string does not match any system opcodes
int parse_instruction(char* str)
{
    if(strlen(str) == 3) {
        switch(str[0]) {
            case 'N':
                if(strcmp(str, "NOP") == 0) return NOP; else return -1;
            case 'I':
                if(strcmp(str, "INT") == 0) return INT;
                else if(strcmp(str, "INC") == 0) return INC;
                else return -1;
            case 'M':
                if(strcmp(str, "MOV") == 0) return MOV; else return -1;
            case 'C':
                if(strcmp(str, "CMP") == 0) return CMP;
                else if(strcmp(str, "CPY") == 0) return CPY;
                else return -1;
            case 'A':
                if(strcmp(str, "ADD") == 0) return ADD; else return -1;
            case 'D':
                if(strcmp(str, "DEC") == 0) return DEC; else return -1;
            case 'S':
                if(strcmp(str, "SUB") == 0) return SUB; else return -1;
            case 'J':
                if(strcmp(str, "JMP") == 0) return JMP;
                else if(strcmp(str, "JEQ") == 0) return JEQ;
                else if(strcmp(str, "JGE") == 0) return JGE;
                else if(strcmp(str, "JLE") == 0) return JLE;
                else return -1;
            case 'P':
                if(strcmp(str, "POP") == 0) return POP; else return -1;
            case 'R':
                if(strcmp(str, "RET") == 0) return RET; else return -1;
        }
    }
    else if(strcmp(str, "CALL") == 0) return CALL;
    else if(strcmp(str, "PUSH") == 0) return PUSH;
    else if(strcmp(str, "FETCH") == 0) return FETCH;
    else if(strcmp(str, "WRITE") == 0) return WRITE;

    return -1;
}

// converts the string into the appropriate system register index
// returns -1 if the string does not match a system register
int parse_register(char* str)
{
    if(strlen(str) < 2) return -1;
    switch(str[0])
    {
        case 'A':
            if(str[1] == 'L') return AL;
            else if(str[1] == 'H') return AH;
            else if(str[1] == 'X') return AX;
            return -1;
        case 'B':
            if(str[1] == 'L') return BL;
            else if(str[1] == 'H') return BH;
            else if(str[1] == 'X') return BX;
            return -1;
        case 'C':
            if(str[1] == 'L') return CL;
            else if(str[1] == 'H') return CH;
            else if(str[1] == 'X') return CX;
            return -1;
        case 'D':
            if(str[1] == 'L') return DL;
            else if(str[1] == 'H') return DH;
            else if(str[1] == 'X') return DX;
            return -1;
        case 'E':
            if(strcmp(str, "EAX") == 0) return EAX;
            else if(strcmp(str, "EBX") == 0) return EBX;
            else if(strcmp(str, "ECX") == 0) return ECX;
            else if(strcmp(str, "EDX") == 0) return EDX;
            else if(strcmp(str, "ESB") == 0) return ESB;
            else if(strcmp(str, "ESP") == 0) return ESP;
            else if(strcmp(str, "ESI") == 0) return ESI;
            else if(strcmp(str, "EDI") == 0) return EDI;
            else if(strcmp(str, "EIP") == 0) return EIP;
            else if(strcmp(str, "FLG") == 0) return FLG;
            return -1;
    }

    return -1;
}

// parse the string as a value
// throws an exception if the string contains non-integral characters
// throws an exception if the integer exceeds the value bounds in limis.h
// possible value modes:
#define PARSE_BYTE  0
#define PARSE_SHORT 1
#define PARSE_INT   2
#define PARSE_FLOAT 3
// sign = value signed/unsigned
#define SIGNED   0
#define UNSIGNED 1
// helper function to check if the integer is within the range
// NOTE: BY DEFAULT WE HAVE SET ALL THE VALUES TO BE UNSIGNED! MEANING NO NEGATIVE VALUES ARE ALLOWED RIGHT NOW!
int sign_check(int i, int mode, int sign){
    switch(sign) {
        case SIGNED:
            switch(mode) {
                case PARSE_BYTE: if(i >= SCHAR_MIN && i <= SCHAR_MAX) return 1; else return 0;
                case PARSE_SHORT: if(i >= SHRT_MIN && i <= SHRT_MAX) return 1; else return 0;
                case PARSE_INT: return 1;
                case PARSE_FLOAT: return 1;
                default: return 0;
            }
        case UNSIGNED:
            switch(mode) {
                case PARSE_BYTE: if(i >= 0 && i <= UCHAR_MAX) return 1; else return 0;
                case PARSE_SHORT: if(i >= 0 && i <= USHRT_MAX) return 1; else return 0;
                case PARSE_INT: if(i >= 0) return 1; else return 0;
                default: return 0;
            }
        default:
            return 0;
    }
    return 0;
}
// the actual parse function:
int parse_value(char* str, int line, int mode, int sign) {
    if(str[0] == '\'') { // if it begins with a quote, then we parse as a byte
        if(str[1] == '\\') {
            if(str[3] != '\'') { printf("Unexpecting value [%s] on line [%i].\n", str, line); exit(-1); }
            return str[2];
        }
        else {
            if(str[2] != '\'') {
                return str[1];
            }
        }
    } else {
        int i = 0;

        // check if its a valid value
        int len = strlen(str);
        for(; i < len; i++) {
            if(!isNumber(str[i])) {
                if(mode == PARSE_FLOAT) {
                    if(str[i] == '.') continue;
                    else {
                        printf("Unexpected value [%s] on line [%i].\n", str, line);
                        exit(-1);
                    }
                }
            }
        }
        if(mode == PARSE_INT) return atoi(str);
        else if(mode == PARSE_FLOAT) return atof(str);
        else {
            i = atoi(str);
            if(sign_check(i, mode, sign) != 1) {
                printf("Error: Expected value [%i] exceeds ", i);
                switch(mode) {
                    case PARSE_BYTE: printf("%sbyte.\n", sign == SIGNED ? "signed" : "unsigned"); break;
                    case PARSE_SHORT: printf("%s short.\n", sign == SIGNED ? "signed" : "unsigned"); break;
                    case PARSE_INT: printf("%s int.\n", sign == SIGNED ? "signed" : "unsigned"); break;
                    case PARSE_FLOAT: printf("%s float.\n", sign == SIGNED ? "signed" : "unsigned"); break;
                    default: printf("[unknown data type error].\n"); break;
                }
                exit(-1);
            }
            return i;
        }
    }
    return 0;
}

// label structure for defining offsets for assembly labels
typedef struct Label {
    char* name; // the name of the assembly label
    int offset; // the offset in bytes
}Label;

// the labels array initally has enough space for 16 labels
#define LABELS_ARRAY_INITAL_SIZE 16
Label* labels = NULL;
unsigned int labels_count = 0;
unsigned int label_index = 0;

// the symbols array is initally enough to store 2048 characters
#define SYMBOLS_ARRAY_INITIAL_SIZE 2048
char* symbols; // the byte buffer for the source code to write
unsigned int symbols_count = 0;
unsigned int symbol_index = 0;

// the post processor structure the references to labels which need to be overwritten with byte offsets
typedef struct PostProcessor
{
    char* name; // the name of the reference
    int offset; // the offset of the symbols to overwrite in bytes
    int line;   // the line number on which this reference appears
    int size;   // the size of the symbol in bytes to overwrite
}PostProcessor;

// the post processors array initally has enough room  for 64 post processor references
#define POST_PROCESSOR_ARRAY_INITIAL_SIZE 64
PostProcessor* post_processors;
unsigned int  post_processors_count = 0;
unsigned int  post_processors_index = 0;

// allocate all the compilation arrays
void allocArrays() {
    // initalize any data structures we need
    labels = (Label*) malloc(sizeof(Label) * LABELS_ARRAY_INITAL_SIZE);
    symbols = (char*) malloc(SYMBOLS_ARRAY_INITIAL_SIZE);
    post_processors = (PostProcessor*) malloc(sizeof(PostProcessor) * POST_PROCESSOR_ARRAY_INITIAL_SIZE);

    if(!labels || !symbols || !post_processors) {
        puts("Memory allocation failure error.");
        exit(-1);
    }

    labels_count = LABELS_ARRAY_INITAL_SIZE;
    symbols_count = SYMBOLS_ARRAY_INITIAL_SIZE;
    post_processors_count = POST_PROCESSOR_ARRAY_INITIAL_SIZE;
}

// dealloc all the compilation arrays we used
void freeArrays() {
    int i = 0;
    for(; i < label_index; i++)
        free(labels[i].name);

    for(i = 0; i < post_processors_index; i++)
        free(post_processors[i].name);

    if(labels) free(labels);
    if(symbols) free(symbols);
    if(post_processors) free(post_processors);
}

// add a label to the labels array
void addLabel(char* str, int offset) {
    if(label_index >= labels_count) {
        labels = realloc(labels, sizeof(Label) * labels_count * 2);
        if(!labels) {
            puts("Memory expansion failure error.");
            exit(-1);
        }
        labels_count *= 2;
    }

    int len = strlen(str);
    labels[label_index].name = (char*) malloc(len);
    if(!labels[label_index].name) {
        puts("Memory allocation failure.");
        exit(-1);
    }
    memcpy(labels[label_index].name, str, len);
    labels[label_index].name[len] = '\0';
    labels[label_index].offset = offset;
    label_index ++;
}

// add the code to the symbols array
void addSymbol(void* ptr, int bytes) {
    while(symbol_index + bytes >= symbols_count) {
        printf("%i + %i = %i >= %i\n", symbol_index, bytes, symbol_index + bytes, symbols_count);
        symbols = (char*) realloc(symbols, symbols_count * 2);
        if(!symbols) {
            puts("Memory expansion failure error.");
            exit(-1);
        }
        symbols_count *= 2;
    }

    memcpy(symbols + symbol_index, ptr, bytes);
    symbol_index += bytes;
}

// add a new post processor to the post processors array
void addPostProcessor(char* name, int offset, int line, int size) {
    if(post_processors_index >= post_processors_count) {
        post_processors = (PostProcessor*) realloc(post_processors, sizeof(PostProcessor) * post_processors_count * 2);
        if(!post_processors) {
            puts("Memory expansion failure error.");
            exit(-1);
        }
        post_processors_count *= 2;
    }

    int len = strlen(name);
    post_processors[post_processors_index].name = (char*) malloc(len + 1);
    memcpy(post_processors[post_processors_index].name, name, len);
    post_processors[post_processors_index].offset = offset;
    post_processors[post_processors_index].line = line;
    post_processors[post_processors_index].size = size;
    post_processors_index ++;
}

// find the label with the given name in the label's array
Label* findLabel(const char* str) {
    int o = 0;
    for(o = 0; o < label_index; o++) {
        if(strcmp(labels[o].name, str) == 0) {
            return &labels[o];
        }
    }

    return NULL;
}

int offset = 0; // the current offset in bytes

void read(const char* path);
void link(const char* path);

int main(int argc, char* argv[])
{
    if(argc != 3) {
        puts("Error: expected 3 arguments.");
        return -1;
    }

    allocArrays();
    read(argv[1]);

    puts("Writing output");
    // write the output
    FILE* output = fopen(argv[2], "wb");
    if(!output) {
        printf("Could not open output file [%s] for writing.\n", argv[2]);
        return -1;
    }
    Label* code_start = findLabel("_CODE_");
    unsigned int start = 0;
    if(code_start == NULL) start = 0; else start = code_start->offset;
    printf("Code start: %i\n", start);
    fwrite(&start, sizeof(unsigned int), 1, output);

    // write the code symbols
    printf("Writing [%i] symbols.\n", symbol_index);
    fwrite(symbols, sizeof(char), symbol_index, output);
    fclose(output);

    freeArrays();

    puts("Successful termination");
    return 0;
}


void read(const char* path)
{
    FILE* input = fopen(path, "r");
    if(!input) {
        printf("Could not open input file [%s] for reading.\n", path);
        exit(-1);
    }

    // temporary use variables
    char reg = 0; // the register
    char opcode = 0; // the current opcode
    unsigned int line = 0; // the current line number
    unsigned int symbol = 0; // the current type of symbol being read (for the FSM)

    char buffer[MAX_LINE_LENGTH]; // the buffer for the currently read string
    unsigned int buffer_index = 0; // the index of the next character to be inserted

    // temporary data types used for writing
    unsigned char t_byte = 0;
    unsigned short t_short = 0;
    unsigned int t_int = 0;
    float t_float = 0;

    while(1)
    {
        char c = fgetc(input);
        if(feof(input) != 0) break;

        switch(c)
        {
            case '"':
                // read the string
                printf("\"");
                while(1) {
                    c = fgetc(input);
                    if(c == '"') break;
                    else if(c == '\\') { // parsing special characters
                        c = fgetc(input);
                        switch(c) {
                            case 'n':
                                c = '\n';
                                break;
                            default:
                                printf("Unknown escape character on line %i: %c.\n", line, c);
                                exit(-1);
                        }
                    }
                    offset++;
                    printf("%c", c);
                    if(feof(input) != 0) {
                        printf("Unexpected end of file on line %i: string not terminated.\n", line);
                        exit(-1);
                    }
                    addSymbol(&c, 1);
                }
                printf("\"\n");
                buffer_index = 0;
                break;

            case '>':
                buffer[buffer_index] = '\0';
                if(buffer[0] != '<') {
                    printf("Bad array type expression on line %i.\n", line);
                    exit(-1);
                }

                // get the type of the array:
                if(strcmp(buffer + 1, "byte") == 0){ printf("bytes: "); symbol = 2; }
                else if(strcmp(buffer + 1, "short") == 0){ printf("shorts: "); symbol = 3; }
                else if(strcmp(buffer + 1, "int") == 0){ printf("integers: "); symbol = 4; }
                else if(strcmp(buffer + 1, "float") == 0){ printf("floats: "); symbol = 5; }
                else {
                    printf("Invalid array type [%s] on line %i.\n", buffer, line);
                    exit(-1);
                }

                if(fgetc(input) != '[') {
                    printf("Error expected array beginning on line %i.\n", line);
                    exit(-1);
                }
                buffer_index = 0;
                break;

            case ':':
                buffer[buffer_index] = '\0'; // if it is a label, terminate at the color
                printf("New label: %s\n", buffer);
                addLabel(buffer, offset);
                buffer_index = 0;
                break;

            case ';':
                buffer[buffer_index] = '\0';
                while(fgetc(input) != '\n' && feof(input) == 0) {} // skip the comment line
            case '\n':
                line++;
            case ' ':
            case '\t':
                if(buffer_index == 0)  { line++; break; } // empty string test
                buffer[buffer_index++] = '\0'; // null terminate the string

                /*if(buffer_index >= 2) {
                    if(buffer[buffer_index - 2] == ':') { // check if the last character is for labels (-2 because the null terminator also counts)
                        buffer[buffer_index - 2] = '\0'; // if it is a label, terminate at the color
                        printf("New label: %s\n", buffer);
                        addLabel(buffer, offset);
                        buffer_index = 0;
                        break;
                    }
                }*/

                // check if this is an include statement
                if(buffer[0] == '#') {
                    if(strcmp(buffer + 1, "include") == 0) {
                        symbol = 14;
                    } else if(strcmp(buffer + 1, "link")) {
                        symbol = 15;
                    } else {
                        printf("Unknown directive [%s] on line [%i].\n", buffer, line);
                        exit(-1);
                    }
                    buffer_index = 0;
                    break;
                }

                switch(symbol)
                {
                    case 0: // opcode mode
                        opcode = parse_instruction(buffer);
                        if(opcode == -1) {
                            printf("Invalid instruction on line %i: [%s]\n", line, buffer);
                            exit(-1);
                        }

                        switch(opcode) {
                            case NOP:   opcode = NOP;   printf("%i: NOP\n",  offset); symbol = 0;  break;
                            case INT:   opcode = INT;   printf("%i: INT ",   offset); symbol = 6;  break;
                            case MOV:   opcode = MOV;   printf("%i: MOV ",   offset); symbol = 1;  break;
                            case CPY:   opcode = CPY;   printf("%i: CPY ",   offset); symbol = 9;  break;
                            case ADD:   opcode = ADD;   printf("%i: ADD ",   offset); symbol = 9;  break;
                            case SUB:   opcode = SUB;   printf("%i: SUB ",   offset); symbol = 9;  break;
                            case INC:   opcode = INC;   printf("%i: INC ",   offset); symbol = 1;  break;
                            case DEC:   opcode = DEC;   printf("%i: DEC ",   offset); symbol = 1;  break;
                            case CMP:   opcode = CMP;   printf("%i: CMP ",   offset); symbol = 9;  break;
                            case JMP:   opcode = JMP;   printf("%i: JMP ",   offset); symbol = 8;  break;
                            case JEQ:   opcode = JEQ;   printf("%i: JEQ ",   offset); symbol = 8;  break;
                            case JLE:   opcode = JLE;   printf("%i: JLE ",   offset); symbol = 8;  break;
                            case JGE:   opcode = JGE;   printf("%i: JGE ",   offset); symbol = 8;  break;
                            case PUSH:  opcode = PUSH;  printf("%i: PUSH ",  offset); symbol = 10; break;
                            case POP:   opcode = POP;   printf("%i: POP ",   offset); symbol = 10; break;
                            case FETCH: opcode = FETCH; printf("%i: FETCH ", offset); symbol = 10; break;
                            case WRITE: opcode = WRITE; printf("%i: WRITE ", offset); symbol = 10; break;
                            case RET:   opcode = RET;   printf("%i: RET\n",  offset); symbol = 0;  break;
                            case CALL:  opcode = CALL;  printf("%i: CALL ",  offset); symbol = 8;  break;
                            default:
                                printf("Unknown opcode [%s] on line %i.\n", buffer, line);
                                exit(-1);
                        }

                        addSymbol(&opcode, 1);
                        buffer_index = 0;
                        offset ++;
                        break;

                    case 14: // expecting the path to the source file to include
                        read(buffer);
                        break;
                    case 15: // expecting the path to the source file to link to
                        link(buffer);
                        break;

                    case 1:  // expecing a register followed by a value
                    case 9:  // expecting 2 registers
                    case 10: // expecting a single register
                        reg = parse_register(buffer);
                        if(reg == -1) {
                            printf("Invalid register on line %i: [%s]\n", line, buffer);
                            exit(-1);
                        }
                        printf("r:%i ", reg);
                        addSymbol(&reg, 1);
                        buffer_index = 0;
                        offset ++; // register indices are byte-sized

                        if(symbol == 9) { // if we are expecting 2 registers, then we pass control to the state which checks for specific register sizes
                            switch(reg) {
                                case AL:
                                case AH:
                                case BL:
                                case BH:
                                case CL:
                                case CH:
                                case DL:
                                case DH:
                                    symbol = 11;
                                    break;

                                case AX:
                                case BX:
                                case CX:
                                case DX:
                                    symbol = 12;
                                    break;

                                case EAX:
                                case EBX:
                                case ECX:
                                case EDX:
                                case ESB:
                                case ESP:
                                case ESI:
                                case EDI:
                                case EIP:
                                case FLG:
                                    symbol = 13;
                                    break;
                            }
                        }
                        else if(symbol == 1) { // if we are expecting the register followed by an integer, pass control to the state which reads the correct value sizes
                            switch(reg) {
                                case AL:
                                case AH:
                                case BL:
                                case BH:
                                case CL:
                                case CH:
                                case DL:
                                case DH:
                                    symbol = 6;
                                    break;

                                case AX:
                                case BX:
                                case CX:
                                case DX:
                                    symbol = 7;
                                    break;

                                case EAX:
                                case EBX:
                                case ECX:
                                case EDX:
                                case ESB:
                                case ESP:
                                case ESI:
                                case EDI:
                                case EIP:
                                case FLG:
                                    symbol = 8;
                                    break;
                            }
                        }
                        else {
                            symbol = 0;
                            printf("\n");
                        }
                        break;

                    case 11: // expecting an 8 bit register
                    case 12: // expecting a 16 bit register
                    case 13: // expecting a 32 bit register
                        reg = parse_register(buffer);
                        if(reg == -1) {
                            printf("Invalid register on line %i: [%s]\n", line, buffer);
                            exit(-1);
                        }

                        if((symbol==11 && !inRange(reg, AL, DH)) || (symbol==12 && !inRange(reg, AX, DX)) || (symbol==13 && !inRange(reg, EAX, FLG))) {
                            printf("Register size mismatch on line %i.\n", line);
                            exit(-1);
                        }

                        printf("r:%i\n", reg);
                        addSymbol(&reg, 1);
                        symbol = 0;
                        buffer_index = 0;
                        offset ++;
                        break;

                    case 6: // expecting a byte
                        if(isLetter(buffer[0])) {
                            t_byte = 0;
                            // insert lookup symbol
                            addPostProcessor(buffer, offset, line, 1);
                            printf("b:[%s]\n", buffer);
                        }
                        else {
                            t_byte = parse_value(buffer, line, PARSE_BYTE, UNSIGNED);
                            printf("b:%i\n", t_byte);
                        }

                        addSymbol(&t_byte, 1);
                        offset ++;
                        buffer_index = 0;
                        symbol = 0;
                        break;

                    case 7: // expecting a short
                        if(isLetter(buffer[0])) {
                            t_short = 0;
                            // insert lookup symbol
                            addPostProcessor(buffer, offset, line, 2);
                            printf("s:[%s]\n", buffer);
                        }
                        else {
                            t_short = parse_value(buffer, line, PARSE_SHORT, UNSIGNED);
                            printf("s:%i\n", t_short);
                        }

                        addSymbol(&t_short, 2);
                        offset += 2;
                        buffer_index = 0;
                        symbol = 0;
                        break;

                    case 8: // expecting an integer
                        if(isLetter(buffer[0])) {
                            t_int = 0;
                            // insert lookup symbol
                            addPostProcessor(buffer, offset, line, 4);
                            printf("i:[%s]\n", buffer);
                        }
                        else {
                            t_int = parse_value(buffer, line, PARSE_INT, UNSIGNED);
                            printf("i:%i\n", t_int);
                        }

                        addSymbol(&t_int, 4);
                        offset += 4;
                        buffer_index = 0;
                        symbol = 0;
                        break;
                }
                break;

            case ',':
            case ']':
                buffer[buffer_index] = '\0';
                switch(symbol) {
                    case 2: // byte array reading mode
                        t_byte = parse_value(buffer, line, PARSE_BYTE, UNSIGNED);
                        printf(", %i", t_byte);
                        addSymbol(&t_byte, 1);
                        offset ++;
                        break;

                    case 3: // short array reading mode
                        t_short = parse_value(buffer, line, PARSE_SHORT, UNSIGNED);
                        printf(", %i", t_short);
                        addSymbol(&t_short, 2);
                        offset += 2;
                        break;

                    case 4: // integer array reading mode
                        t_int = parse_value(buffer, line, PARSE_INT, UNSIGNED);
                        printf(", %i", t_int);
                        addSymbol(&t_int, 4);
                        offset += 4;
                        break;

                    case 5: // float array reading mode
                        t_float = parse_value(buffer, line, PARSE_FLOAT, UNSIGNED);
                        printf(", %ff", t_float);
                        addSymbol(&t_float, 4);
                        offset += 4;
                        break;

                    default:
                        printf("Unexpected symbol [%c] on line %i.\n", c, line);
                        exit(-1);
                }
                buffer_index = 0;
                if(c == ']') symbol = 0;
                break;

            default:
                buffer[buffer_index++] = c;
                break;
        }
    }

    int i = 0;
    for(; i < post_processors_index; i++) {
        printf("Substituting %i of %i: ", i, post_processors_index);
        PostProcessor* p = &post_processors[i];
        printf("%s\n", p->name);
        Label* label = findLabel(p->name);
        printf("Line %i: Swap [%s] at byte [%i] of size [%i].\n", p->line, p->name, p->offset, p->size);

        if(label == NULL) {
            printf("Error: Could not find label [%s].\n", p->name);
            exit(-1);
        }

        switch(p->size) {
            case 1:
                if(!inRange(label->offset, SCHAR_MIN, SCHAR_MAX)) {
                    printf("Error: Label [%s] offset exceeds the value of a char. Substitution failure at line %i.\n", p->name, p->line);
                    exit(-1);
                }
                t_byte = (char) label->offset;
                memcpy(symbols + p->offset, &t_byte, sizeof(char));
                printf("Substituted %i\n", t_byte);
                break;

            case 2:
                if(!inRange(label->offset, SHRT_MIN, SHRT_MAX)) {
                    printf("Error: Label [%s] offset exceeds the value of a short. Substition failure at line %i.\n", p->name, p->line);
                    exit(-1);
                }
                t_short = (short) label->offset;
                memcpy(symbols + p->offset, &t_short, sizeof(short));
                printf("Substituted %i\n", t_short);
                break;

            case 4:
                memcpy(symbols + p->offset, &label->offset, sizeof(int));
                printf("Substituted %i\n", *(int*) (symbols + p->offset));
                break;
        }
    }

    fclose(input);
}

void link(const char* path)
{

}

