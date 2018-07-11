include $(NETDIR)/lwip/Makefile

NET_SRCFILES := \
		$(NETDIR)/input.c \
		$(NETDIR)/output.c \

NET_OBJFILES := $(patsubst $(NETDIR)/%.c, $(OBJDIR)/$(NETDIR)/%.o, $(NET_SRCFILES))

$(OBJDIR)/$(NETDIR)/%.o: $(NETDIR)/%.c $(OBJDIR)/.vars.USER_CFLAGS $(OBJDIR)/.vars.NET_CFLAGS
	@echo + cc[USER] $<
	@mkdir -p $(@D)
	$(CC) -nostdinc $(USER_CFLAGS) $(NET_CFLAGS) -c -o $@ $<

$(OBJDIR)/$(NETDIR)/ns: $(OBJDIR)/$(NETDIR)/serv.o $(NET_OBJFILES) $(OBJDIR)/$(LIBDIR)/entry.o $(OBJDIR)/$(LIBDIR)/libtoynix.a $(OBJDIR)/$(LIBDIR)/liblwip.a $(USRDIR)/user.ld
	@echo + ld $@
	$(LD) -o $@ $(ULDFLAGS) $(LDFLAGS) -nostdlib \
		$(OBJDIR)/$(LIBDIR)/entry.o $< $(NET_OBJFILES) \
		-L$(OBJDIR)/$(LIBDIR) -llwip -ltoynix $(GCC_LIB)
	$(OBJDUMP) -S $@ >$@.asm

$(OBJDIR)/$(NETDIR)/test%: $(OBJDIR)/$(NETDIR)/test%.o $(NET_OBJFILES) $(OBJDIR)/$(LIBDIR)/entry.o $(OBJDIR)/$(LIBDIR)/libtoynix.a $(OBJDIR)/$(LIBDIR)/liblwip.a $(USRDIR)/user.ld
	@echo + ld $@
	$(LD) -o $@ $(ULDFLAGS) $(LDFLAGS) -nostdlib \
		$(OBJDIR)/$(LIBDIR)/entry.o $< $(NET_OBJFILES) \
		-L$(OBJDIR)/$(LIBDIR) -llwip -ltoynix $(GCC_LIB)
	$(OBJDUMP) -S $@ >$@.asm