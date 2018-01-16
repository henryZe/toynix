CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump                                                                    

CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -O2 -Wall -MD -ggdb -m32 -Werror -fno-omit-frame-pointer
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
ASFLAGS = -m32 -gdwarf-2 -Wa,-divide
LDFLAGS += -m $(shell $(LD) -V | grep elf_i386 2>/dev/null | head -n 1)

INCDIR = ../include

all: kernel

kernel: entry.S entry_pgdir.c
	$(CC) $(CFLAGS) -O -nostdinc -I$(INCDIR) -c entry_pgdir.c
	$(CC) $(CFLAGS) -nostdinc -I$(INCDIR) -c entry.S
	$(LD) $(LDFLAGS) -N -e entry -Ttext 0xf0100000 -o $@.o entry.o entry_pgdir.o
	$(OBJDUMP) -S $@.o > $@.asm
	$(OBJCOPY) -S -O binary -j .text $@.o $@

clean:
	@-rm *.d *.o *.asm kernel

