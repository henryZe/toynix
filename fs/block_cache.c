#include <lib.h>
#include <fs/fs.h>

// Return the virtual address of this disk block.
void *
diskaddr(uint32_t blockno)
{
	if (blockno == 0 || (super && blockno >= super->s_nblocks))
		panic("bad block number %08x in %s", blockno, __func__);

	return (char *)DISKMAP + blockno * BLKSIZE;
}

// Is this virtual address mapped?
bool
va_is_mapped(void *va)
{
	return (uvpd[PDX(va)] & PTE_P) && (uvpt[PGNUM(va)] & PTE_P);
}

// Is this virtual address dirty?
bool
va_is_dirty(void *va)
{
	return (uvpt[PGNUM(va)] & PTE_D) != 0;
}

// Fault any disk block that is read in to memory by
// loading it from disk.
static void
bc_pgfault(struct UTrapframe *utf)
{
	void *addr = (void *)utf->utf_fault_va;
	uint32_t blockno = ((uint32_t)addr - DISKMAP) / BLKSIZE;
	int ret;
	void *block_addr = ROUNDDOWN(addr, PGSIZE);

	// Check that the fault was within the block cache region
	if (addr < (void *)DISKMAP || addr >= (void *)(DISKMAP + DISKSIZE))
		panic("page fault in FS: eip %08x, va %p, err %04x",
		      utf->utf_eip, addr, utf->utf_err);

	// Sanity check the block number
	if (super && blockno >= super->s_nblocks)
		panic("reading non-existent block %08x\n", blockno);

	// Allocate a page in the disk map region, read the contents
	// of the block from the disk into that page.
	ret = sys_page_alloc(0, block_addr, PTE_SYSCALL);
	if (ret < 0)
		panic("%s: sys_page_alloc %e", __func__, ret);

	ret = ide_read(blockno * BLKSECTS, block_addr, BLKSECTS);
	if (ret < 0)
		panic("%s: ide_read %e", __func__, ret);

	// Clear the dirty bit for the disk block page since we just read the
	// block from disk
	ret = sys_page_map(0, block_addr, 0, block_addr,
				uvpt[PGNUM(block_addr)] & PTE_SYSCALL);
	if (ret < 0)
		panic("in %s, sys_page_map: %e", __func__, ret);

	// Check that the block we read was allocated.
	// (exercise for the reader:
	//    why do we do this *after* reading the block in?)
	// A: when we read bitmap, we need to check after reading block
	if (bitmap && block_is_free(blockno))
		panic("reading free block %08x\n", blockno);
}

// Flush the contents of the block containing VA out to disk if
// necessary, then clear the PTE_D bit using sys_page_map.
// If the block is not in the block cache or is not dirty, does
// nothing.
void
flush_block(void *addr)
{
	int ret;
	void *block_addr = ROUNDDOWN(addr, PGSIZE);
	uint32_t blockno = ((uint32_t)addr - DISKMAP) / BLKSIZE;

	if (addr < (void *)DISKMAP || addr >= (void *)(DISKMAP + DISKSIZE))
		panic("%s of bad va %08x", __func__, addr);

	if (!va_is_mapped(addr) || !va_is_dirty(addr))
		return;

	ret = ide_write(blockno * BLKSECTS, block_addr, BLKSECTS);
	if (ret < 0)
		panic("%s: ide_write %e", __func__, ret);

	/* clear dirty bit */
	ret = sys_page_map(0, block_addr, 0, block_addr,
				uvpt[PGNUM(block_addr)] & PTE_SYSCALL);
	if (ret < 0)
		panic("%s: sys_page_map %e", __func__, ret);
}

// Test that the block cache works, by smashing the superblock and
// reading it back.
static void
check_bc(void)
{
	struct Super backup;

	// back up super block
	memmove(&backup, diskaddr(1), sizeof(backup));

	// smash it
	strcpy(diskaddr(1), "OOPS!\n");
	flush_block(diskaddr(1));
	flush_block(diskaddr(1));
	assert(va_is_mapped(diskaddr(1)));
	assert(!va_is_dirty(diskaddr(1)));

	// clear it out
	sys_page_unmap(0, diskaddr(1));
	assert(!va_is_mapped(diskaddr(1)));

	// read it back in
	assert(strcmp(diskaddr(1), "OOPS!\n") == 0);

	// fix it
	memmove(diskaddr(1), &backup, sizeof(backup));
	flush_block(diskaddr(1));

	// Now repeat the same experiment, but pass an unaligned address to
	// flush_block.

	// back up super block
	memmove(&backup, diskaddr(1), sizeof(backup));

	// smash it
	strcpy(diskaddr(1), "OOPS!\n");

	// Pass an unaligned address to flush_block.
	flush_block(diskaddr(1) + 20);
	assert(va_is_mapped(diskaddr(1)));

	// Skip the !va_is_dirty() check because it makes the bug somewhat
	// obscure and hence harder to debug.
	//assert(!va_is_dirty(diskaddr(1)));

	// clear it out
	sys_page_unmap(0, diskaddr(1));
	assert(!va_is_mapped(diskaddr(1)));

	// read it back in
	assert(strcmp(diskaddr(1), "OOPS!\n") == 0);

	// fix it
	memmove(diskaddr(1), &backup, sizeof(backup));
	flush_block(diskaddr(1));

	cprintf("block cache is good\n");
}

void
bc_init(void)
{
	struct Super super;

	set_pgfault_handler(bc_pgfault);
	sys_add_vma(0, DISKMAP, DISKSIZE, PTE_W);
	check_bc();

	// cache the super block by reading it once
	memmove(&super, diskaddr(1), sizeof(super));
}
