Name: Manik Chaudhari



Assignment: To implement a operating system routinue related with process scheduling, memory management, disk schedulng, file management.

The project provides me knowledge of how operating system actually works, how it comunicates with user and hardware 
and also the knowledge of system calls, process scheduling, memory management, disk schedulng and file management.


Code files which i have written:

base.c    : contains SVC(), interrupt_handler(), fault_handler() and etc.
process.c : contains defination of process related functions.
process.h : declaration of global variables, process related function as well as queue related function
Queue.c   : contains defination of queue related functions.
page.h	  : contains some global variables and structures and function related to frame
page.c	  : contains defination of frame related functions.



'Doc' folder contain Architectural document which decribes all routines I have included in my design and flow of system calls,
memory management, disk scheduling and file system management.
'Project files' folder contain all source files. folder contain all source files.
'Images' folder contain two images of memory management and disk scheduling.

I have done test2a to test2g. All test run fine with expected output.
I also written additional test2i for file system which has open, create, close file system calls.

Enviornment for project : Windows.

command to compile code : gcc *.c -lm -o os   
command to run code : os test0/ os test1a/ os test1b/ os test1c/ os test1d/ os test1f/ os test1g/ os test1h / os test1i/ os test1j/ os test12a/ os test2b/ os test2c/ os test2d/ os test2e/os test2h                 