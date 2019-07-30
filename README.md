# Toynix

## 0 Introduction

Toynix is a tiny kernel (which was programmed just for fun) in Unix-like interface. It is composed of simplified fs and network module in micro-kernel spirit.

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

The tiny boot image is based on x86 architecture (that only occupies one sector(512 bytes)).

## 7 User Guideline

### 7.1 Install Tool Chain

* apt-get install qemu
* apt-get install gcc
* apt-get install gcc-multilib

### 7.2 Built Option

* make qemu-nox
  > Run the kernel within terminal mode
* make qemu-nox-gdb
  > Run the kernel with debug mode
* make gdb
  > Run gdb and auto-link target QEMU

### 7.3 Shell Command Line

[Details about commands.](./readme/command_line.md)

## 8 Todo List

### 8.1 Features

1. fix lwip connect function by user/echotest.c
2. convert organization of fs data block into LIST
3. replace static lib with share lib
4. fine-gained lock instead of global kernel lock (page allocator, console driver, scheduler, IPC state)

### 8.2 Bug Report

1. Kernel lock sometimes is illegally unlocked.

## 9 Ported Modules Claim

* lwip network stack module
