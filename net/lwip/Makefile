LWIP_INCLUDES := \
	-I./net/lwip/include \
	-I./net/lwip/include/ipv4 \
	-I./net/lwip/ext \
	-I./net/lwip/toynix \
	-I./include

OBJDIRS += \

USER_INC += $(LWIP_INCLUDES)

LWIP_SRCFILES += \

LWIP_OBJFILES := $(patsubst %.c, $(OBJDIR)/%.o, $(LWIP_SRCFILES))
LWIP_OBJFILES := $(patsubst %.S, $(OBJDIR)/%.o, $(LWIP_OBJFILES))

$(OBJDIR)/$(NETDIR)/lwip/%.o: $(NETDIR)/lwip/%.c $(OBJDIR)/.vars.USER_CFLAGS
	@echo + cc[USER] $<
	mkdir -p $(@D)
	$(CC) -nostdinc $(USER_CFLAGS) $(USER_INC) -c -o $@ $<

$(OBJDIR)/$(NETDIR)/lwip/%.o: $(NETDIR)/lwip/%.S $(OBJDIR)/.vars.USER_CFLAGS
	@echo + as[USER] $<
	@mkdir -p $(@D)
	$(CC) -nostdinc $(USER_CFLAGS) -c -o $@ $<

$(OBJDIR)/$(LIBDIR)/liblwip.a: $(LWIP_OBJFILES)
	@echo + ar $@
	mkdir -p $(@D)
	$(AR) r $@ $(LWIP_OBJFILES)