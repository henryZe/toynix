#
# Copyright (C) 2018 Henry.Zeng <henryz_e@163.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the MIT License.

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
	python $(BOOTDIR)/sign.py $(OBJDIR)/$(BOOTDIR)/boot
