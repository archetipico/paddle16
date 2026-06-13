update_ball_x_position:
        cmp     word [ball_x], 302              ; screen - ball_width - margin - bar_width = 320 - 4 - 10 - 4 = 302, result saved in FLAGS
        jl      .skip_right_flip                ; ball is inside the screen, skip the next instruction

        ; Avoid bug where the ball gets stuck on the right paddle
        ; "Is the ball moving towards the right paddle?"
        cmp     word [ball_dx], 0
        jle     .skip_right_flip

        mov     si, right_paddle_y
        mov     cl, 1
        call    paddle_bounce
.skip_right_flip:
        cmp     word [ball_x], 14
        jg      .skip_left_flip

        ; Avoid bug where the ball gets stuck on the left paddle
        ; "Is the ball moving towards the left paddle?"
        mov     si, [ball_dx]
        cmp     si, 0
        jge     .skip_left_flip

        mov     si, left_paddle_y
        mov     cl, 0
        call    paddle_bounce
.skip_left_flip:
        mov     bp, [ball_dx]
        add     [ball_x], bp                    ; update ball position
.goal_check:
        cmp     word [ball_x], 0                ; is the ball past the left border?
        jg      .no_left_goal

        ; reset ball at center (right goal)
        inc     byte [right_score]
        call    play_score_sound

        ; win check
        cmp     byte [right_score], VICTORY_SCORE
        jl      .skip_win_right
        mov     byte [winner], 1
        mov     byte [game_state], STATE_GAMEOVER
.skip_win_right:
        mov     word [ball_x], 160
        mov     word [ball_y], 100
        mov     word [ball_dx], 2               ; start towards left (the loser) with speed 2
.no_left_goal:
        cmp     word [ball_x], 316              ; is the ball past the right border?
        jl      .no_goal

        ; reset ball at center (left goal)
        inc     byte [left_score]
        call    play_score_sound

        ; win check
        cmp     byte [left_score], VICTORY_SCORE
        jl      .skip_win_left
        mov     byte [winner], 0
        mov     byte [game_state], STATE_GAMEOVER
.skip_win_left:
        mov     word [ball_x], 160
        mov     word [ball_y], 100
        mov     word [ball_dx], -2              ; start towards right (the loser) with speed 2
.no_goal:
        ret

; SI = paddle_y address variable
; CL = 1 -> righe paddle, 0 -> left paddle
paddle_bounce:
        mov     bp, [ball_y]                    ; BP = ball_y
        cmp     bp, [si]                        ; is the ball over the paddle?
        jl      .skip

        mov     bx, [si]
        add     bx, 40
        cmp     bp, bx                          ; is the ball below the paddle?
        jg      .skip

        ; ball speed depends on paddle's collision angle
        ; |offset|    = ball_y - (paddle_y + 20)          ; range -20..+20
        ; |ball_dy|   = |offset| / 4                      ; range -5..+5
        ; |ball_dx|   = 2 + |ball_dy| / 2                 ; range 2..4
        ; clamp |ball_dx| at 4
        ; |ball_dx|   = - |ball_dx| (for right paddle, |ball_dx| for left paddle)
        sub     bx, 20                          ; BX = paddle + 20 (paddle Y-center)
        sub     bp, bx                          ; BP = ball_y - paddle_Y_center = |offset|
        sar     bp, 2                           ; BP = |offset| / 4
        mov     [ball_dy], bp
        mov     ax, bp
        cmp     ax, 0
        jge     .dy_abs                         ; if >= 0 it is already |dy|
        neg     ax
.dy_abs:
        sar     ax, 1                           ; AX = |dy| / 2
        add     ax, 2                           ; AX = 2 + |dy| / 2
        cmp     ax, 4                           ; remember, the range is 2..4
        jle     .clamp
        mov     ax, 4                           ; clamp at 4
.clamp:
        test    cl, cl
        jz      .store
        neg     ax                              ; because it is the right paddle, skip for left paddle
.store:
        mov     [ball_dx], ax                   ; save ball_dx status
        call    play_paddle_hit_sound
.skip:
        ret

update_ball_y_position:
        cmp     word [ball_y], 196
        jl      .skip_bottom_flip
        neg     word [ball_dy]
.skip_bottom_flip:
        cmp     word [ball_y], 0
        jg      .skip_top_flip
        neg     word [ball_dy]
.skip_top_flip:
        mov     bp, [ball_dy]
        add     [ball_y], bp

        ret
