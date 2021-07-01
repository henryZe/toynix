/* Simple linker script for the Toynix kernel.
   See the GNU ld 'info' manual ("info ld") to learn the syntax. */

OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
	/* Link the kernel at this address: "." means the current address */
	/* VMA */
	. = 0xF0100000;

	/* AT(...) gives the load address of this section, which tells
	   the boot loader where to load the kernel in physical memory */
	/* LMA */
	.text : AT(0x100000) {
		PROVIDE(stext = .);
		*(.text .stub .text.* .gnu.linkonce.t.*)
		PROVIDE(etext = .);	/* Define the 'etext' symbol to this value */
	}

	.rodata : {
		PROVIDE(srodata = .);
		*(.rodata .rodata.* .gnu.linkonce.r.*)
		PROVIDE(erodata = .);
	}

	/* Include debugging information in kernel memory */
	.stab : {
		PROVIDE(__STAB_BEGIN__ = .);
		*(.stab);
		PROVIDE(__STAB_END__ = .);
		BYTE(0)		/* Force the linker to allocate space
				   for this section */
	}

	.stabstr : {
		PROVIDE(__STABSTR_BEGIN__ = .);
		*(.stabstr);
		PROVIDE(__STABSTR_END__ = .);
		BYTE(0)		/* Force the linker to allocate space
				   for this section */
	}

	/* Adjust the address for the data segment to the next page */
	. = ALIGN(0x1000);

	/* The data segment */
	.data : {
		PROVIDE(sdata = .);
		*(.data .data.*)
		PROVIDE(edata = .);
	}

	.bss : {
		PROVIDE(sbss = .);
		*(.dynbss)
		*(.bss .bss.*)
		*(COMMON)
		PROVIDE(end = .);
	}


	/DISCARD/ : {
		*(.eh_frame .note.GNU-stack .comment .note)
	}
}
