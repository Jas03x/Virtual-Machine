
; read the first 6 bytes of the hard-drive
; it should say "VM OS\n"

MOV EAX 0
MOV EBX 6
INT 4 ; read hard-drive interrupt

; print the data to the screen
CPY ESI ESB ; copy the stack base pointer
; printing 5 bytes, offset EBX by 6:
CPY EBX ESB
INC EBX 6

loop:
    FETCH EAX
    INT 3
    INC ESI 1
    CMP ESI EBX
    JLE loop

INT 1

