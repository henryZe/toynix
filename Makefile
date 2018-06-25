OBJDIR := obj
INCDIR := include
BOOTDIR := boot
KERNDIR := kernel
LIBDIR := lib
USRDIR := user
FSDIR := fs

CC	:= gcc -pipe
AS	:= as
AR	:= ar
LD	:= ld
OBJCOPY	:= objcopy
OBJDUMP	:= objdump
NM	:= nm

# Native commands
NATIVE_CFLAGS := $(CFLAGS) $(DEFS) $(LABDEFS) -I. -MD -Wall
TAR := gtar
PERL := perl

# Compiler flags
# -fno-builtin is required to avoid refs to undefined functions in the kernel.
# Only optimize to -O1 to discourage inlining, which complicates backtraces.
CFLAGS := $(CFLAGS) $(DEFS) $(LABDEFS) -O1 -fno-builtin -I$(INCDIR) -MD
CFLAGS += -fno-omit-frame-pointer
CFLAGS += -std=gnu99
CFLAGS += -static
CFLAGS += -Wall -Wno-format -Wno-unused -Werror -gstabs -m32
# -fno-tree-ch prevented gcc from sometimes reordering read_ebp() before
# mon_backtrace()'s function prologue on gcc version: (Debian 4.7.2-5) 4.7.2
CFLAGS += -fno-tree-ch

# Add -fno-stack-protector if the option exists.
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

# Common linker flags
LDFLAGS := -m elf_i386

GCC_LIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

# Make sure that 'all' is the first target
all:

KERN_CFLAGS := $(CFLAGS) -DTOYNIX_KERNEL -gstabs
USER_CFLAGS := $(CFLAGS) -DTOYNIX_USER -gstabs

# Update .vars.X if variable X has changed since the last make run.
#
# Rules that use variable X should depend on $(OBJDIR)/.vars.X.  If
# the variable's value has changed, this will update the vars file and
# force a rebuild of the rule that depends on it.
$(OBJDIR)/.vars.%: FORCE
	echo "$($*)" | cmp -s $@ || echo "$($*)" > $@
.PRECIOUS: $(OBJDIR)/.vars.%
.PHONY: FORCE

# Include Makefrags for subdirectories
include $(BOOTDIR)/Makefrag
include $(LIBDIR)/Makefrag
include $(USRDIR)/Makefrag
include $(FSDIR)/Makefrag
include $(KERNDIR)/Makefrag

clean:
	rm -rf $(OBJDIR) .gdbinit jos.in qemu.log

# try to infer the correct QEMU
ifndef QEMU
QEMU := $(shell if which qemu >/dev/null 2>&1; \
	then echo qemu; exit; \
        elif which qemu-system-i386 >/dev/null 2>&1; \
        then echo qemu-system-i386; exit; \
	else \
	qemu=/Applications/Q.app/Contents/MacOS/i386-softmmu.app/Contents/MacOS/i386-softmmu; \
	if test -x $$qemu; then echo $$qemu; exit; fi; fi; \
	echo "***" 1>&2; \
	echo "*** Error: Couldn't find a working QEMU executable." 1>&2; \
	echo "*** Is the directory containing the qemu binary in your PATH" 1>&2; \
	echo "*** or have you tried setting the QEMU variable in conf/env.mk?" 1>&2; \
	echo "***" 1>&2; exit 1)
endif

GDBPORT := $(shell expr `id -u` % 5000 + 25000)

CPUS ?= 1

# disk 0: kernel.img, disk 1: fs.img
QEMUOPTS = -m 256 -drive file=$(OBJDIR)/$(KERNDIR)/kernel.img,index=0,media=disk,format=raw -serial mon:stdio -gdb tcp::$(GDBPORT)
QEMUOPTS += $(shell if $(QEMU) -nographic -help | grep -q '^-D '; then echo '-D qemu.log'; fi)
IMAGES = $(OBJDIR)/$(KERNDIR)/kernel.img
QEMUOPTS += -smp $(CPUS)
QEMUOPTS += -drive file=$(OBJDIR)/$(FSDIR)/fs.img,index=1,media=disk,format=raw
IMAGES += $(OBJDIR)/$(FSDIR)/fs.img
QEMUOPTS += $(QEMUEXTRA)

# debug bootloader, or debug kernel in default
#DEBUG_BOOT = -e "s/obj\/kernel\/kernel/obj\/boot\/boot.out/"
.gdbinit: .gdbinit.tmpl
	sed -e "s/localhost:1234/localhost:$(GDBPORT)/" $(DEBUG_BOOT) < $^ > $@

gdb:
	gdb -n -x .gdbinit

pre-qemu: .gdbinit

qemu: $(IMAGES) pre-qemu
	$(QEMU) $(QEMUOPTS)

qemu-nox: $(IMAGES) pre-qemu
	@echo "***"
	@echo "*** Use Ctrl-a x to exit qemu"
	@echo "***"
	$(QEMU) -nographic $(QEMUOPTS)

qemu-gdb: $(IMAGES) pre-qemu
	@echo "***"
	@echo "*** Now run 'make gdb'." 1>&2
	@echo "***"
	$(QEMU) $(QEMUOPTS) -S

qemu-nox-gdb: $(IMAGES) pre-qemu
	@echo "***"
	@echo "*** Now run 'make gdb'." 1>&2
	@echo "***"
	$(QEMU) -nographic $(QEMUOPTS) -S

# For test runs
prep-%:
	$(MAKE) "INIT_CFLAGS=${INIT_CFLAGS} -DTEST=`case $* in *_*) echo $*;; *) echo user_$*;; esac`" $(IMAGES)

run-%-nox-gdb: prep-% pre-qemu
	$(QEMU) -nographic $(QEMUOPTS) -S

run-%-gdb: prep-% pre-qemu
	$(QEMU) $(QEMUOPTS) -S

run-%-nox: prep-% pre-qemu
	$(QEMU) -nographic $(QEMUOPTS)

run-%: prep-% pre-qemu
	$(QEMU) $(QEMUOPTS)

print-gdbport:
	@echo $(GDBPORT)
