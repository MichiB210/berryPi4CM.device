#!/bin/sh
# Build berrypi4cm.device with m68k-amigaos-gcc (bebbo toolchain).
set -e
cd "$(dirname "$0")/src"
GCC=${GCC:-m68k-amigaos-gcc}
CFLAGS="-O2 -m68020 -fomit-frame-pointer -fno-builtin -ffreestanding -fno-strict-aliasing -noixemul -Wall -Wextra -Wno-unused-parameter"
$GCC $CFLAGS -c start.S -o start.o
for f in device hw channel xfer rh irq util; do
    $GCC $CFLAGS -c $f.c -o $f.o
done
$GCC -nostdlib -nostartfiles -noixemul start.o device.o hw.o channel.o xfer.o rh.o irq.o util.o -lgcc -o ../berrypi4cm.device
echo "built ../berrypi4cm.device"
cd ../tools
$GCC -O2 -m68020 -noixemul -Wall -o ../B4Diag B4Diag.c
echo "built ../B4Diag"
