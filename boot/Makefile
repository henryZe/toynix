BOOT_OBJS := $(OBJDIR)/$(BOOTDIR)/boot.o $(OBJDIR)/$(BOOTDIR)/bootmain.o

$(OBJDIR)/$(BOOTDIR)/%.o: $(BOOTDIR)/%.S
	@echo + as[BOOT] $<
	@mkdir -p $(@D)
	$(CC) -nostdinc $(BOOT_CFLAGS) -c -o $@ $<

$(OBJDIR)/$(BOOTDIR)/%.o: $(BOOTDIR)/%.c
	@echo + cc[BOOT] -Os $<
	$(CC) -nostdinc $(BOOT_CFLAGS) -Os -c -o $@ $<

$(OBJDIR)/$(BOOTDIR)/boot: $(BOOT_OBJS)
	@echo + ld[BOOT] $(BOOTDIR)/boot
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o $@.out $^
	$(OBJDUMP) -S $@.out >$@.asm
	$(OBJCOPY) -S -O binary -j .text $@.out $@
	perl $(BOOTDIR)/sign.pl $(OBJDIR)/$(BOOTDIR)/boot
