# paddle16

A two-player paddle game built in NASM that boots straight off the metal. No operating system, no runtime, no libraries.

It runs in 16-bit real mode, draws in VGA mode 0x13 (320x200, 256 colors), keeps a steady 60 FPS, reads the keyboard through its own interrupt handler, and beeps through the PC speaker. The whole project fits in five disk sectors (less than 2560 Bytes).

Play it in your browser: https://archetipico.github.io/paddle16/

## Controls

| Key        | Action                       |
|------------|------------------------------|
| W / S      | Left paddle up / down        |
| Up / Down  | Right paddle up / down       |
| Space      | Select in the menu           |
| P          | Pause and resume             |

## Build and run

You need an emulator to run it. Anything that boots a raw floppy image works. The commands below use [QEMU](https://www.qemu.org/).

```sh
nasm -f bin main.asm -o paddle16.img
qemu-system-i386 -audiodev pipewire,id=snd0 -machine pcspk-audiodev=snd0 \
  -drive format=raw,file=paddle16.img,if=floppy
```

If you want to put it on real hardware, write the image to a USB stick or floppy and boot from it. Remember: it expects a BIOS, not UEFI.
