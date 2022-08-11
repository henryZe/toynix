# Command Line

1. sh - command interpreter  
  `$ sh [OPTIONS]...`

2. echo - display a line of text  
  `$ echo [STRING]...`

3. cat - concatenate files and print on the standard output  
  `$ cat [FILE]...`

4. num - show line number of the specified file  
  `$ cat file | num`

5. lsfd - display the occupying file descriptor and its property  
  `$ lsfd`

6. ls - list directory contents  
  `$ ls [-dFl] [file...]`

7. debug_info - show the current running status  
  `$ debug_info [cpu|mem|fs|env|vma]`

~~~
/$ /debug_info cpu
CPU   status      env name
  0  started     1005 ns_input
  1  started     2008 /debug_info
  2  started     1003 ns_timer
  3  started     2007 sh
~~~

~~~
/$ /debug_info mem
Total pages: 65536
 Free pages: 62830
 Used pages: 2706
      Usage: 4.129028%
~~~

~~~
/$ /debug_info fs
Total blocks: 1024
 Used blocks: 310
       Usage: 30.273438%
~~~

~~~
/$ /debug_info env
     Env             Name   Status CPU  Run times   Father      Father name
    1000               fs  pending  2       1974        0
    1001               ns  pending  2      17151        0
    1002           initsh  running  2     454048        0
    1003         ns_timer  running  0     448687     1001               ns
    1004               sh  waiting  2     456317     1002           initsh
    1005         ns_input  running  0     451158     1001               ns
    1006        ns_output  pending  0         16     1001               ns
    9007               sh  running  1        215     1004               sh
    8008      /debug_info  running  1         31     9007               sh
~~~

~~~
/$ /debug_info vma 1000
Env 1000:
VMA           Begin           Size            Perm
0             00200000           12bd2        6
1             00800000            6680        4
2             00807000            8000        6
3             eebf6000            8000        6
4             10000000        c0000000        6
~~~

8. httpd - a web server and waits for the incoming client requests  
  `$ httpd &`

9. pwd - print name of current/working directory  
  `$ pwd`

10. touch - create empty file  
  `$ touch FILE...`

11. rm - remove files or directories  
  `$ rm [FILE]...`

12. mkdir - make directories  
  `$ mkdir DIRECTORY...`

13. cd - switch current working directory  
  `$ cd DIRECTORY`

14. mv - move (rename) files  
  `$ mv SOURCE DIRECTORY`
