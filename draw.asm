
; while the window isn't closed
loop:
    INT 6 ; poll the window's events
    JMP loop

