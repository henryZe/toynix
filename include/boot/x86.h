/*
 *  include/boot/x86.h
 *
 *  Copyright (C) 2018 Henry.Zeng <henryz_e@163.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the MIT License.
 */
// Routines to let C code use special x86 instructions.

#include <boot/mmu.h>

static inline uchar
inb(ushort port)
{
	uchar data;

	asm volatile("in %1,%0" : "=a" (data) : "d" (port));
	return data;
}

static inline void
insl(int port, void *addr, int cnt)
{
	asm volatile("cld; rep insl" :
				"=D" (addr), "=c" (cnt) :
				"d" (port), "0" (addr), "1" (cnt) :
				"memory", "cc");
}

static inline void
outb(ushort port, uchar data)
{
	asm volatile("out %0,%1" : : "a" (data), "d" (port));
}

static inline void
outw(ushort port, ushort data)
{
	asm volatile("out %0,%1" : : "a" (data), "d" (port));
}

static inline void
outsl(int port, const void *addr, int cnt)
{
	asm volatile("cld; rep outsl" :
				"=S" (addr), "=c" (cnt) :
				"d" (port), "0" (addr), "1" (cnt) :
				"cc");
}

static inline void
stosb(void *addr, int data, int cnt)
{
	asm volatile("cld; rep stosb" :
				"=D" (addr), "=c" (cnt) :
				"0" (addr), "1" (cnt), "a" (data) :
				"memory", "cc");
}

static inline void
stosl(void *addr, int data, int cnt)
{
	asm volatile("cld; rep stosl" :
				"=D" (addr), "=c" (cnt) :
				"0" (addr), "1" (cnt), "a" (data) :
				"memory", "cc");
}

static inline void
lgdt(struct segdesc *p, int size)
{
	volatile ushort pd[3];

	pd[0] = size-1;
	pd[1] = (uint)p;
	pd[2] = (uint)p >> 16;

	asm volatile("lgdt (%0)" : : "r" (pd));
}

static inline void
lidt(struct gatedesc *p, int size)
{
	volatile ushort pd[3];

	pd[0] = size-1;
	pd[1] = (uint)p;
	pd[2] = (uint)p >> 16;

	asm volatile("lidt (%0)" : : "r" (pd));
}

static inline void
ltr(ushort sel)
{
	asm volatile("ltr %0" : : "r" (sel));
}

static inline uint
readeflags(void)
{
	uint eflags;

	asm volatile("pushfl; popl %0" : "=r" (eflags));
	return eflags;
}

static inline void
loadgs(ushort v)
{
	asm volatile("movw %0, %%gs" : : "r" (v));
}

static inline void
cli(void)
{
	asm volatile("cli");
}

static inline void
sti(void)
{
	asm volatile("sti");
}

static inline uint
xchg(volatile uint *addr, uint newval)
{
	uint result;

	// The + in "+m" denotes a read-modify-write operand.
	asm volatile("lock; xchgl %0, %1" :
				"+m" (*addr), "=a" (result) :
				"1" (newval) :
				"cc");
	return result;
}

static inline uint
rcr2(void)
{
	uint val;

	asm volatile("movl %%cr2,%0" : "=r" (val));
	return val;
}

static inline void
lcr3(uint val)
{
	asm volatile("movl %0,%%cr3" : : "r" (val));
}
