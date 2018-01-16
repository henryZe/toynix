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

# entry.S must be first, so that it's the first code in the text segment!!!
#
# We also snatch the use of a couple handy source files
# from the lib directory, to avoid gratuitous code duplication.
KERN_SRCFILES := entry.S \
			entry_pgdir.c \
			init.c \
			console.c \
			printf.c \
			../lib/printfmt.c \
			../lib/string.c

KERN_OBJFILES := $(patsubst %.c, %.o, $(KERN_SRCFILES))
KERN_OBJFILES := $(patsubst %.S, %.o, $(KERN_OBJFILES))
KERN_OBJFILES := $(patsubst ../lib/%, %, $(KERN_OBJFILES))

kernel: $(KERN_SRCFILES)
	$(CC) $(CFLAGS) -nostdinc -I$(INCDIR) -c $(KERN_SRCFILES)
	$(LD) $(LDFLAGS) -N -e entry -Ttext 0xf0100000 -o $@.o $(KERN_OBJFILES)
	$(OBJDUMP) -S $@.o > $@.asm
	$(OBJCOPY) -S -O binary -j .text $@.o $@

clean:
	rm -rf *.d *.o *.asm kernel

