/* SPDX-License-Identifier: MIT */
/*
 *  Copyright (C) 2018 Henry.Zeng <henryz_e@163.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the MIT License.
 */

#include <boot/asm.h>
#include <boot/mmu.h>

# Start the first CPU: switch to 32-bit protected mode, jump into C.
# The BIOS loads this code from the first sector of the hard disk into
# memory at physical address 0x7c00 and starts executing in real mode
# with %cs=0 %ip=7c00.

.code16				# Assemble for 16-bit mode
.global start
start:
	cli			# close interrupt (BIOS enabled interrupts)

	# clean segment registers
	xorw    %ax, %ax	# set %ax to zero
	movw    %ax, %ds	# data segment
	movw    %ax, %es	# extra segment
	movw    %ax, %ss	# stack segment

	# Physical address line A20 is tied to zero so that the first PCs
	# with 2 MB would run software that assumed 1 MB.  Undo that.
seta20.1:
	inb     $0x64, %al	# Wait for not busy
	testb   $0x2, %al
	jnz     seta20.1

	movb    $0xd1, %al	# 0xd1 -> port 0x64
	outb    %al, $0x64

seta20.2:
	inb     $0x64, %al	# Wait for not busy
	testb   $0x2, %al
	jnz     seta20.2

	movb    $0xdf, %al	# 0xdf -> port 0x60
	outb    %al, $0x60

	# Switch from real to protected mode.  Use a bootstrap GDT that makes
	# virtual addresses map directly to physical addresses so that the
	# effective memory map doesn't change during the transition.
	lgdt    gdtdesc
	movl    %cr0, %eax
	orl     $CR0_PE, %eax
	movl    %eax, %cr0

	# PAGEBREAK!
	# Complete the transition to 32-bit protected mode by using a long jmp
	# to reload %cs and %eip.  The segment descriptors are set up with no
	# translation, so that the mapping is still the identity mapping.
	# SEG_KCODE is the index of gdt
	ljmp    $(SEG_KCODE << 3), $start32

.code32			# Tell assembler to generate 32-bit code now.
start32:
	# Set up the protected-mode data segment registers
	movw    $(SEG_KDATA << 3), %ax  # Our data segment selector
	movw    %ax, %ds                # -> DS: Data Segment
	movw    %ax, %es                # -> ES: Extra Segment
	movw    %ax, %ss                # -> SS: Stack Segment
	movw    $0, %ax                 # Zero segments not ready for use
	movw    %ax, %fs                # -> FS
	movw    %ax, %gs                # -> GS

	# Set up the stack pointer and call into C.
	movl    $start, %esp		# scratch space as stack
	call    bootmain

	# If bootmain returns (it shouldn't), trigger a Bochs
	# breakpoint if running under Bochs, then loop.
	movw    $0x8a00, %ax            # 0x8a00 -> port 0x8a00
	movw    %ax, %dx
	outw    %ax, %dx

	movw    $0x8ae0, %ax            # 0x8ae0 -> port 0x8a00
	outw    %ax, %dx

spin:
	jmp     spin

# Bootstrap GDT
.p2align 2				# force 4 byte alignment
gdt:
	SEG_NULLASM                             # null seg
	SEG_ASM(STA_X | STA_R, 0x0, 0xffffffff) # code seg, index 1, +x/+r, 0 ~ 0xffffffff
	SEG_ASM(STA_W, 0x0, 0xffffffff)         # data seg, index 2, +w, 0 ~ 0xffffffff

gdtdesc:
	.word   (gdtdesc - gdt - 1)             # sizeof(gdt) - 1
	.long   gdt                             # address gdt
