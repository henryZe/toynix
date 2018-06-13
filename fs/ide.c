/*
 * Minimal PIO-based (non-interrupt-driven) IDE driver code.
 * For information about what all this IDE/ATA magic means,
 * see the materials available on the class references page.
 */

#include <lib.h>
#include <x86.h>

#define IDE_BSY		0x80
#define IDE_DRDY	0x40
#define IDE_DF		0x20
#define IDE_ERR		0x01

static int diskno = 1;

static int
ide_wait_ready(bool check_error)
{
	int ret;

	ret = inb(0x1F7);
	while ((ret & (IDE_BSY | IDE_DRDY)) != IDE_DRDY);

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
