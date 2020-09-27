cme: Coronal Mass Exception - Inject unexpected bit-flips in a process.
=======================================================================
cme is a utility for simulating "random" bit-flips in a running program. This
tool can be used to test the resilience of your program towards unexpected
bit flips.

Notes
-----
* This idea was posed to me by a good friend of mine, thanks count!
* cme requires privileged access to run (it uses ptrace).
* cme is only expected to work on linux systems (systems that run
  a gnu flavor of libc and on systems that have a ptrace system call).

Usage
-----
Run cme on a process ID (pid) is already running running and specify a number
of unexpected bit-flips to introduce:

`./cme -p <process id of target> -n <number of bit-flips>` 

Building
--------
Run `make`

Resources
---------
* Examples of ptrace usage: 
  * http://visa.lab.asu.edu/gitlab/fstrace/fstrace/commit/31fa8a22b17b2f898513b68e04269597147d2478
  * https://android.googlesource.com/platform/bionic/+/master/tests/sys_ptrace_test.cpp


Contact
-------
Matt Davis: https://github.com/enferex
