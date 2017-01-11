data:
"Hello there!\n"
end:

_CODE_:
JMP main

MyPrintFunction:
    MOV EBX end
    MOV ECX 0
    POP EDX ; copy the return address
    loop:
        POP EAX
        INT 3
        INC ECX 1
        CMP EBX ECX
        JGE loop
        JEQ loop
    PUSH EDX ; set the return address again
    RET

main:
    MOV ESI data
    MOV EDX end

    i_loop:
        FETCH EAX
        PUSH EAX
        INC ESI 1
        CMP ESI EDX
        JLE i_loop

    CALL MyPrintFunction
    MOV EAX ':'
    INT 3
    MOV EAX ')'
    INT 3

    INT 1

