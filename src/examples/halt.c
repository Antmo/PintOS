/* halt.c

   Simple program to test whether running a user program works.
 	
   Just invokes a system call that shuts down the OS. */

#include <syscall.h>
#include <stdlib.h>
int
main (void)
{
  exit (42);
  exit (55);
  halt ();
  /* not reached */
}
