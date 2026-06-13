; Needed because QEMU and others manage their own 0x3DA
wait_bios_tick:
        mov     ax, [0x046C]                    ; 0x046C is where the BIOS keeps the timer counter
.check_tick:
        cmp     [0x046C], ax                    ; if the BIOS has not received a tick yet, AX = [0x046C]
        je      .check_tick

        ret

vsync:
        mov     dx, 0x3DA                       ; 0x3DA = "Input Status Register #1"
.wait_not_retrace:
        in      al, dx
        test    al, 0x08                        ; set ZF = 1 if 3-rd bit = 0, ZF = 0 otherwise
        jnz     .wait_not_retrace
.wait_retrace:
        in      al, dx
        test    al, 0x08
        jz      .wait_retrace

        ret
