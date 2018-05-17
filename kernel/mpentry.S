#include <mmu.h>
#include <memlayout.h>

###################################################################
# entry point for APs
###################################################################

# Each non-boot CPU ("AP") is started up in response to a STARTUP
# IPI from the boot CPU.  Section B.4.2 of the Multi-Processor
# Specification says that the AP will start in real mode with CS:IP
# set to XY00:0000, where XY is an 8-bit value sent with the
# STARTUP. Thus this code must start at a 4096-byte boundary.
#
# Because this code sets DS to zero, it must run from an address in
# the low 2^16 bytes of physical memory.
#
# boot_aps() (in init.c) copies this code to MPENTRY_PADDR (which
# satisfies the above restrictions).  Then, for each AP, it stores the
# address of the pre-allocated per-core stack in mpentry_kstack, sends
# the STARTUP IPI, and waits for this code to acknowledge that it has
# started (which happens in mp_main in init.c).
#
# This code is similar to boot/boot.S except that
#    - it does not need to enable A20
#    - it uses MPBOOTPHYS to calculate absolute addresses of its
#      symbols, rather than relying on the linker to fill them

#define RELOC(x) ((x) - KERNBASE)
#define MPBOOTPHYS(s) ((s) - mpentry_start + MPENTRY_PADDR)

.code16           
.globl mpentry_start
mpentry_start:
	cli

	# set ds/es/ss as 0
	xorw    %ax, %ax
	movw    %ax, %ds
	movw    %ax, %es
	movw    %ax, %ss

	# Turn on Protection Mode
	lgdt	MPBOOTPHYS(gdtdesc)
	movl	%cr0, %eax
	orl		$CR0_PE, %eax
	movl	%eax, %cr0

	ljmpl	$GD_KT, $(MPBOOTPHYS(start32))

.code32
start32:
	# set ds/es/ss as PROT_MODE_DSEG
	movw    $GD_KD, %ax
	movw    %ax, %ds
	movw    %ax, %es
	movw    %ax, %ss

	movw    $0, %ax
	movw    %ax, %fs
	movw    %ax, %gs

	# Set up initial page table. We cannot use kern_pgdir yet because
	# we are still running at a low EIP.
	movl	$(RELOC(entry_pgdir)), %eax
	movl	%eax, %cr3

	# Turn on Paging.
	movl    %cr0, %eax
	orl		$(CR0_PE | CR0_PG | CR0_WP), %eax
	movl	%eax, %cr0

	# Switch to the per-cpu stack allocated in boot_aps()
	movl	mpentry_kstack, %esp
	# nuke frame pointer
	movl	$0, %ebp

	# Call mp_main().
	movl	$mp_main, %eax
	# Do not directly call because it is still running at a low EIP.
	call	*%eax

	# If mp_main returns (it should not), let it loop.
spin:
	jmp		spin

# Bootstrap GDT
.p2align 2								# force 4 byte alignment
gdt:
	SEG_NULL							# null seg
	SEG(STA_X | STA_R, 0x0, 0xffffffff)	# code seg
	SEG(STA_W, 0x0, 0xffffffff)			# data seg

gdtdesc:
	.word   (gdtdesc - gdt - 1)			# sizeof(gdt) - 1
	.long	MPBOOTPHYS(gdt) 			# address gdt

.globl mpentry_end
mpentry_end:
	nop