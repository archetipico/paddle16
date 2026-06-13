play_tone:
        push    ax
        mov     al, 0xB6                        ; 10 11 011 0 -> channel 2 | lobyte+hibyte access mode | square wave generator | binary value
        ; the PIT now expects a lobyte+hibyte configuration
        out     0x43, al                        ; write command to PIT
        pop     ax
        out     0x42, al                        ; send the divisor lobyte (0x42 is channel 2)
        mov     al, ah                          ; copy the hibyte
        out     0x42, al                        ; send the hibyte
        ; sending to 0x42 is not enough (that's just an electric signal) -> we must also turn on 2 bits on 0x61 (PPI / Programmable Peripheral Interface port B)
        in      al, 0x61
        or      al, 0x03
        out     0x61, al
        mov     [sound_remaining], cx

        ret

tick_sound:
        cmp     word [sound_remaining], 0       ; is there a sound playing?
        je      .done
        dec     word [sound_remaining]
        jnz     .done                           ; if the counter is positive, let the speaker playing -> go to the next frame
        in      al, 0x61                        ; read 0x61 to prepare to turn off the speaker
        and     al, 0xFC                        ; 1111 1100 -> selectively turning off the speaker
        out     0x61, al                        ; output the value
.done:
        ret

play_paddle_hit_sound:
        mov     ax, 0x0A98                      ; 440Hz
        mov     cx, 3
        call    play_tone

        ret

play_score_sound:
        mov     ax, 0x1530                      ; 220Hz
        mov     cx, 12
        call    play_tone

        ret

play_jingle:
        cmp     word [sound_remaining], 0
        jne     .done
        cmp     byte [jingle_index], JINGLE_LEN ; did we play all the notes?
        je      .done
        movzx   bx, byte [jingle_index]         ; indexing the jingle
        shl     bx, 2                           ; each entry = 2 words
        mov     ax, [jingle_data + bx]          ; n-th entry
        mov     cx, [jingle_data + bx + 2]      ; frame duration
        call    play_tone
        inc     byte [jingle_index]
.done:
        ret

; variables
sound_remaining: dw      0
jingle_index:    db      0
jingle_data:
        dw      0x0950, 8                       ; A4 440Hz
        dw      0x07F8, 8                       ; C5 523Hz
        dw      0x06B0, 8                       ; E5 659Hz
        dw      0x05F2, 12                      ; G5 784Hz
        dw      0x06B0, 6                       ; E5 659Hz
        dw      0x07F8, 6                       ; C5 523Hz
        dw      0x04C8, 16                      ; A5 880Hz
        dw      0x05F2, 8                       ; G5 784Hz
        dw      0x0474, 20                      ; C6 1047Hz
JINGLE_LEN equ     4
