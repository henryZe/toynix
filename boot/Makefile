CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -fvar-tracking -fvar-tracking-assignments -O0 -g -Wall -MD -gdwarf-2 -m32 -Werror -fno-omit-frame-pointer
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
ASFLAGS = -m32 -gdwarf-2 -Wa,-divide
LDFLAGS += -m $(shell $(LD) -V | grep elf_i386 2>/dev/null | head -n 1)

INCDIR = ../include

all: bootblock

bootblock: boot.S bootmain.c
	$(CC) $(CFLAGS) -O -nostdinc -I$(INCDIR) -c bootmain.c
	$(CC) $(CFLAGS) -nostdinc -I$(INCDIR) -c boot.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o $@.o boot.o bootmain.o
	$(OBJDUMP) -S $@.o > $@.asm
	$(OBJCOPY) -S -O binary -j .text $@.o $@
	./sign.pl $@

clean:
	rm -rf *.d *.o *.asm bootblock

