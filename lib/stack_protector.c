#include <assert.h>
#include <x86.h>

void __stack_chk_fail(void)
{
    panic("STACK ALREADY CORRUPTED sp: 0x%x\n", read_esp());
}
