all: checkpatch

checkpatch:
	../linux-stable/scripts/checkpatch.pl -f -q */*.c */*/*.h */*.S

