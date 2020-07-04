# Toynix

## 0 Introduction

Toynix is a tiny kernel (which was programmed just for fun) in Unix-like interface. It is composed of simplified fs and network module in micro-kernel spirit. [**[Framework]**](./readme/framework.md)

Here are some critical features you might be concerned about:

## 1 Support Multi-Task & Multi-CPUs

Toynix kernel runs between user mode and kernel mode. It supports multiple user processes running at the same time and requesting system services. It is designed to work on multi-CPUs hardware. The task scheduler adopts Round-Robin strategy.

Within user land, it supports thread and ITC(inter-thread communication) for communication between threads (like semaphore, mail-box).

## 2 Trap-Framework

It’s easy and flexible to register trap and interrupt functions in kernel. It provides interrupt handler function with an independent exception stack in user space.

## 3 Memory-Manage

Toynix supplies the general protection mechanism according to mapping privilege level, and only process itself and its parent process allowed to modify the specific process’s mapping. Meanwhile, it offers IPC interface to communicate between processes.

Toynix even provides the programmable page fault interface for user, which massively promotes page mapping flexibility and compatibility for various handle strategy.

[Details about address space management.](./readme/mm.md)

## 4 File-System

The file system follows micro-kernel spirit, which is relied on an independent user process running in the background. This solution keeps kernel tiny and allows low coupling with kernel. The file system is simple but powerful enough to provide some basic features: creating, reading, writing, and deleting files which can be organized under directory structure.

## 5 Network

The network module follows micro-kernel spirit, which is relied on an independent user process listening in the background. This module is based on PCI network card. There is a simplified httpd server.

## 6 Boot Loader

The only one thing done by bootloader is reading the kernel image from disk. This tiny boot is based on x86 architecture (that only occupies one sector(512 bytes)).

## 7 How to Build it

### 7.1 Install Tool Chain

* apt-get install qemu
* apt-get install gcc
* apt-get install gcc-multilib

### 7.2 Built Option

* make qemu-nox
  * Run the kernel within terminal mode
* make qemu-nox-gdb
  * Run the kernel with debug mode
* make gdb
  * Run gdb and auto-link target QEMU

### 7.3 Shell Command Line

[Details about commands.](./readme/command_line.md)

## 8 Test Framework

(✔️: test pass ❌: test fail ❗: no test yet)

| Items | description | test file/function | Status |
| ----- | ----------- | ------ | ------ |
| bootloader |             |             | ✔️ |
| memory management |             |             | ✔️ |
| trap |             |             | ✔️ |
| multiple task & core | | | ✔️ |
| file system | | | ✔️ |
| network | | | ❌ |

## 9 Todo List

### 9.1 Optimize

* [ ] Implement signal operation set
* [ ] Replace static lib with share lib
* [ ] Implement recycling mechanism for page cache of fs block
  * [x] add file_close for releasing page cache
  * [ ] add list for recoding most recent access file
  * [ ] have ability to decide when to release
* [ ] Add VMA structure which describes a memory area:
  * [x] including start address and size
  * [ ] flags to determine access rights and behaviors (such as page_fault handler)
  * [ ] specifies which file is being mapped by the area, if any
* [ ] Use VMA pg_fault handler to replace global pg_fault handler
* [ ] Distinguish anonymous and mmap pages (whether need to copy original page)
* [ ] Modify map_segment from read to mmap images
* [ ] Fine-gained lock instead of global kernel lock:
  * [ ] page allocator
  * [ ] console driver
  * [ ] scheduler
  * [ ] IPC state
* [ ] Replace Makefile compiling framework with Scons (& menuconfig feature)
* [x] Support float print
* [x] Optimize malloc with fusion/split block method, which based on `sbrk`
* [x] Support directory operation:
  * [x] Allow to scan directory by `ls` command
  * [x] Allow to change workpath by `cd` command & `chdir` syscall
  * [x] Implement `mv` command & `rename` syscall
  * [x] Allow `mkdir` & `rmdir` directory operations
* [x] Support shell background run with `&` descriptor
* [x] Provide `debug_info` online method to show running status (such as mem, fs)
* [x] Implement file remove operation, and supply `rm` command

### 9.2 Bug

* Failed to build connection with server (lwip connect failed in user/echotest.c, server: user/echosrv.c)

## 10 Ported Modules Claim

* lwip network stack module
