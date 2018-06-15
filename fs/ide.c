/*
 * Minimal PIO-based (non-interrupt-driven) IDE driver code.
 * For information about what all this IDE/ATA magic means,
 * see the materials available on the class references page.
 */

#include <lib.h>
#include <x86.h>
#include <fs/fs.h>

#define IDE_BSY		0x80
#define IDE_DRDY	0x40
#define IDE_DF		0x20
#define IDE_ERR		0x01

static int diskno = 1;

static int
ide_wait_ready(bool check_error)
{
	int ret;

	do {
		ret = inb(0x1F7);
	} while ((ret & (IDE_BSY | IDE_DRDY)) != IDE_DRDY);

	if (check_error && (ret & (IDE_DF | IDE_ERR)))
		return -1;

	return 0;
}

bool
ide_probe_disk1(void)
{
	int ret, i;

	// wait for Device 0 to be ready
	ide_wait_ready(0);

	// switch to Device 1
	outb(0x1F6, 0xE0 | (1 << 4));

	// check for Device 1 to be ready for a while
	ret = inb(0x1F7);
	for (i = 0; (i < 1000) && (ret & (IDE_BSY | IDE_DF | IDE_ERR)); i++);

	// switch back to Device 0
	outb(0x1F6, 0xE0 | (0 << 4));

	cprintf("Device 1 presence: %d\n", (i < 1000));
	return (i < 1000);
}

void
ide_set_disk(int disk_no)
{
	if (disk_no != 0 && disk_no != 1)
		panic("bad disk number");

	diskno = disk_no;
}

int
ide_read(uint32_t secno, void *dst, size_t nsecs)
{
	int ret;

	assert(nsecs <= 256);

	ide_wait_ready(0);

	outb(0x1F2, nsecs);
	outb(0x1F3, secno & 0xFF);
	outb(0x1F4, (secno >> 8) & 0xFF);
	outb(0x1F5, (secno >> 16) & 0xFF);
	outb(0x1F6, 0xE0 | ((diskno&1)<<4) | ((secno>>24)&0x0F));
	outb(0x1F7, 0x20);	// CMD 0x20 means read sector

	for (; nsecs > 0; nsecs--, dst += SECTSIZE) {
		ret = ide_wait_ready(1);
		if (ret < 0)
			return ret;

		/* read sector length per operation */
		insl(0x1F0, dst, SECTSIZE / 4);
	}

	return 0;
}

int
ide_write(uint32_t secno, const void *src, size_t nsecs)
{
	int ret;

	assert(nsecs <= 256);

	ide_wait_ready(0);

	outb(0x1F2, nsecs);
	outb(0x1F3, secno & 0xFF);
	outb(0x1F4, (secno >> 8) & 0xFF);
	outb(0x1F5, (secno >> 16) & 0xFF);
	outb(0x1F6, 0xE0 | ((diskno&1)<<4) | ((secno>>24)&0x0F));
	outb(0x1F7, 0x30);	// CMD 0x30 means write sector

	for (; nsecs > 0; nsecs--, src += SECTSIZE) {
		ret = ide_wait_ready(1);
		if (ret < 0)
			return ret;

		outsl(0x1F0, src, SECTSIZE / 4);
	}

	return 0;
}
