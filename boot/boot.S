# Start the first CPU: switch to 32-bit protected mode, jump into C.
# The BIOS loads this code from the first sector of the hard disk into
# memory at physical address 0x7c00 and starts executing in real mode
# with %cs=0 %ip=7c00.

.code16					# Assemble for 16-bit mode
.global start
start:
	cli					# close interrupt (BIOS enabled interrupts)

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





