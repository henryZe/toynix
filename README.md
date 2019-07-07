# Toynix

## Introduction

Toynix is sourced from “toy” kernel (which was programmed in the beginning just for fun) in Unix-like interface. It is designed in micro-kernel spirit and in “exokernel” fashion.

Here are some critical features you might be concerned about:

## Support Multi-Task & Multi-CPUs

The Toynix kernel runs between user mode and kernel mode. It supports multiple user tasks running at the same time and request the service from system. It is also designed to support and perform multi-CPUs hardware platform. The task scheduler adopts Round-Robin way.

## Trap-Framework

It’s easy and flexible for kernel codes to register and configure the trap and interrupt functions into kernel. It provides interrupt handler function with an independent exception stack in user space.

## Memory-Manage

Toynix supplies the general protection mechanism according to mapping privilege level, and only process itself and its parent process allowed to modify the specific process’s mapping.

Toynix even provides the programmable page fault function to user, and this largely promotes page mapping flexibility and compatibility for variable handle strategy.

[Details about address space management.](./readme/mm.md)

## File-System

The file system is according to micro-kernel spirit, which is working in user space. The solution keeps kernel tiny & low coupling and makes file system modular. The file system is simple but powerful enough to provide the basic features: creating, reading, writing, and deleting files organized in a hierarchical directory structure. And of course, it is without any license corrupt.

## Network Stack

Supports simple httpd server. (Including user-level thread, semaphore, mail-box, timer features and PCI network interface card).

## Usage

### Necessary Tools

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
* find . -name "*.[chS]" | xargs cat | wc -l
  > calculate code lines

### Command Line

* hello - just for debug

  ~~~ shell
  $ hello
  hello, world
  i am environment 00001008
  ~~~

* echo - display a line of text

  ~~~ shell
  $ echo content
  content
  ~~~

* cat - concatenate files and print on the standard output

  ~~~ shell
  $ cat file
  file content
  ~~~

* num - show line number of the specified file

  ~~~ shell
  $ cat file | num
     1 file content
  ~~~

* lsfd - display the occupying file descriptor and its property

  ~~~ shell
  $ lsfd
  fd 0: name <cons> isdir 0 size 0 dev cons
  fd 1: name <cons> isdir 0 size 0 dev cons
  ~~~

* ls - list directory contents

  ~~~ shell
  $ ls
  current directory: / 8192
  r            15652           hello
  r            26756           testpteshare
  r            15656           faultio
  r            32492           init
  r            26976           sh
  r            22212           echo
  r            22240           cat
  r            22296           num
  ~~~

* debug_info - show the current information of system

  ~~~ shell
  $ debug_info cpu
  select option: cpu
  CPU num: 1

  $ debug_info mem
  select option: mem
  Total Pages Num: 65536
  Free Pages Num: 63106
  Used Pages Num: 2430

  $ debug_info fs
  Total Blocks Num: 1024
  Used Blocks Num: 97
  ~~~

* sh - command interpreter

  ~~~ shell
  $ sh < script.sh
  perform script
  ~~~

* httpd - a web server and waits for the incoming server requests

  ~~~ shell
  $ httpd
  Waiting for http connections...
  ~~~

* pwd - print name of current/working directory

  ~~~ shell
  $ pwd
  /
  ~~~

* touch - change file timestamps

  ~~~ shell
  $ touch file1 file2
  $ ls
  r            0       file1
  r            0       file2
  ~~~

## Todo List

* High Priority
  1. BUG: Kernel lock sometimes is illegally unlocked.

* Medium Priority
  1. implement `mkdir` `rm` command
  2. support background run flag `&`
  3. implement recycling mechanism for page cache of fs block
  4. replace static lib with share lib
  5. fine-gained lock instead of global kernel lock
    a. page allocator  
    b. console driver  
    c. scheduler  
    d. IPC state  

## Ported Modules

* Port lwip modules
