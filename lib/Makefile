LIB_SRCFILES :=	\
	$(LIBDIR)/entry.S \
	$(LIBDIR)/libmain.c \
	$(LIBDIR)/syscall.c \
	$(LIBDIR)/exit.c \
	$(LIBDIR)/panic.c \
	$(LIBDIR)/printfmt.c \
	$(LIBDIR)/printf.c \
	$(LIBDIR)/readline.c \
	$(LIBDIR)/string.c \
	$(LIBDIR)/pgfault.c \
	$(LIBDIR)/pfentry.S \
	$(LIBDIR)/fork.c \
	$(LIBDIR)/ipc.c \
	$(LIBDIR)/pageref.c \
	$(LIBDIR)/file.c \
	$(LIBDIR)/fd.c \
	$(LIBDIR)/spawn.c \
	$(LIBDIR)/wait.c \
	$(LIBDIR)/console.c \
	$(LIBDIR)/fprintf.c \
	$(LIBDIR)/pipe.c \
	$(LIBDIR)/args.c \
	$(LIBDIR)/malloc.c \
	$(LIBDIR)/nsipc.c \
	$(LIBDIR)/sockets.c \
	$(LIBDIR)/debug.c \
	$(LIBDIR)/perror.c \
	$(LIBDIR)/thread.c \
	$(LIBDIR)/longjmp.S \
	$(LIBDIR)/itc.c \
	$(LIBDIR)/sleep.c \
	$(LIBDIR)/math.c \
	$(LIBDIR)/div64.c \
	$(LIBDIR)/buddy.c \
	$(LIBDIR)/sbrk.c \
	$(LIBDIR)/signal.c \
	$(LIBDIR)/stack_protector.c \

LIB_OBJFILES := $(patsubst $(LIBDIR)/%.c, $(OBJDIR)/$(LIBDIR)/%.o, $(LIB_SRCFILES))
LIB_OBJFILES := $(patsubst $(LIBDIR)/%.S, $(OBJDIR)/$(LIBDIR)/%.o, $(LIB_OBJFILES))

$(OBJDIR)/$(LIBDIR)/%.o: $(LIBDIR)/%.c
	@echo + cc[USER] $<
	@mkdir -p $(@D)
	$(CC) -nostdinc $(USER_CFLAGS) -c -o $@ $<

$(OBJDIR)/$(LIBDIR)/%.o: $(LIBDIR)/%.S
	@echo + as[USER] $<
	@mkdir -p $(@D)
	$(CC) -nostdinc $(USER_CFLAGS) -c -o $@ $<

$(OBJDIR)/$(LIBDIR)/libtoynix.a: $(LIB_OBJFILES)
	@echo + ar[USER] $@
	$(AR) r $@ $(LIB_OBJFILES)
