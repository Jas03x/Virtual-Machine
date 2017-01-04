#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system.h"

#include <SDL.h>
#include <OpenGL/gl3.h>

#define CONSOLE_WIDTH 640
#define CONSOLE_HEIGHT 480

// debug statement:
//#define DEBUG_MODE

#ifdef DEBUG_MODE
    #define dprintf(format, ...) printf((format), __VA_ARGS__);
#else
    #define dprintf(...)
#endif

static unsigned char* RAM;

#define L 0
#define H 1
// NOTE: the high and low registers are _ONLY_ for the lower m16 byte like normal CPUs
typedef union REG32
{
    char m8[2];
    short m16;
    int m32;
    float f32;
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
    REGISTERS[ESB].m32 += size;
    REGISTERS[ESP].m32 += size;

    fclose(file);
    return 0;
}

static inline char read_b() { return RAM[REGISTERS[EIP].m32++]; }
static inline int read_s(){ return RAM[REGISTERS[EIP].m32++] + (RAM[REGISTERS[EIP].m32++] << 8); }
static inline int read_i(){ return RAM[REGISTERS[EIP].m32++] + (RAM[REGISTERS[EIP].m32++] << 8) + (RAM[REGISTERS[EIP].m32++] << 16) + (RAM[REGISTERS[EIP].m32++] << 24); }

/*******************************************************************/
/******************************* gpu *******************************/
/*******************************************************************/

#define GPU_MAX_VBO_SIZE 256

GLuint GPU_DEFAULT_SHADER;
GLint GPU_VERTEX_SOURCE;
GLint GPU_COLOUR_SOURCE;
GLint GPU_MATRIX_SOURCE;
GLuint GPU_VBO, GPU_VAO;
float GPU_RGB[3] = {0};
float GPU_PROJECTION_MATRIX[16] = {
    2.0f / ((float) CONSOLE_WIDTH), 0, 0, 0,
    0, -2.0f / ((float) CONSOLE_HEIGHT), 0, 0,
    0, 0, 1, 0,
    -1, 1, 0, 1
};

void gpu_error_check() {
    int error = 0;
    switch(error) {
        case 0: break;
        default:
            printf("GPU GL Error: %i\n", error);
            break;
    }
}

void gpu_init_shaders() {
    GLuint shaders[2];
    shaders[0] = glCreateShader(GL_VERTEX_SHADER);
    shaders[1] = glCreateShader(GL_FRAGMENT_SHADER);
    static const char* VERTEX_SHADER_SOURCE[] = {
        "#version 330\n",
        "in vec2 vertex;\n",
        "uniform mat4 matrix;\n"
        "void main() {\n",
        "   gl_PointSize = 10.0f;\n"
        "   gl_Position = matrix * vec4(vertex.x, vertex.y, 0, 1);\n",
        "}\n"
    };
    static const char* FRAGMENT_SHADER_SOURCE[] = {
        "#version 330\n",
        "uniform vec3 colour;\n",
        "out vec4 c_out;\n"
        "void main() {\n",
        "   c_out = vec4(colour, 1);\n",
        "}\n"
    };
    glShaderSource(shaders[0], sizeof(VERTEX_SHADER_SOURCE) / sizeof(char*), VERTEX_SHADER_SOURCE, NULL);
    glShaderSource(shaders[1], sizeof(FRAGMENT_SHADER_SOURCE) / sizeof(char*), FRAGMENT_SHADER_SOURCE, NULL);
    glCompileShader(shaders[0]);
    glCompileShader(shaders[1]);
    unsigned int i = 0;
    for(; i < 2; i++) {
        GLint status = 0;
        glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &status);
        if(status == GL_FALSE) {
            int length = 0;
            glGetShaderiv(shaders[i], GL_INFO_LOG_LENGTH, &length);
            char* error = (char*) malloc(length);
            if(!error){ puts("Memory allocation failure."); exit(-1); }
            glGetShaderInfoLog(shaders[i], length, &length, error);
            printf("Error: GPU %s shader error:\n%s\n", i == 0 ? "vertex" : "fragment", error);
            free(error);
            exit(-1);
        }
    }
    GPU_DEFAULT_SHADER = glCreateProgram();
    glAttachShader(GPU_DEFAULT_SHADER, shaders[0]);
    glAttachShader(GPU_DEFAULT_SHADER, shaders[1]);
    glLinkProgram(GPU_DEFAULT_SHADER);

    GLint status = 0;
    glGetProgramiv(GPU_DEFAULT_SHADER, GL_LINK_STATUS, &status);
    if(status == GL_FALSE) {
        int length = 0;
        glGetProgramiv(GPU_DEFAULT_SHADER, GL_INFO_LOG_LENGTH, &length);
        char* error = (char*) malloc(length);
        if(!error){ puts("Memory allocation failure."); exit(-1); }
        glGetShaderInfoLog(GPU_DEFAULT_SHADER, length, &length, error);
        printf("Error: GPU program linking failure:\n%s\n", error);
        free(error);
        glDeleteShader(shaders[0]);
        glDeleteShader(shaders[1]);
        exit(-1);
    }

    glDetachShader(GPU_DEFAULT_SHADER, shaders[0]);
    glDetachShader(GPU_DEFAULT_SHADER, shaders[1]);
    glDeleteShader(shaders[0]);
    glDeleteShader(shaders[1]);
    gpu_error_check();
}

void gpu_init() {
    glGenBuffers(1, &GPU_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, GPU_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * GPU_MAX_VBO_SIZE, NULL, GL_STREAM_DRAW);

    glGenVertexArrays(1, &GPU_VAO);
    glBindVertexArray(GPU_VAO);

    gpu_init_shaders();
    GPU_VERTEX_SOURCE = glGetAttribLocation(GPU_DEFAULT_SHADER,  "vertex");
    GPU_COLOUR_SOURCE = glGetUniformLocation(GPU_DEFAULT_SHADER, "colour");
    GPU_MATRIX_SOURCE = glGetUniformLocation(GPU_DEFAULT_SHADER, "matrix");
    if(GPU_VERTEX_SOURCE == -1 || GPU_COLOUR_SOURCE == -1 || GPU_MATRIX_SOURCE == -1) {
        puts("Error: GPU source(s) not found in default shader.");
        exit(-1);
    }
    glUseProgram(GPU_DEFAULT_SHADER);
    glBindBuffer(GL_ARRAY_BUFFER, GPU_VBO);
    glEnableVertexAttribArray(GPU_VERTEX_SOURCE);
    glVertexAttribPointer(GPU_VERTEX_SOURCE, 2, GL_FLOAT, GL_FALSE, 0, (void*) 0);
    glUniformMatrix4fv(GPU_MATRIX_SOURCE, 1, GL_FALSE, GPU_PROJECTION_MATRIX);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    gpu_error_check();
}

void gpu_draw(void* data, unsigned int count) {
    printf("DRAW %i\n", count);
    printf("Coordinates: %f %f\n", *((float*) data), *(((float*) data) + 1));
    printf("%f %f\n", *(((float*) data) + 2), *(((float*) data) + 3));
    printf("%f %f\n", *(((float*) data) + 4), *(((float*) data) + 5));
    if(count > GPU_MAX_VBO_SIZE) {
        puts("Error: Elements to draw exceeds maximum gpu limit.");
        exit(-1);
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * GPU_MAX_VBO_SIZE, NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * count * 2, data);
    glUniform3fv(GPU_COLOUR_SOURCE, 1, GPU_RGB);
    glDrawArrays(GL_POINTS, 0, count);
    gpu_error_check();
}

void gpu_free() {
    if(glIsBuffer(GPU_VBO) == GL_TRUE) glDeleteBuffers(1, &GPU_VBO);
    if(glIsVertexArray(GPU_VAO) == GL_TRUE) glDeleteVertexArrays(1, &GPU_VAO);
    if(glIsProgram(GPU_DEFAULT_SHADER) == GL_TRUE) glDeleteProgram(GPU_DEFAULT_SHADER);
    gpu_error_check();
}

/*******************************************************************/
/******************************* gpu *******************************/
/*******************************************************************/

int main()
{
    // allocate the heap
    RAM = (unsigned char*) malloc(RAM_SIZE);
    if(!RAM) return -1;
    memset(RAM, 0, RAM_SIZE); // 0 - initalize the memory

    if(readROM() != 0)
    {
        puts("ROM reading exception.");
        return -1;
    }

    // open the file descriptor for the hard-drive:
    FILE* DRIVE = fopen("DRIVE", "rb");
    if(!DRIVE) {
        puts("Error opening virtual machine drive file.");
        return -1;
    }

    // create the screen:
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL initalization error:\n%s\n", SDL_GetError());
        return -1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window* window = SDL_CreateWindow("Virtual Machine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, CONSOLE_WIDTH, CONSOLE_HEIGHT, SDL_WINDOW_SHOWN);
    if(window == NULL) {
        printf("SDL window creation error:\n%s\n", SDL_GetError());
        return -1;
    }
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if(context == NULL) {
        printf("SDL opengl context creation error:\n%s\n", SDL_GetError());
        return -1;
    }

    gpu_init();

    int RUNNING = 1; // runing boolean
    int i; // a temporary use integer
    unsigned char OP, v0, v1; // temporary use characters
    SDL_Event event;

    while(RUNNING)
    {
        // increment the stack pointer for the next bit
        OP = read_b();

        switch(OP)
        {
            case INT:
                v0 = read_b();
                dprintf("INT %i\n", v0);
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

                    case READ_DISK:
                        dprintf("READ: %i to %i\n", REGISTERS[EAX].m32, REGISTERS[EBX].m32);
                        if(REGISTERS[EAX].m32 < 0 || REGISTERS[EBX].m32 < 0 || REGISTERS[EAX].m32 >= REGISTERS[EBX].m32) {
                            printf("VM Crash: Invalid read registers: EAX=[%i] EBX=[%i]\n", REGISTERS[EAX].m32, REGISTERS[EBX].m32);
                        }

                        if(fseek(DRIVE, REGISTERS[EAX].m32, SEEK_SET) != 0) {
                            printf("VM Crash: Invalid READ_DISK start position [%i].\n", REGISTERS[EAX].m32);
                            return -1;
                        }

                        if(REGISTERS[EBX].m32 - REGISTERS[EAX].m32 + REGISTERS[ESP].m32 > RAM_SIZE) {
                            puts("VM Crash: READ_DISK stack overflow.");
                            return -1;
                        }

                        if(fread(&RAM[REGISTERS[ESP].m32], 1, REGISTERS[EBX].m32 - REGISTERS[EAX].m32, DRIVE) != REGISTERS[EBX].m32 - REGISTERS[EAX].m32) {
                            puts("VM Crash: READ_DISK failure.");
                            return -1;
                        }

                        REGISTERS[ESP].m32 += REGISTERS[EBX].m32 - REGISTERS[EAX].m32;
                        break;

                    case POLL: // check if any events were made:
                        while(SDL_PollEvent(&event)) {
                            switch(event.type) {
                                case SDL_QUIT:
                                    RUNNING = 0;
                                    break;
                                default:
                                    break;
                            }
                        }

                    case REDRAW:
                        SDL_GL_SwapWindow(window);
                        break;

                    case SET_COLOR:
                        printf("first number = %i\n", RAM[REGISTERS[ESI].m32]);
                        GPU_RGB[0] = RAM[REGISTERS[ESI].m32] / 255.0f;
                        GPU_RGB[1] = RAM[REGISTERS[ESI].m32+1] / 255.0f;
                        GPU_RGB[2] = RAM[REGISTERS[ESI].m32+2] / 255.0f;
                        printf("New color = %f %f %f\n", GPU_RGB[0], GPU_RGB[1], GPU_RGB[2]);
                        break;

                    case DRAW:
                        gpu_draw(&RAM[REGISTERS[ESI].m32], REGISTERS[EAX].m32);
                        break;

                    default:
                        printf("VM Crash: Bad interrupt [%i]\n", v0);
                        return -1;
                }
                break;

            case MOV:
                v0 = read_b();
                dprintf("MOV %i ", v0);
                switch(v0)
                {
                    case AL: REGISTERS[EAX].m8[L] = read_b(); dprintf("%i\n", REGISTERS[EAX].m8[L]); break;
                    case BL: REGISTERS[EBX].m8[L] = read_b(); dprintf("%i\n", REGISTERS[EBX].m8[L]); break;
                    case CL: REGISTERS[ECX].m8[L] = read_b(); dprintf("%i\n", REGISTERS[ECX].m8[L]); break;
                    case DL: REGISTERS[EDX].m8[L] = read_b(); dprintf("%i\n", REGISTERS[EDX].m8[L]); break;

                    case AH: REGISTERS[EAX].m8[H] = read_b(); dprintf("%i\n", REGISTERS[EAX].m8[H]); break;
                    case BH: REGISTERS[EBX].m8[H] = read_b(); dprintf("%i\n", REGISTERS[EBX].m8[H]); break;
                    case CH: REGISTERS[ECX].m8[H] = read_b(); dprintf("%i\n", REGISTERS[ECX].m8[H]); break;
                    case DH: REGISTERS[EDX].m8[H] = read_b(); dprintf("%i\n", REGISTERS[EDX].m8[H]); break;

                    case AX: REGISTERS[EAX].m16 = read_s(); dprintf("%i\n", REGISTERS[EAX].m16); break;
                    case BX: REGISTERS[EBX].m16 = read_s(); dprintf("%i\n", REGISTERS[EBX].m16); break;
                    case CX: REGISTERS[ECX].m16 = read_s(); dprintf("%i\n", REGISTERS[ECX].m16); break;
                    case DX: REGISTERS[EDX].m16 = read_s(); dprintf("%i\n", REGISTERS[EDX].m16); break;

                    case EAX: REGISTERS[EAX].m32 = read_i(); dprintf("%i\n", REGISTERS[EAX].m32); break;
                    case EBX: REGISTERS[EBX].m32 = read_i(); dprintf("%i\n", REGISTERS[EBX].m32); break;
                    case ECX: REGISTERS[ECX].m32 = read_i(); dprintf("%i\n", REGISTERS[ECX].m32); break;
                    case EDX: REGISTERS[EDX].m32 = read_i(); dprintf("%i\n", REGISTERS[EDX].m32); break;
                    case ESB: REGISTERS[ESB].m32 = read_i(); dprintf("%i\n", REGISTERS[ESB].m32); break;
                    case ESP: REGISTERS[ESP].m32 = read_i(); dprintf("%i\n", REGISTERS[ESP].m32); break;
                    case ESI: REGISTERS[ESI].m32 = read_i(); dprintf("%i\n", REGISTERS[ESI].m32); break;
                    case EDI: REGISTERS[EDI].m32 = read_i(); dprintf("%i\n", REGISTERS[EDI].m32); break;

                    default:
                        printf("VM Crash: Bad move destination register [%i]\n", v0);
                        return -1;
                }
                break;

            case CPY:
                v0 = read_b();
                v1 = read_b();
                dprintf("CPY %c %c\n", v0, v1);
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
                        REGISTERS[(int) v0].m32 = REGISTERS[(int) v1].m32;
                        break;

                    default:
                        printf("VM Crash: Bad copy source register [%i].\n", v0);
                        return -1;
                }
                break;

            case INC:
                v0 = read_b();
                dprintf("INC %i -> ", v0);
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
                        *REG8[v0 - REG8_OFFSET] += read_b(); // 8 bit mode - add 1 byte
                        dprintf("%i\n", *REG8[v0 - REG8_OFFSET]);
                        break;

                    case AX:
                    case BX:
                    case CX:
                    case DX:
                        *REG16[v0 - REG16_OFFSET] += read_s(); // 16 bit mode - add 1 short
                        dprintf("%i\n", *REG16[v0 - REG16_OFFSET]);
                        break;

                    case EAX:
                    case EBX:
                    case ECX:
                    case EDX:
                    case ESP:
                    case ESI:
                    case EDI:
                        REGISTERS[(int) v0].m32 += read_i(); // 32 bit mode - add 1 int
                        dprintf("%i\n", REGISTERS[v0].m32);
                        break;

                    default:
                        printf("VM Crash: Bad increment register [%i]\n", v0);
                        return -1;
                }
                break;
                
            case DEC:
                v0 = read_b();
                dprintf("DEC %c\n", v0);
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
                        REGISTERS[(int) v0].m32 -= read_i();
                        break;

                    default:
                        printf("VM Crash: Bad increment register [%i]\n", v0);
                        return -1;
                }
                break;

            case ADD:
                v0 = read_b();
                v1 = read_b();
                dprintf("ADD %i %i\n", v0, v1);
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
                        REGISTERS[(int) v0].m32 += REGISTERS[(int) v1].m32;
                        break;

                    default:
                        printf("VM Crash: Bad add source register [%i].\n", v0);
                        return -1;
                }
                break;

            case SUB:
                v0 = read_b();
                v1 = read_b();
                dprintf("SUB %i %i\n", v0, v1);
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
                        REGISTERS[(int) v0].m32 -= REGISTERS[(int) v1].m32;
                        break;

                    default:
                        printf("VM Crash: Bad sub source register [%i].\n", v0);
                        return -1;
                }
                break;

            case JMP:
                REGISTERS[EIP].m32 = read_i();
                dprintf("JMP -> %i\n", REGISTERS[EIP].m32);
                break;

            case JEQ:
                i = read_i();
                dprintf("JEQ %i\n", i);
                if(REGISTERS[FLG].m32 & EQ_FLAG) REGISTERS[EIP].m32 = i;
                break;

            case JGE:
                i = read_i();
                dprintf("JGE %i\n", i);
                if(REGISTERS[FLG].m32 & GT_FLAG) REGISTERS[EIP].m32 = i;
                break;

            case JLE:
                i = read_i();
                dprintf("JLE %i\n", i);
                if(REGISTERS[FLG].m32 & LS_FLAG) REGISTERS[EIP].m32 = i;
                break;

            case CMP:
                v0 = read_b();
                v1 = read_b();
                dprintf("CMP %i %i -> ", v0, v1);
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
                        dprintf("%i vs %i\n", *REG8[v0 - REG8_OFFSET], *REG8[v1 - REG8_OFFSET]);
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
                        dprintf("%i vs %i\n", *REG16[v0 - REG16_OFFSET], *REG16[v1 - REG16_OFFSET]);
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
                        dprintf("%i vs %i\n", REGISTERS[v0].m32, REGISTERS[v1].m32);
                        v0 = REGISTERS[(int) v0].m32 - REGISTERS[(int) v1].m32;
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
                dprintf("PUSH %i\n", v0);
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
                    case EIP:
                    case FLG:
                        memcpy(&RAM[REGISTERS[ESP].m32], &REGISTERS[(int) v0].m32, sizeof(int));
                        REGISTERS[ESP].m32 += 4;
                        break;

                    default:
                        printf("VM Crash: Invalid push register [%i].\n", v0);
                        return -1;
                }
                break;

            case POP:
                v0 = read_b();
                dprintf("POP %i\n", v0);
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
                        *REG8[v0 - REG8_OFFSET] = RAM[REGISTERS[ESP].m32 - 1];
                        REGISTERS[ESP].m32 --;
                        break;

                    case AX:
                    case BX:
                    case CX:
                    case DX:
                        memcpy(&REG16[v0 - REG16_OFFSET], &RAM[REGISTERS[ESP].m32 - 2], sizeof(short));
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
                        memcpy(&REGISTERS[(int) v0].m32, &RAM[REGISTERS[ESP].m32 - 4], sizeof(int));
                        REGISTERS[ESP].m32 -= 4;
                        break;

                    default:
                        printf("VM Crash: Invalid pop register [%i].\n", v0);
                        return -1;
                }
                break;

            case WRITE:
                v0 = read_b();
                dprintf("WRITE %i\n", v0);
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
                        memcpy(&RAM[REGISTERS[EDI].m32], &REGISTERS[(int) v0].m32, sizeof(int));
                        break;

                    default:
                        printf("VM Crash: Invalid set operation register [%i].\n", v0);
                        return -1;
                }
                break;

            case FETCH:
                v0 = read_b();
                dprintf("FETCH %i -> ", v0);
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
                        dprintf("%i\n", *REG8[v0 - REG8_OFFSET]);
                        break;

                    case AX:
                    case BX:
                    case CX:
                    case DX:
                        memcpy(REG16[v0 - REG16_OFFSET], &RAM[REGISTERS[ESI].m32], sizeof(short));
                        dprintf("%i\n", *REG16[v0 - REG16_OFFSET]);
                        break;

                    case EAX:
                    case EBX:
                    case ECX:
                    case EDX:
                    case ESB:
                    case ESP:
                    case ESI:
                    case EDI:
                        memcpy(&REGISTERS[(int) v0].m32, &RAM[REGISTERS[ESI].m32], sizeof(int));
                        dprintf("%i\n", REGISTERS[v0].m32);
                        break;

                    default:
                        printf("VM Crash: Invalid get operation register [%i].\n", v0);
                        return -1;
                }
                break;

            case CALL:
                i = read_i(); // the location to jump to
                // push the instruction register to the stack
                memcpy(&RAM[REGISTERS[ESP].m32], &REGISTERS[EIP].m32, sizeof(int));
                dprintf("Pushed EIP: %i\n", REGISTERS[EIP].m32);
                REGISTERS[ESP].m32 += 4;
                // jump to the location in memory
                REGISTERS[EIP].m32 = i;
                dprintf("Set EIP: %i\n", i);
                break;

            case RET:
                memcpy(&REGISTERS[EIP].m32, &RAM[REGISTERS[ESP].m32 - 4], sizeof(int));
                REGISTERS[ESP].m32 -= 4;
                dprintf("RET -> %i\n", REGISTERS[EIP].m32);
                break;

            default:
                printf("VM Crash: Invalid op-code [%i].\n", OP);
                return -1;
        }
    }

    fclose(DRIVE);

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    free(RAM);
    gpu_free();

    return 0;
}

