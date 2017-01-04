
vertices:
<float>[320, 225, 320, 240, 320, 255]

red:<byte>[255, 0, 0]
blue:<byte>[0, 255, 0]
green:<byte>[0, 0, 255]

draw:
    ; set the color with EBX
    CPY ESI EBX
    INT 8 ; set color
    
    ; render the vertex at ECX
    CPY ESI ECX
    MOV EAX 1
    INT 5 ; draw
    RET

_CODE_:

MOV EBX red
MOV ECX vertices

MOV EDX 0
; loop 3 times
loop:
    CALL draw
    CCMP EDX 3
    INC EBX 3 ; color offset by 3
    INC ECX 8 ; vertex offset by 8
    INC EDX 1
    JEQ break ; break the loop if we are done
    JMP loop ; otherwise loop

break:

; while the window isn't closed
wait:
    INT 6 ; poll the window's events
    ; INT 7 ; redraw the window
    JMP wait

