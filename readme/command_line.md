# Command Line

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

* rm - remove files or directories

  ~~~ shell
  $ rm file1 file2
  ~~~
