# Linker flags for user programs
USR_LDFLAGS := $(LDFLAGS) -T $(USRDIR)/user.ld -nostdlib

USER_SRCFILES := \
	$(USRDIR)/initsh.c \

USER_TESTFILES := \
	$(USRDIR)/hello.c \
	$(USRDIR)/divzero.c \
	$(USRDIR)/badsegment.c \
	$(USRDIR)/softint.c \
	$(USRDIR)/faultread.c \
	$(USRDIR)/faultreadkernel.c \
	$(USRDIR)/faultwrite.c \
	$(USRDIR)/faultwritekernel.c \
	$(USRDIR)/breakpoint.c \
	$(USRDIR)/testbss.c \
	$(USRDIR)/buggyhello.c \
	$(USRDIR)/buggyhello2.c \
	$(USRDIR)/evilhello.c \
	$(USRDIR)/yield.c \
	$(USRDIR)/idle.c \
	$(USRDIR)/dumbfork.c \
	$(USRDIR)/faultdie.c \
	$(USRDIR)/faultalloc.c \
	$(USRDIR)/faultallocbad.c \
	$(USRDIR)/forktree.c \
	$(USRDIR)/spin.c \
	$(USRDIR)/faultregs.c \
	$(USRDIR)/faultnostack.c \
	$(USRDIR)/faultbadhandler.c \
	$(USRDIR)/faultevilhandler.c \
	$(USRDIR)/stresssched.c \
	$(USRDIR)/sendpage.c \
	$(USRDIR)/pingpong.c \
	$(USRDIR)/primes.c \
	$(USRDIR)/testfile.c \
	$(USRDIR)/spawnhello.c \
	$(USRDIR)/testpteshare.c \
	$(USRDIR)/testfdsharing.c \
	$(USRDIR)/testkbd.c \
	$(USRDIR)/icode.c \
	$(USRDIR)/spawnfaultio.c \
	$(USRDIR)/testshell.c \
	$(USRDIR)/primespipe.c \
	$(USRDIR)/testtime.c \
	$(USRDIR)/spawninit.c \
	$(USRDIR)/echosrv.c \
	$(USRDIR)/httpd.c \
	$(USRDIR)/fairness.c \
	$(USRDIR)/testprint.c \

USER_OBJFILES := $(patsubst $(USRDIR)/%.c, $(OBJDIR)/$(USRDIR)/%.o, $(USER_SRCFILES))
USER_OBJFILES += $(patsubst $(USRDIR)/%.c, $(OBJDIR)/$(USRDIR)/%.o, $(USER_TESTFILES))

$(OBJDIR)/$(USRDIR)/%.o: $(USRDIR)/%.c
	@echo + cc[USER] $<
	@mkdir -p $(@D)
	$(CC) -nostdinc $(USER_CFLAGS) -c -o $@ $<

$(OBJDIR)/$(USRDIR)/%: $(OBJDIR)/$(USRDIR)/%.o $(OBJDIR)/$(LIBDIR)/entry.o $(OBJDIR)/$(LIBDIR)/libtoynix.a $(OBJDIR)/$(LIBDIR)/liblwip.a $(USRDIR)/user.ld
	@echo + ld[USER] $@
	$(LD) -o $@.debug $(USR_LDFLAGS) $(OBJDIR)/$(LIBDIR)/entry.o $@.o -L$(OBJDIR)/$(LIBDIR) -ltoynix -llwip $(GCC_LIB)
	$(OBJDUMP) -S $@.debug > $@.asm
	$(NM) -n $@.debug > $@.sym
	$(OBJCOPY) $@.debug $@
	$(OBJCOPY) -R .stab -R .stabstr --add-gnu-debuglink=$(basename $@.debug) $@.debug $@
