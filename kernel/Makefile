KERN_LDFLAGS := $(LDFLAGS) -T $(KERNDIR)/kernel.ld -nostdlib

# entry.S must be first, so that it's the first code in the text segment!
#
# We also snatch the use of a couple handy source files
# from the lib directory, to avoid gratuitous code duplication.
KERN_SRCFILES := \
		$(KERNDIR)/entry.S \
		$(KERNDIR)/entry_pgdir.c \
		$(KERNDIR)/init.c \
		$(KERNDIR)/console.c \
		$(KERNDIR)/printf.c \
		$(KERNDIR)/monitor.c \
		$(KERNDIR)/kdebug.c \
		$(KERNDIR)/panic.c \
		$(KERNDIR)/env.c \
		$(KERNDIR)/trapentry.S \
		$(KERNDIR)/trap.c \
		$(KERNDIR)/pmap.c \
		$(KERNDIR)/kclock.c \
		$(KERNDIR)/syscall.c \
		$(KERNDIR)/mpentry.S \
		$(KERNDIR)/mpconfig.c \
		$(KERNDIR)/lapic.c \
		$(KERNDIR)/picirq.c \
		$(KERNDIR)/spinlock.c \
		$(KERNDIR)/sched.c \
		$(KERNDIR)/pci.c \
		$(KERNDIR)/time.c \
		$(KERNDIR)/e1000.c \
		$(KERNDIR)/vm.c \
		$(LIBDIR)/string.c \
		$(LIBDIR)/printfmt.c \
		$(LIBDIR)/readline.c \
		$(LIBDIR)/stack_protector.c \
		$(LIBDIR)/math.c \
		$(LIBDIR)/div64.c \

# Binary program images to embed within the kernel.
KERN_OBJFILES := $(patsubst $(KERNDIR)/%.c, $(OBJDIR)/$(KERNDIR)/%.o, $(KERN_SRCFILES))
KERN_OBJFILES := $(patsubst $(KERNDIR)/%.S, $(OBJDIR)/$(KERNDIR)/%.o, $(KERN_OBJFILES))
KERN_OBJFILES := $(patsubst $(LIBDIR)/%.c, $(OBJDIR)/$(KERNDIR)/%.o, $(KERN_OBJFILES))

# Binary program images to embed within the kernel.
KERN_BINFILES := \
		$(OBJDIR)/$(FSDIR)/fs \
		$(OBJDIR)/$(NETDIR)/ns \
		$(patsubst %.o, %, $(USER_OBJFILES)) \
		$(patsubst %.o, %, $(NET_TESTOBJS)) \

# How to build kernel object files
$(OBJDIR)/$(KERNDIR)/%.o: $(KERNDIR)/%.c
	@echo + cc[KERN] $<
	@mkdir -p $(@D)
	$(CC) -nostdinc $(KERN_CFLAGS) -c -o $@ $<

$(OBJDIR)/$(KERNDIR)/%.o: $(KERNDIR)/%.S
	@echo + as[KERN] $<
	@mkdir -p $(@D)
	$(CC) -nostdinc $(KERN_CFLAGS) -c -o $@ $<

$(OBJDIR)/$(KERNDIR)/%.o: $(LIBDIR)/%.c
	@echo + cc[KERN] $<
	@mkdir -p $(@D)
	$(CC) -nostdinc $(KERN_CFLAGS) -c -o $@ $<

# Special flags for kernel/init
$(OBJDIR)/$(KERNDIR)/init.o: override KERN_CFLAGS+=$(INIT_CFLAGS)
$(OBJDIR)/$(KERNDIR)/init.o: $(OBJDIR)/.vars.INIT_CFLAGS

# How to build the kernel itself
$(OBJDIR)/$(KERNDIR)/kernel: $(KERN_OBJFILES) $(KERN_BINFILES) $(KERNDIR)/kernel.ld
	@echo + ld[KERN] $@
	$(LD) -o $@ $(KERN_LDFLAGS) $(KERN_OBJFILES) $(GCC_LIB) -b binary $(KERN_BINFILES)
	$(OBJDUMP) -S $@ > $@.asm
	$(NM) -n $@ > $@.sym

# How to build the kernel disk image
$(OBJDIR)/$(KERNDIR)/kernel.img: $(OBJDIR)/$(KERNDIR)/kernel $(OBJDIR)/$(BOOTDIR)/boot
	@echo + mk[KERN] $@
	dd if=/dev/zero of=$(OBJDIR)/$(KERNDIR)/kernel.img~ count=10000 2>/dev/null
	dd if=$(OBJDIR)/$(BOOTDIR)/boot of=$(OBJDIR)/$(KERNDIR)/kernel.img~ conv=notrunc 2>/dev/null
	dd if=$(OBJDIR)/$(KERNDIR)/kernel of=$(OBJDIR)/$(KERNDIR)/kernel.img~ seek=1 conv=notrunc 2>/dev/null
	mv $(OBJDIR)/$(KERNDIR)/kernel.img~ $(OBJDIR)/$(KERNDIR)/kernel.img

all: $(OBJDIR)/$(KERNDIR)/kernel.img
