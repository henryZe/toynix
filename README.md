# Toynix

## Introduction

Toynix is a tiny kernel (which was programmed just for fun) in Unix-like interface. It is composed of simplified fs and network module in micro-kernel spirit.

Here are some critical features you might be concerned about:

## Support Multi-Task & Multi-CPUs

Toynix kernel runs between user mode and kernel mode. It supports multiple user processes running at the same time and requesting system services. It is designed to work on multi-CPUs hardware. The task scheduler adopts Round-Robin strategy.

Within user land, it supports thread and ITC for communication between threads (like semaphore, mail-box).

## Trap-Framework

It’s easy and flexible to register trap and interrupt functions in kernel. It provides interrupt handler function with an independent exception stack in user space.

## Memory-Manage

Toynix supplies the general protection mechanism according to mapping privilege level, and only process itself and its parent process allowed to modify the specific process’s mapping. Meanwhile, it offers IPC interface to communicate between processes.

Toynix even provides the programmable page fault interface for user, which massively promotes page mapping flexibility and compatibility for various handle strategy.

[Details about address space management.](./readme/mm.md)

## File-System

The file system follows micro-kernel spirit, which is relied on an independent user process running in the background. This solution keeps kernel tiny and allows low coupling with kernel. The file system is simple but powerful enough to provide some basic features: creating, reading, writing, and deleting files which can be organized under directory structure.

## Network

The network module follows micro-kernel spirit, which is relied on an independent user process listening in the background. This module is based on PCI network card. There is a simplified httpd server.

## Guideline

### Install Tool Chain

* apt-get install qemu
* apt-get install gcc
* apt-get install gcc-multilib

### Makefile Option

* make qemu-nox
  > Run the kernel within terminal mode
* make qemu-nox-gdb
  > Run the kernel with debug mode
* make gdb
  > Run gdb and auto-link target QEMU

### Command Line

[Details about commands.](./readme/command_line.md)

## Todo List

### Features

1. support background run flag `&`
2. implement `mkdir` command
3. implement recycling mechanism for page cache of fs block
4. replace static lib with share lib
5. fine-gained lock instead of global kernel lock (page allocator, console driver, scheduler, IPC state)

### Bug Report

1. Kernel lock sometimes is illegally unlocked.

## Ported Modules

* Port lwip modules
