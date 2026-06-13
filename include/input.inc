keyboard_isr:
        push    ax                              ; preserve AX
        push    ds
        xor     ax, ax
        mov     ds, ax                          ; DS = 0
        in      al, 0x60                        ; read a byte (scan code of the last keyboard event) from port 0x60 (keyboard data port)
        cmp     al, 0xE0                        ; is the scan code extended (up/down)?
        jne     .not_extended
        mov     byte [extended_pending], 1      ; remember the next scan code is extended
        jmp     .ack
.not_extended:
        cmp     byte [extended_pending], 0      ; it may be the second byte of an extended sequence
        je      .normal
        mov     byte [extended_pending], 0      ; consume the flag
        cmp     al, 0x48                        ; UP ARROW make code
        je      .set_up
        cmp     al, 0xC8                        ; UP ARROW break code (0x48 | 0x80)
        je      .clear_up
        cmp     al, 0x50
        je      .set_down
        cmp     al, 0xD0
        je      .clear_down
        jmp     .ack
.normal:
        cmp     al, 0x11
        je      .set_w
        cmp     al, 0x91
        je      .clear_w
        cmp     al, 0x1F
        je      .set_s
        cmp     al, 0x9F
        je      .clear_s
        cmp     al, 0x39
        je      .set_space
        cmp     al, 0xB9
        je      .clear_space
        cmp     al, 0x19
        je      .set_p
        cmp     al, 0x99
        je      .clear_p
        jmp     .ack
.set_w:
        mov     byte [key_w], 1
        jmp     .ack
.clear_w:
        mov     byte [key_w], 0
        jmp     .ack
.set_s:
        mov     byte [key_s], 1
        jmp     .ack
.clear_s:
        mov     byte [key_s], 0
        jmp     .ack
.set_up:
        mov     byte [key_up], 1
        jmp     .ack
.clear_up:
        mov     byte [key_up], 0
        jmp     .ack
.set_down:
        mov     byte [key_down], 1
        jmp     .ack
.clear_down:
        mov     byte [key_down], 0
        jmp     .ack
.set_space:
        mov     byte [key_space], 1
        jmp     .ack
.clear_space:
        mov     byte [key_space], 0
        jmp     .ack
.set_p:
        mov     byte [key_p], 1
        jmp     .ack
.clear_p:
        mov     byte [key_p], 0
        jmp     .ack
.ack:
        mov     al, 0x20                        ; EOI (End Of Interrupt)
        ; send AL to I/O port (0x20)
        ; -> when PIC (Programmable Interrupt Controller) receives 0x20 on 0x20 it frees the interrupt on the current slot
        out     0x20, al
        pop     ds
        pop     ax                              ; restore AX
        iret

poll_keyboard:
        cmp     byte [key_p], 0
        je      .not_p
        mov     byte [key_p], 0                 ; consume
        mov     byte [game_state], STATE_PAUSED
        ret
.not_p:
        cmp     byte [key_w], 0                 ; is the buffer != 'w'?
        je      .not_w
        cmp     word [left_paddle_y], 4
        jle     .not_w
        sub     word [left_paddle_y], 4
.not_w:
        cmp     byte [key_s], 0
        je      .not_s
        cmp     word [left_paddle_y], 156       ; 200 - 40 - 4
        jge     .not_s
        add     word [left_paddle_y], 4
.not_s:
        cmp     byte [key_up], 0
        je      .not_up
        cmp     word [right_paddle_y], 4
        jle     .not_up
        sub     word [right_paddle_y], 4
.not_up:
        cmp     byte [key_down], 0
        je      .done
        cmp     word [right_paddle_y], 156
        jge     .done
        add     word [right_paddle_y], 4
.done:
        ret

handle_pause_input:
        cmp     byte [key_p], 0
        je      .done
        mov     byte [key_p], 0                 ; consume
        mov     byte [game_state], STATE_PLAYING
.done:
        ret

handle_gameover_input:
        cmp     byte [key_space], 0
        je      .done
        mov     byte [key_space], 0             ; consume
        mov     byte [game_state], STATE_MENU
.done:
        ret

handle_menu_input:
        cmp     byte [key_up], 0
        je      .no_up
        mov     byte [menu_selection], 0
        mov     byte [key_up], 0                ; consume (avoid auto-repeat)
.no_up:
        cmp     byte [key_down], 0
        je      .no_down
        mov     byte [menu_selection], 1
        mov     byte [key_down], 0
.no_down:
        cmp     byte [key_w], 0
        je      .no_w
        mov     byte [menu_selection], 0
        mov     byte [key_w], 0
.no_w:
        cmp     byte [key_s], 0
        je      .no_s
        mov     byte [menu_selection], 1
        mov     byte [key_s], 0
.no_s:
        cmp     byte [key_space], 0
        je      .done
        mov     byte [key_space], 0
        cmp     byte [menu_selection], 0
        jne     .exit_choice

        ; "START" selected
        call    reset_game
        mov     byte [game_state], STATE_COUNTDOWN
        jmp     .done
.exit_choice:
        ; "EXIT" selected
        cli
        mov     ax, 0x2000
        mov     dx, 0x604                       ; QEMU shutdown
        out     dx, ax
        mov     dx, 0xB004                      ; Bochs shutdown
        out     dx, ax
        mov     dx, 0x4004                      ; VirtualBox shutdown
        out     dx, ax
        hlt                                     ; Real hardware shutdown
        jmp     $
.done:
        ret

; variables
key_w:            db      0
key_s:            db      0
key_up:           db      0
key_down:         db      0
key_space:        db      0
key_p:            db      0
extended_pending: db      0
