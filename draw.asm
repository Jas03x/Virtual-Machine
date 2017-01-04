
vertices:
<float>[320, 240, 320, 239, 320, 241]

color:<byte>[255, 0, 0]

_CODE_:

; set the render color
MOV ESI color
INT 8

; render the pixels onto the screen
MOV ESI vertices
MOV EAX 3 ; 3 vertices/elements to draw
INT 5 ; draw

; while the window isn't closed
loop:
    INT 6 ; poll the window's events
    ; INT 7 ; redraw the window
    JMP loop

