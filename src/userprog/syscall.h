#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "threads/interrupt.h"

void syscall_init (void);
void sys_halt(void);                                             // Shutdown the system
void sys_exit(int, struct intr_frame*);                          // Exit current thread
void sys_read(int, char*, unsigned, struct intr_frame*);         // 
void sys_exec(const char*, struct intr_frame*);
void sys_plist(void);
void sys_sleep(int);
void sys_wait(int, struct intr_frame*);
void sys_write(int, const char*, unsigned, struct intr_frame*);  // 
void sys_open(const char*, struct intr_frame*);
void sys_close(int);
void sys_remove(const char*, struct intr_frame*);
void sys_create(const char*, unsigned, struct intr_frame*);
void sys_seek(int, unsigned);
void sys_tell(int, struct intr_frame*);
void sys_filesize(int, struct intr_frame*);
#endif /* userprog/syscall.h */
