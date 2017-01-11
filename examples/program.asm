; hello world program

str_start: "  HELLO WORLD\n" ; RAW data
str_end:

_CODE_:

MOV ESI str_start ; stack source index set to the string start
MOV EDX str_end ; EDX used to mark the string end

print:
FETCH EAX ; memory access at point ESP

INT 3; system interupt
INC ESI 1 ; increment the stack pointer to the next character

CMP ESI EDX
JLE print

INT 1 ; interreupt with termination status in EAX
