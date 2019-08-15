include $(NETDIR)/lwip/Makefile

NET_SRCFILES := \
		$(NETDIR)/input.c \
		$(NETDIR)/output.c \
		$(NETDIR)/timer.c \

NET_TESTFILES := \
		$(NETDIR)/testoutput.c \
		$(NETDIR)/testinput.c \
		$(NETDIR)/testthread.c \

NET_OBJFILES := $(patsubst $(NETDIR)/%.c, $(OBJDIR)/$(NETDIR)/%.o, $(NET_SRCFILES))
NET_TESTOBJS := $(patsubst $(NETDIR)/%.c, $(OBJDIR)/$(NETDIR)/%.o, $(NET_TESTFILES))

$(OBJDIR)/$(NETDIR)/%.o: $(NETDIR)/%.c $(OBJDIR)/.vars.USER_CFLAGS $(OBJDIR)/.vars.NET_CFLAGS
	@echo + cc[USER] $<
	@mkdir -p $(@D)
	$(CC) -nostdinc $(USER_CFLAGS) $(NET_CFLAGS) -c -o $@ $<

$(OBJDIR)/$(NETDIR)/ns: $(OBJDIR)/$(NETDIR)/serv.o $(NET_OBJFILES) $(OBJDIR)/$(LIBDIR)/entry.o $(OBJDIR)/$(LIBDIR)/libtoynix.a $(OBJDIR)/$(LIBDIR)/liblwip.a $(USRDIR)/user.ld
	@echo + ld $@
	$(LD) -o $@ $(USR_LDFLAGS) $(LDFLAGS) -nostdlib \
		$(OBJDIR)/$(LIBDIR)/entry.o $< $(NET_OBJFILES) \
		-L$(OBJDIR)/$(LIBDIR) -llwip -ltoynix $(GCC_LIB)
	$(OBJDUMP) -S $@ > $@.asm
	$(NM) -n $@ > $@.sym

$(OBJDIR)/$(NETDIR)/test%: $(OBJDIR)/$(NETDIR)/test%.o $(NET_OBJFILES) $(OBJDIR)/$(LIBDIR)/entry.o $(OBJDIR)/$(LIBDIR)/libtoynix.a $(OBJDIR)/$(LIBDIR)/liblwip.a $(USRDIR)/user.ld
	@echo + ld $@
	$(LD) -o $@ $(ULDFLAGS) $(LDFLAGS) -nostdlib \
		$(OBJDIR)/$(LIBDIR)/entry.o $< $(NET_OBJFILES) \
		-L$(OBJDIR)/$(LIBDIR) -llwip -ltoynix $(GCC_LIB)
	$(OBJDUMP) -S $@ > $@.asm
