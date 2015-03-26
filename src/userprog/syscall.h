#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "threads/interrupt.h"

void syscall_init (void);
void sys_halt(void);                                             // Shutdown the system
void sys_exit(int, struct intr_frame*);                          // Exit current thread
void sys_read(int, char*, unsigned, struct intr_frame*);         // 
void sys_write(int, const char*, unsigned, struct intr_frame*);  // 


#endif /* userprog/syscall.h */
