CC := $(TOOLPREFIX)gcc
AS := $(TOOLPREFIX)gas
LD := $(TOOLPREFIX)ld
OBJCOPY := $(TOOLPREFIX)objcopy
OBJDUMP := $(TOOLPREFIX)objdump                                                                    
NM := $(GCCPREFIX)nm

CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -fvar-tracking -fvar-tracking-assignments -O0 -g -Wall -MD -gdwarf-2 -m32 -Werror -fno-omit-frame-pointer
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
ASFLAGS = -m32 -gdwarf-2 -Wa,-divide
LDFLAGS += -m $(shell $(LD) -V | grep elf_i386 2>/dev/null | head -n 1)
KERN_LDFLAGS := $(LDFLAGS) -T kernel.ld -nostdlib
GCC_LIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

INCDIR = ../include

all: kernblock install

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

kernblock: $(KERN_SRCFILES)
	$(CC) $(CFLAGS) -nostdinc -I$(INCDIR) -c $(KERN_SRCFILES)
	$(LD) -o $@ $(KERN_LDFLAGS) $(KERN_OBJFILES) $(GCC_LIB)
	$(OBJDUMP) -S $@ > $@.asm
	$(NM) -n $@ > $@.sym

install:
	cp kernblock /zhsa022/usrhome/henry.zeng/github/lab/obj/kern/kernel

clean:
	rm -rf *.d *.o *.asm *.sym kernblock

