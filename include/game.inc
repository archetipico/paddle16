; Uses [show_vbar]
draw_playing:
        mov     al, 0x0F                        ; load "white" color

        ; left racket
        mov     bp, [left_paddle_y]
        mov     si, 10
        call    compute_position
        mov     dx, 4                           ; rectangle "horizontal cells" to write (width)
        mov     bx, 40                          ; rectangle "lines" to write (height)
        call    draw_rect                       ; I want subroutines and variables to have no underscore at the beginning

        ; right racket
        mov     bp, [right_paddle_y]
        mov     si, 306
        call    compute_position
        mov     dx, 4
        mov     bx, 40
        call    draw_rect

        ; score
        call    draw_score

        cmp     byte [show_vbar_and_ball], 0
        je      .done
        mov     al, 0x0F                        ; reload color

        ; ball
        mov     bp, [ball_y]
        mov     si, [ball_x]
        call    compute_position
        mov     dx, 4
        mov     bx, 4
        call    draw_rect

        ; vertical bar
        call    draw_vertical_bar
.done:
        ret

draw_gameover:
        mov     di, 60 * 320 + 124
        cmp     byte [winner], 0
        jne     .right_won
        mov     si, gameover_left
        jmp     .print_winner
.right_won:
        mov     si, gameover_right
.print_winner:
        call    draw_string
        mov     di, 110 * 320 + 126
        mov     si, press_space
        call    draw_string
        call    draw_score

        ret

draw_menu:
        ; title at row 5, col 18
        mov     di, 40 * 320 + 136
        mov     si, menu_title
        call    draw_string

        ; "START" selector
        mov     di, 90 * 320 + 140
        cmp     byte [menu_selection], 0
        je      .mark_start_marker
        mov     al, ' '
        jmp     .draw_start_marker
.mark_start_marker:
        mov     al, '>'
.draw_start_marker:
        call    draw_char
        mov     si, menu_start
        call    draw_string

        ; "EXIT" selector
        mov     di, 110 * 320 + 140
        cmp     byte [menu_selection], 1
        je      .mark_exit_marker
        mov     al, ' '
        jmp     .draw_exit_marker
.mark_exit_marker:
        mov     al, '>'
.draw_exit_marker:
        call    draw_char
        mov     si, menu_exit
        call    draw_string

        mov     di, 180 * 320 + 50
        mov     si, menu_hint
        call    draw_string

        ret

clear_screen:
        cld                                     ; clear direction flag (we want to fill from VRAM byte 0 to byte 63999 -> DF=0 = increment)
        mov     di, 0                           ; destination index (DI) is at the beginning of the framebuffer (ES:DI)
        mov     al, 0x0C                        ; load the "light red" color as default for game
        cmp     byte [game_state], STATE_PLAYING
        je      .fill_screen
        mov     al, 0x00                        ; load the "black" color as default for menu and game over
.fill_screen:
        mov     cx, 32000                       ; 320 x 200 screen = 32000 pixels * 2 words
        mov     ah, al                          ; duplicate AL on both sides of AX
        rep     stosw                           ; repeat stosw (AX content) until CX=0

        ret

reset_game:
        mov     byte [countdown_value], 3
        mov     byte [countdown_timer], 60      ; 60 frames = 1 sec at 60FPS
        mov     word [ball_x], 158
        mov     word [ball_y], 98
        mov     word [ball_dx], 2
        mov     word [ball_dy], 2
        mov     word [right_paddle_y], 80
        mov     word [left_paddle_y], 80
        mov     byte [right_score], 0
        mov     byte [left_score], 0
        mov     byte [jingle_index], 0

        ret

_main_loop:
        call    clear_screen
        cmp     byte [game_state], STATE_COUNTDOWN
        je      .countdown_branch
        cmp     byte [game_state], STATE_MENU
        je      .menu_branch
        cmp     byte [game_state], STATE_GAMEOVER
        je      .gameover_branch
        cmp     byte [game_state], STATE_PAUSED
        je      .paused_branch

        ; playing
        call    draw_playing
        call    update_ball_x_position
        call    update_ball_y_position
        call    poll_keyboard
        jmp     .sync
.paused_branch:
        call    draw_playing
        call    handle_pause_input
        jmp     .sync
.countdown_branch:
        call    draw_countdown
        call    update_countdown
        jmp     .sync
.menu_branch:
        call    draw_menu
        call    handle_menu_input
        jmp     .sync
.gameover_branch:
        call    draw_gameover
        call    play_jingle
        call    handle_gameover_input
        jmp     .sync
.sync:
        call    blit_to_vram
        call    wait_bios_tick
        call    tick_sound
        call    vsync
        jmp     _main_loop

update_countdown:
        dec     word [countdown_timer]
        jnz     .done

        dec     byte [countdown_value]
        jz      .start_game

        mov     word [countdown_timer], 60
        ret
.start_game:
        mov     byte [show_vbar_and_ball], 1
        mov     byte [game_state], STATE_PLAYING
.done:
        ret

; variables
game_state:         db      0                   ; 0 is menu, 1 is game
menu_selection:     db      0                   ; 0 = Start, 1 = Exit
menu_title:         db      "PADDLE16", 0
menu_start:         db      "START", 0
menu_exit:          db      "EXIT", 0
menu_hint:          db      "SPACE PICK / UP-DOWN MOVE / P PAUSE", 0
countdown_timer:    dw      0
countdown_value:    db      3
ball_x:             dw      158                 ; define ball x position
ball_y:             dw      98                  ; defined ball y position
ball_dx:            dw      2                   ; define ball horizontal velocity
ball_dy:            dw      2                   ; define ball vertical velocity
right_paddle_y:     dw      80                  ; initial Y position for right paddle
left_paddle_y:      dw      80                  ; initial Y position for left paddle
right_score:        db      0
left_score:         db      0
winner:             db      0                   ; 0 = left, 1 = right
gameover_left:      db      "WINNER: LEFT", 0
gameover_right:     db      "WINNER: RIGHT", 0
press_space:        db      "PRESS SPACE", 0
show_vbar_and_ball: db      1
