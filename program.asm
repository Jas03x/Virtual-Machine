; hello world program

str_start: {  HELLO WORLD\n} ; RAW data
str_end:

_CODE_:

MOV ESP str_start ; stack pointer set to the string start
MOV ECX str_end ; ECX used to mark the string end

print:
GET EBX ESP ; memory access at point ESP
INT 3; system interupt
INC ESP 1 ; increment the stack pointer to the next character

CMP ESP ECX
JLE print

INT 1 ; interreupt with termination status in EAX
