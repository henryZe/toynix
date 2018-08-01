# Toynix

## Introduction

Toynix is sourced from “toy” kernel (which was programmed in the beginning just for fun) in Unix-like interface. It is designed in micro-kernel spirit and in “exokernel” fashion.

Here are some critical features you might be concerned about:

## Support Multi-Task & Multi-CPUs

The Toynix kernel runs between user mode and kernel mode. It supports multiple user tasks running at the same time, and request the service from system. It is also designed to support and perform multi-CPUs hardware platform. The task scheduler adopts Round-Robin way.

## Trap-Framework

It’s easy and flexible for kernel codes to register and configure the trap and interrupt functions into kernel. It provides interrupt handler function with an independent exception stack in user space.

## Memory-Manage

Toynix supplies the general protection mechanism according to mapping privilege level, and only process itself and its parent process allowed to modify the specific process’s mapping.
Toynix even provides the programmable page fault function to user, and this largely promotes page mapping flexibility and compatibility for variable handle strategy.

## File-System

The file system is according to micro-kernel spirit, which is working in user space. The solution keeps kernel tiny & low coupling, and makes file system modular. The file system is simple but powerful enough to provide the basic features: creating, reading, writing, and deleting files organized in a hierarchical directory structure. And of course, it is without any license corrupt.

## Network Stack (Todo, in designing)

## Usage

### Necessary Tools

* apt-get install qemu
* apt-get install gcc
* apt-get install gcc-multilib

### Makefile Option

* make qemu-nox
  > Run the kernel
* make qemu-nox-gdb
  > Run the kernel with debug mode
* make gdb
  > Run gdb and auto-link target QEMU
* find . -name "*.[chS]" | xargs cat | wc -l
  > calculate code lines

## Todu List

* High Priority
  1. Network

* Medium Priority
  1. fine-gained lock instead of global kernel lock  
    a. page allocator  
    b. console driver  
    c. scheduler  
    d. IPC state  

## Ported Modules

* Bootloader configuration
* Multiple-CPU configuration
* IDE driver
* PCI Bus driver
* Port lwip/api lwip/core lwip/netif modules