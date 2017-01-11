
add:
<short>[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
end:

_CODE_:

MOV ESI add ; ESI points to the array start
MOV EDX end ; EDX points to the array end

loop: ; for each char in the array
    FETCH BX ; fetch the short (16 bit mode)
    ADD EAX EBX ; add the char as a 32 bit integer onto EAX
    INC ESI 2 ; increment by 2 bytes since shorts are 2 bytes
    ; loop again if there are more shorts:
    CMP ESI EDX
    JLE loop

INT 2 ; interrupt 2 - print int

MOV EAX '\n'
INT 3 ; interrupt 3 - print char

INT 1 ; interrupt 1 - exit

