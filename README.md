# Toynix

## Introduction

## Usage

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
