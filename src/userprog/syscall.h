#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void halt(void);    // Shutdown the system
void exit(int); // Exit current thread

#endif /* userprog/syscall.h */
