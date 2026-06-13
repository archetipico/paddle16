; ===========================================================================
; main.asm  --  here lives the main core of the application
; ===========================================================================

        bits    16                              ; 16-bit code mode
        org     0x7C00                          ; bootloader address (standard)

_initial_setup:
        cli                                     ; clear interrupt
        xor     ax, ax                          ; zero-ing AX
        mov     ds, ax                          ; data segment = 0
        mov     es, ax                          ; extra data segment = 0
        mov     ss, ax                          ; stack segment = 0
        mov     sp, 0x7C00                      ; stack pointer = 0x7C00 (on top, growing to the bottom)

        ; We need the following because we are using more than 512 Byte
        mov     ah, 0x02                        ; "read sectors" function
        mov     al, 4                           ; number of sectors required (4 * 512 Byte = 2048 Byte) -> Total of 2048 + 512 = 2560 Byte on RAM
        mov     ch, 0                           ; cylinder track = 0 -> first track
        mov     cl, 2                           ; initial sector = 2 -> "first to be read" (1 = boot)
        mov     dh, 0                           ; disk head position
        mov     bx, 0x7E00                      ; 0x7C00 + 0x200 (512 Byte)
        int     0x13                            ; call disk services
        jc      .disk_error                     ; jump if CF = 1

        ; We need special keyboard settings so the BIOS installs its own ISR at INT 0x09: we will override it
        mov     word [0x24], keyboard_isr       ; custom ISR offset
        mov     word [0x26], 0                  ; custom segment (bootloader has CS = 0)

        ; There are different emulators but we still want 60Hz (see "wait_bios_tick" in timing.inc for more informations)
        mov     al, 0x36                        ; 00 11 011 0 -> channel 0 | LSB before MSB mode | square wave generator | binary value
        out     0x43, al                        ; output AL to the PIT (Programmable Interval Timer)
        mov     al, 0xAE                        ; lo-byte of 0x4DAE for 60Hz (0x4DAE = 19886 = 60 FPS, 0x9B5D = 39773 = 30 FPS) -> PIT channel output frequency is 1193182 / 19886 = 60 Hz
        out     0x40, al                        ; output AL to Channel 0
        mov     al, 0x4D                        ; hi-byte of 0x4DAE for 60Hz
        out     0x40, al                        ; output AL to Channel 0

        sti                                     ; enable interrupt
        jmp     0x0000:_start_video_mode        ; 0x0000 is needed because we have multiple sectors (far jump)

.disk_error:
        jmp     $                               ; infinite loop for debugging

_loop_and_padding:
        jmp     $                               ; infinite loop (jump to self)
        times   510 - ($ - $$) db 0             ; bring the NASM pointer to position 510 -> "define byte 0 N times where N = 510 - (current_pointer - session_start_pointer)"
        dw      0xAA55                          ; emit 0xAA55 (standard marker to check for bootable source)

section .stage2 vstart=0x7E00 follows=.text

%define STATE_MENU 0
%define STATE_PLAYING 1
%define STATE_GAMEOVER 2
%define STATE_PAUSED 3
%define STATE_COUNTDOWN 4
%define VICTORY_SCORE 10

%include "include/video.inc"
%include "include/game.inc"
%include "include/physics.inc"
%include "include/input.inc"
%include "include/timing.inc"
%include "include/audio.inc"
