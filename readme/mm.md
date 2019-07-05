# Address Space Management

```
 Virtual memory map:                                Permissions
                                                    kernel/user

    4 Gig -------->  +------------------------------+
                     |                              | RW/--
                     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                     :              .               :
                     :              .               :
                     :              .               :
                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~| RW/--
                     |                              | RW/--
                     |   Remapped Physical Memory   | RW/--
                     |                              | RW/--
    KERNBASE, ---->  +------------------------------+ 0xf0000000      --+
    KSTACKTOP        |     CPU0's Kernel Stack      | RW/--  KSTKSIZE   |
                     | - - - - - - - - - - - - - - -|                   |
                     |      Invalid Memory (*)      | --/--  KSTKGAP    |
                     +------------------------------+                   |
                     |     CPU1's Kernel Stack      | RW/--  KSTKSIZE   |
                     | - - - - - - - - - - - - - - -|                 PTSIZE
                     |      Invalid Memory (*)      | --/--  KSTKGAP    |
                     +------------------------------+                   |
                     :              .               :                   |
                     :              .               :                   |
    MMIOLIM ------>  +------------------------------+ 0xefc00000      --+
                     |       Memory-mapped I/O      | RW/--  PTSIZE
 ULIM, MMIOBASE -->  +------------------------------+ 0xef800000
                     |  Cur. Page Table (User R-)   | R-/R-  PTSIZE
    UVPT      ---->  +------------------------------+ 0xef400000
                     |          RO PAGES            | R-/R-  PTSIZE
    UPAGES    ---->  +------------------------------+ 0xef000000
                     |           RO ENVS            | R-/R-  PTSIZE
 UTOP,UENVS ------>  +------------------------------+ 0xeec00000
 UXSTACKTOP -/       |     User Exception Stack     | RW/RW  PGSIZE
                     +------------------------------+ 0xeebff000
                     |       Empty Memory (*)       | --/--  PGSIZE
    USTACKTOP  --->  +------------------------------+ 0xeebfe000
                     |      Normal User Stack       | RW/RW  PGSIZE
                     +------------------------------+ 0xeebfd000
                     .                              .
                     .                              .
                     +------------------------------+ 0xd0040000
                     |         Struct File          | RW/RW  MAXFD * PGSIZE
   FILEDATA ------>  +------------------------------+ 0xd0020000
                     |    Struct File Descriptor    | RW/RW  MAXFD * PGSIZE
    FDTABLE ------>  +------------------------------+ 0xd0000000
                     .                              .
                     .       Empty Memory (*)       .
  (DISKMAP, PKTMAP)  .                              .
        mend ----->  +------------------------------+ 0x10000000
                     |      Invalid Memory (*)      | --/--  PGSIZE
                     +------------------------------+ 0x0ffff000
                     |          Union Nsipc         | RW/RW  QUEUE_SIZE * PGSIZE
       REQVA ----->  +------------------------------+ 0x0ffeb000
                     :        Heap of Malloc        :
      mbegin ----->  +------------------------------+ 0x08000000
                     |         Program Data         |
                     |   .test/.rodata/.data/.bss   |
    UTEXT -------->  +------------------------------+ 0x00800000
    PFTEMP ------->  |       Empty Memory (*)       |        PTSIZE
                     |                              |
    UTEMP -------->  +------------------------------+ 0x00400000      --+
                     |       Empty Memory (*)       |                   |
                     | - - - - - - - - - - - - - - -|                   |
                     |  User STAB Data (optional)   |                 PTSIZE
    USTABDATA ---->  +------------------------------+ 0x00200000        |
                     |       Empty Memory (*)       |                   |
    0 ------------>  +------------------------------+                 --+

 (*) Note: The kernel ensures that "Invalid Memory" is *never* mapped.
     "Empty Memory" is normally unmapped, but user programs may map pages
     there if desired. User programs map pages temporarily at UTEMP.
```
