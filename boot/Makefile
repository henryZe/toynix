CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump                                                                    

CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -O2 -Wall -MD -ggdb -m32 -Werror -fno-omit-frame-pointer
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
ASFLAGS = -m32 -gdwarf-2 -Wa,-divide
LDFLAGS += -m $(shell $(LD) -V | grep elf_i386 2>/dev/null | head -n 1)

all: bootblock

bootblock: boot.S
	$(CC) $(CFLAGS) -fno-pic -nostdinc -I../ -c boot.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o $@.o boot.o
	$(OBJDUMP) -S $@.o > $@.asm
	$(OBJCOPY) -S -O binary -j .text $@.o $@
	./sign.pl $@

clean:
	rm boot.o boot.asm

