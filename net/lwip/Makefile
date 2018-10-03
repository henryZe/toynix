LWIP_INCLUDES := \
	-I$(NETDIR)/lwip/include \
	-I$(NETDIR)/lwip/include/ipv4 \
	-I$(NETDIR)/lwip/ext \
	-I$(NETDIR)/lwip/toynix \
	-I$(INCDIR)

OBJDIRS += \
	$(NETDIR)/lwip \
	$(NETDIR)/lwip/api \
	$(NETDIR)/lwip/core \
	$(NETDIR)/lwip/core/ipv4 \
	$(NETDIR)/lwip/netif \

USER_INC += $(LWIP_INCLUDES)

LWIP_SRCFILES += \
		$(NETDIR)/lwip/api/api_lib.c \
		$(NETDIR)/lwip/api/api_msg.c \
		$(NETDIR)/lwip/api/err.c \
		$(NETDIR)/lwip/api/sockets.c \
		$(NETDIR)/lwip/api/tcpip.c \
		$(NETDIR)/lwip/api/netbuf.c \
		$(NETDIR)/lwip/core/init.c \
		$(NETDIR)/lwip/core/tcp_in.c \
		$(NETDIR)/lwip/core/dhcp.c \
		$(NETDIR)/lwip/core/mem.c \
		$(NETDIR)/lwip/core/memp.c \
		$(NETDIR)/lwip/core/netif.c \
		$(NETDIR)/lwip/core/pbuf.c \
		$(NETDIR)/lwip/core/raw.c \
		$(NETDIR)/lwip/core/stats.c \
		$(NETDIR)/lwip/core/sys.c \
		$(NETDIR)/lwip/core/tcp.c \
		$(NETDIR)/lwip/core/ipv4/ip_addr.c \
		$(NETDIR)/lwip/core/ipv4/icmp.c \
		$(NETDIR)/lwip/core/ipv4/ip.c \
		$(NETDIR)/lwip/core/ipv4/ip_frag.c \
		$(NETDIR)/lwip/core/ipv4/inet_chksum.c \
		$(NETDIR)/lwip/core/ipv4/inet.c \
		$(NETDIR)/lwip/core/tcp_out.c \
		$(NETDIR)/lwip/core/udp.c \
		$(NETDIR)/lwip/netif/etharp.c \
		$(NETDIR)/lwip/netif/loopif.c \
		$(NETDIR)/lwip/toynix/arch/thread.c \
		$(NETDIR)/lwip/toynix/arch/longjmp.S \
		$(NETDIR)/lwip/toynix/arch/sys_arch.c \
		$(NETDIR)/lwip/toynix/arch/perror.c \

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
