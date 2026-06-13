_start_video_mode:
        mov     ax, 0x0013                      ; set video mode (AH=0x00) with mode number AL=0x13
        int     0x10                            ; video BIOS interrupt
        mov     ax, 0x1000                      ; ES = back buffer (not VRAM)
        mov     es, ax                          ; ES is now the first framebuffer pixel
        jmp     _main_loop

blit_to_vram:
        mov     ax, 0x1000
        mov     ds, ax                          ; DS = back buffer source
        mov     ax, 0xA000                      ; framebuffer position is 0xA000 (standard)
        mov     es, ax                          ; ES = VRAM destination
        cld
        xor     si, si
        xor     di, di
        mov     cx, 32000                       ; 64000 / 2 = 32000 word
        rep     movsw                           ; atomic copy back buffer -> VRAM

        ; restore segments for the next frames
        mov     ax, 0x1000
        mov     es, ax                          ; ES goes back to back buffer
        xor     ax, ax
        mov     ds, ax                          ; DS goes back to 0 for variables use

        ret

; Offset = y * 320 + x
; BP = x-offset
; SI = y-offset
compute_position:
        mov     bx, bp                          ; now BX = BP
        shl     bp, 8                           ; BP = y * 2 ^ 8 = y * 256
        shl     bx, 6                           ; BX = y * 2 ^ 6 = y * 64
        add     bp, bx                          ; BP = y * 256 + y * 64 = y * 320
        add     bp, si                          ; BP = y * 256 + y * 64 + SI = y * 320 + SI
        mov     di, bp                          ; save result to DI

        ret

; AL = color
; DI = top-left offset
; DX = width
; BX = height
draw_rect:
        push    dx
        mov     si, 320
        sub     si, dx
        pop     dx
.row_loop:
        mov     cx, dx                          ; CX = DX
        rep     stosb                           ; write AL content from DI position CX times (aka "draw one line")
        add     di, si                          ; DI is now on the next line
        dec     bx                              ; one row drawn, decrease BX -> this also sets ZF = 1 if BX becomes 0
        jnz     .row_loop                       ; if ZF = 0 repeat

        ret

draw_score:
        mov     al, [left_score]                ; AL = current left score byte (0..255 but we expect 0.99)
        xor     ah, ah
        mov     bl, 10
        div     bl

        push    ax
        mov     di, 10 * 320 + 40               ; left score tens at pixel (40,10)
        call    draw_digit
        pop     ax
        mov     al, ah
        mov     di, 10 * 320 + 46               ; tens are 5px wide + 1px gap
        call    draw_digit

        ; same for right score
        mov     al, [right_score]
        xor     ah, ah
        mov     bl, 10
        div     bl

        push    ax
        mov     di, 10 * 320 + 268
        call    draw_digit
        pop     ax
        mov     al, ah
        mov     di, 10 * 320 + 274
        call    draw_digit

        ret

draw_vertical_bar:
        mov     cx, 25                          ; 25 segments (200 / 8)
        mov     di, 2 * 320 + 158               ; start at (158,2)
.dash_loop:
        push    cx
        mov     dx, 1                           ; dash width 1px
        mov     bx, 4                           ; dash height 4px
        mov     al, 0x0F
        call    draw_rect
        add     di, 320 * 4                     ; jump 4 rows below (4 gap)
        pop     cx
        loop    .dash_loop

        ret

draw_countdown:
        ; scene preview
        mov     byte [show_vbar_and_ball], 0
        call    draw_playing
        ; centered countdown digit
        mov     al, [countdown_value]
        mov     di, 90 * 320 + 155
        call    draw_digit

        ret

; SI = pointer to the glyph's 8 Bytes (1 Byte per row, top 5 bits used)
; DI = top-left framebuffer offset
; draws a 5x8 white glyph directly to VRAM, leaving the background untouched
draw_glyph:
        push    di
        mov     cx, 8                           ; outer loop is 8 rows tall
.row_loop:
        mov     al, [si]
        push    cx                              ; needed to count remaining rows
        push    di                              ; needed for re-position
        mov     cx, 5
.col_loop:
        shl     al, 1                           ; MSB goes to CF
        jnc     .skip_pixel
        mov     byte [es:di], 0x0F              ; draw a white pixel at position
.skip_pixel:
        inc     di                              ; go to the next pixel to the right
        loop    .col_loop

        pop     di
        add     di, 320                         ; next scan line
        inc     si                              ; next byte of the glyph
        pop     cx
        loop    .row_loop
        pop     di

        ret

; AL = digit (0-9)
; DI = framebuffer offset
draw_digit:
        push    si
        xor     ah, ah                          ; zero-ing AH -> AX = digit as word
        mov     bx, 8                           ; every glyph = 8 Byte
        mul     bx                              ; AX = digit * 8 = font offset
        mov     si, digits_font_data
        add     si, ax                          ; SI is now at the start of the requested glyph
        call    draw_glyph
        pop     si

        ret

; AL = ASCII character
; DI = framebuffer offset
; Supports [0-9A-Z\s] and ':'
; Advances DI by 6px (5px for the glyph + 1px for the gap)
draw_char:
        cmp     al, ':'
        je      .colon
        cmp     al, '>'
        je      .gt
        cmp     al, ' '
        je      .advance
        cmp     al, '0'                         ; below '0', skip
        jb      .advance
        cmp     al, '9'                         ; over '9', skip
        ja      .try_letter
        sub     al, '0'                         ; 0-9 = 0..9
        call    draw_digit
        jmp     .advance
.try_letter:
        and     al, 0xDF                        ; convert in uppercase every character (bit 5 -> 0)
        cmp     al, 'A'
        jb      .advance                        ; below 'A', skip
        cmp     al, 'Z'
        ja      .advance                        ; over 'Z', skip
        sub     al, 'A'                         ; A-Z = 0..25
        xor     ah, ah
        mov     bx, 8
        mul     bx
        mov     si, letters_font_data
        add     si, ax                          ; SI = letter's glyph
        call    draw_glyph
        jmp     .advance
.colon:
        mov     si, colon_font_data
        call    draw_glyph
        jmp     .advance
.gt:
        mov     si, gt_font_data
        call    draw_glyph
.advance:
        add     di, 6

        ret

; SI = pointer to zero-terminated string
; DI = first-character framebuffer offset
draw_string:
.loop:
        mov     al, [si]                        ; load current byte
        test    al, al                          ; is it the end of the string?
        jz      .done
        ; increase SI and draw character
        inc     si
        push    si
        call    draw_char
        pop     si
        jmp     .loop
.done:
        ret

; variables
digits_font_data:
        db      0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70 ; 0
        db      0x20, 0x60, 0x20, 0x20, 0x20, 0x20, 0x20, 0x70 ; 1
        db      0x70, 0x88, 0x08, 0x10, 0x20, 0x40, 0x80, 0xF8 ; 2
        db      0x70, 0x88, 0x08, 0x30, 0x08, 0x08, 0x88, 0x70 ; 3
        db      0x10, 0x30, 0x50, 0x90, 0xF8, 0x10, 0x10, 0x10 ; 4
        db      0xF8, 0x80, 0x80, 0xF0, 0x08, 0x08, 0x88, 0x70 ; 5
        db      0x30, 0x40, 0x80, 0xF0, 0x88, 0x88, 0x88, 0x70 ; 6
        db      0xF8, 0x08, 0x10, 0x10, 0x20, 0x20, 0x40, 0x40 ; 7
        db      0x70, 0x88, 0x88, 0x70, 0x88, 0x88, 0x88, 0x70 ; 8
        db      0x70, 0x88, 0x88, 0x78, 0x08, 0x08, 0x10, 0x60 ; 9
letters_font_data:
        db      0x70, 0x88, 0x88, 0x88, 0xF8, 0x88, 0x88, 0x88 ; A
        db      0xF0, 0x88, 0x88, 0xF0, 0x88, 0x88, 0x88, 0xF0 ; B
        db      0x70, 0x88, 0x80, 0x80, 0x80, 0x80, 0x88, 0x70 ; C
        db      0xF0, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0xF0 ; D
        db      0xF8, 0x80, 0x80, 0xF0, 0x80, 0x80, 0x80, 0xF8 ; E
        db      0xF8, 0x80, 0x80, 0xF0, 0x80, 0x80, 0x80, 0x80 ; F
        db      0x70, 0x88, 0x80, 0x80, 0xB8, 0x88, 0x88, 0x70 ; G
        db      0x88, 0x88, 0x88, 0xF8, 0x88, 0x88, 0x88, 0x88 ; H
        db      0x70, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x70 ; I
        db      0x38, 0x10, 0x10, 0x10, 0x10, 0x10, 0x90, 0x60 ; J
        db      0x88, 0x90, 0xA0, 0xC0, 0xA0, 0x90, 0x88, 0x88 ; K
        db      0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xF8 ; L
        db      0x88, 0xD8, 0xA8, 0xA8, 0x88, 0x88, 0x88, 0x88 ; M
        db      0x88, 0xC8, 0xA8, 0x98, 0x88, 0x88, 0x88, 0x88 ; N
        db      0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70 ; O
        db      0xF0, 0x88, 0x88, 0xF0, 0x80, 0x80, 0x80, 0x80 ; P
        db      0x70, 0x88, 0x88, 0x88, 0x88, 0xA8, 0x90, 0x68 ; Q
        db      0xF0, 0x88, 0x88, 0xF0, 0xA0, 0x90, 0x88, 0x88 ; R
        db      0x70, 0x88, 0x80, 0x70, 0x08, 0x08, 0x88, 0x70 ; S
        db      0xF8, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20 ; T
        db      0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70 ; U
        db      0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x50, 0x20 ; V
        db      0x88, 0x88, 0x88, 0x88, 0xA8, 0xA8, 0xA8, 0x50 ; W
        db      0x88, 0x88, 0x50, 0x20, 0x50, 0x50, 0x88, 0x88 ; X
        db      0x88, 0x88, 0x50, 0x20, 0x20, 0x20, 0x20, 0x20 ; Y
        db      0xF8, 0x08, 0x10, 0x20, 0x40, 0x80, 0x80, 0xF8 ; Z
colon_font_data:
        db      0x00, 0x00, 0x20, 0x00, 0x00, 0x20, 0x00, 0x00 ; :
gt_font_data:
        db      0x80, 0x40, 0x20, 0x10, 0x10, 0x20, 0x40, 0x80 ; >
