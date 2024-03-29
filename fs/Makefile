FSOFILES := \
	$(OBJDIR)/$(FSDIR)/ide.o \
	$(OBJDIR)/$(FSDIR)/block_cache.o \
	$(OBJDIR)/$(FSDIR)/fs.o \
	$(OBJDIR)/$(FSDIR)/test.o \
	$(OBJDIR)/$(FSDIR)/serv.o \

USERAPPS := \
	$(OBJDIR)/$(USRDIR)/hello \
	$(OBJDIR)/$(USRDIR)/testpteshare \
	$(OBJDIR)/$(USRDIR)/faultio \
	$(OBJDIR)/$(USRDIR)/init \
	$(OBJDIR)/$(USRDIR)/echosrv \
	$(OBJDIR)/$(USRDIR)/echotest \
	$(OBJDIR)/$(USRDIR)/writemotd \
	$(OBJDIR)/$(USRDIR)/testmalloc \
	$(OBJDIR)/$(USRDIR)/testpipe \
	$(OBJDIR)/$(USRDIR)/testpiperace \
	$(OBJDIR)/$(USRDIR)/testpiperace2 \
	$(OBJDIR)/$(USRDIR)/pingpongs \
	$(OBJDIR)/$(USRDIR)/pingpong \
	$(OBJDIR)/$(USRDIR)/testuserstack \
	$(OBJDIR)/$(USRDIR)/breakpoint \
	$(OBJDIR)/$(USRDIR)/testprint \
	$(OBJDIR)/$(USRDIR)/signal_test \

FSIMGTXTFILES := \
	$(FSDIR)/newmotd \
	$(FSDIR)/motd \
	$(FSDIR)/lorem \
	$(FSDIR)/testshell.sh \
	$(FSDIR)/script \
	$(FSDIR)/testshell.key \
	$(FSDIR)/index.html \

BUSYBOX := \
	$(OBJDIR)/$(USRDIR)/sh \
	$(OBJDIR)/$(USRDIR)/echo \
	$(OBJDIR)/$(USRDIR)/cat \
	$(OBJDIR)/$(USRDIR)/num \
	$(OBJDIR)/$(USRDIR)/lsfd \
	$(OBJDIR)/$(USRDIR)/ls \
	$(OBJDIR)/$(USRDIR)/debug_info \
	$(OBJDIR)/$(USRDIR)/httpd \
	$(OBJDIR)/$(USRDIR)/pwd \
	$(OBJDIR)/$(USRDIR)/touch \
	$(OBJDIR)/$(USRDIR)/rm \
	$(OBJDIR)/$(USRDIR)/mkdir \
	$(OBJDIR)/$(USRDIR)/mv \

FSIMGFILES := $(BUSYBOX)
FSIMGFILES += $(USERAPPS) $(FSIMGTXTFILES)

# Compile FS object files
$(OBJDIR)/$(FSDIR)/%.o: $(FSDIR)/%.c $(OBJDIR)/.vars.USER_CFLAGS
	@echo + cc[USER] $<
	@mkdir -p $(@D)
	$(CC) -nostdinc $(USER_CFLAGS) -c -o $@ $<

# FS environment
$(OBJDIR)/$(FSDIR)/fs: $(FSOFILES) $(OBJDIR)/$(LIBDIR)/entry.o $(OBJDIR)/$(LIBDIR)/libtoynix.a $(USRDIR)/user.ld
	@echo + ld $@
	mkdir -p $(@D)
	$(LD) -o $@ $(USR_LDFLAGS) \
		$(OBJDIR)/$(LIBDIR)/entry.o $(FSOFILES) \
		-L$(OBJDIR)/$(LIBDIR) -ltoynix $(GCC_LIB)
	$(OBJDUMP) -S $@ > $@.asm
	$(NM) -n $@ > $@.sym

# How to build the file system image
$(OBJDIR)/$(FSDIR)/fsformat: $(FSDIR)/fsformat.c
	@echo + mk $(OBJDIR)/$(FSDIR)/fsformat
	mkdir -p $(@D)
	$(CC) $(NATIVE_CFLAGS) -o $(OBJDIR)/$(FSDIR)/fsformat $(FSDIR)/fsformat.c

# Generate image of 16MB (4096 * BLKSIZE)
$(OBJDIR)/$(FSDIR)/clean-fs.img: $(OBJDIR)/$(FSDIR)/fsformat $(FSIMGFILES)
	@echo + mk $(OBJDIR)/$(FSDIR)/clean-fs.img
	mkdir -p $(@D)
	$(OBJDIR)/$(FSDIR)/fsformat $(OBJDIR)/$(FSDIR)/clean-fs.img 4096 $(FSIMGFILES)

$(OBJDIR)/$(FSDIR)/fs.img: $(OBJDIR)/$(FSDIR)/clean-fs.img
	@echo + cp $(OBJDIR)/$(FSDIR)/clean-fs.img $@
	cp $< $@

all: $(OBJDIR)/$(FSDIR)/fs.img
