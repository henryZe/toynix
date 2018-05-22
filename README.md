# toynix

toy kernel

## Usage

* make qemu-nox
  > Run the kernel
* make qemu-nox-gdb
  > Run the kernel with debug mode
* make gdb
  > Run gdb and auto-link target QEMU
* find . -name "*" | xargs cat | wc -l
  > calculate code lines

## Todu List

* High Priority
  1. IPC mechanism
  2. File System
  3. Network Stack

* Medium Priority
  1. fine-gained lock instead of global kernel lock
    a. page allocator
    b. console driver
    c. scheduler
    d. IPC state