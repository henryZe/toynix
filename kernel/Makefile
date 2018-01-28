CC := $(TOOLPREFIX)gcc
AS := $(TOOLPREFIX)gas
LD := $(TOOLPREFIX)ld
OBJCOPY := $(TOOLPREFIX)objcopy
OBJDUMP := $(TOOLPREFIX)objdump
NM := $(GCCPREFIX)nm

CFLAGS = -pipe -nostdinc -O0 -fno-builtin -MD -fno-omit-frame-pointer -std=gnu99 -static -Wall -Wno-format -Wno-unused -Werror -gstabs -m32 -fno-tree-ch -fno-stack-protector -DJOS_KERNEL -gstabs
LDFLAGS += -m $(shell $(LD) -V | grep elf_i386 2>/dev/null | head -n 1)
KERN_LDFLAGS := $(LDFLAGS) -T kernel.ld -nostdlib
GCC_LIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

INCDIR = ../include

all: kernblock.img

# entry.S must be first, so that it's the first code in the text segment!!!
#
# We also snatch the use of a couple handy source files
# from the lib directory, to avoid gratuitous code duplication.
KERN_SRCFILES := entry.S \
			entry_pgdir.c \
			init.c \
			console.c \
			printf.c \
			monitor.c \
			kdebug.c \
			panic.c \
			pmap.c \
			kclock.c \
			../lib/printfmt.c \
			../lib/string.c \
			../lib/readline.c

KERN_OBJFILES := $(patsubst %.c, %.o, $(KERN_SRCFILES))
KERN_OBJFILES := $(patsubst %.S, %.o, $(KERN_OBJFILES))
KERN_OBJFILES := $(patsubst ../lib/%, %, $(KERN_OBJFILES))

kernblock: $(KERN_SRCFILES)
	@echo + mk $@
	$(CC) $(CFLAGS) -I$(INCDIR) -c $(KERN_SRCFILES)
	$(LD) -o $@ $(KERN_LDFLAGS) $(KERN_OBJFILES) $(GCC_LIB)
	$(OBJDUMP) -S $@ > $@.asm
	$(NM) -n $@ > $@.sym

kernblock.img: kernblock ../boot/bootblock
	@echo + mk $@
	dd if=/dev/zero of=kernblock.img~ count=10000 2>/dev/null
	dd if=../boot/bootblock of=kernblock.img~ conv=notrunc 2>/dev/null
	dd if=kernblock of=kernblock.img~ seek=1 conv=notrunc 2>/dev/null
	mv kernblock.img~ kernblock.img

clean:
	rm -rf *.d *.o *.asm *.sym kernblock kernblock.img
