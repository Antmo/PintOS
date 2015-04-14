#include <stdio.h>
#include <syscall-nr.h>
#include "userprog/syscall.h"
#include "threads/interrupt.h"
#include "threads/thread.h"

/* header files you probably need, they are not used yet */
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/vaddr.h"
#include "threads/init.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "devices/input.h"

static void syscall_handler (struct intr_frame *);


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


/* This array defined the number of arguments each syscall expects.
   For example, if you want to find out the number of arguments for
   the read system call you shall write:
   
   int sys_read_arg_count = argc[ SYS_READ ];
   
   All system calls have a name such as SYS_READ defined as an enum
   type, see `lib/syscall-nr.h'. Use them instead of numbers.
 */
const int argc[] = {
  /* basic calls */
  0, 1, 1, 1, 2, 1, 1, 1, 3, 3, 2, 1, 1, 
  /* not implemented */
  2, 1,    1, 1, 2, 1, 1,
  /* extended */
  0
};

static void
syscall_handler (struct intr_frame *f) 
{
  int32_t* esp = (int32_t*)f->esp;
  
  switch ( *esp ) {
  case SYS_HALT:
    sys_halt();
    break;
  case SYS_EXIT:
    sys_exit(esp[1], f);
    break;
  case SYS_READ:
    sys_read(esp[1],(char*)esp[2],esp[3], f);
    break;
  case SYS_WRITE:
    sys_write(esp[1],(const char*)esp[2],esp[3], f);
    break;
  case SYS_OPEN:
    sys_open((const char *)esp[1],f);
    break;
  case SYS_CLOSE:
    sys_close(esp[1]);
    break;
  case SYS_REMOVE:
    sys_remove((const char*)esp[1], f);
    break;
  case SYS_CREATE:
    sys_create((const char *)esp[1],esp[2],f);
    break;
  case SYS_TELL:
    sys_tell(esp[1],f);
    break;
  case SYS_SEEK:
    sys_seek(esp[1],esp[2]);
    break;
  case SYS_FILESIZE:
    sys_filesize(esp[1],f);
    break;
  default:
    printf ("Executed an unknown system call!\n");
    printf ("Stack top + 0: %d\n", esp[0]);
    printf ("Stack top + 1: %d\n", esp[1]);
      
    thread_exit ();
  }
}

void
sys_halt(void)
{
  power_off();
}

void
sys_exit(int status, struct intr_frame* f)
{
  /* get the current running thread */
  struct thread* tid = thread_current();
 
  printf("\n exit_status: %d\n",status);
  printf("\n this is the current running thread: %s\n",tid->name);
  
  /* close all open files in the open filelist */
  map_clear(&(tid->file_list));
  
  f->eax = status;
  thread_exit();  
}

/*
 * Reads up to length characters and stores them in @buffer
 */
void
sys_read(int fd, char *buffer, unsigned length, struct intr_frame* f)
{
  if (fd == STDOUT_FILENO)
    {
      f->eax = -1;
      return;
    }

  if (fd == STDIN_FILENO )
    {
      uint8_t ch;
      unsigned i;

      memset(buffer,0,length);
      for ( i = 0; i < length; ++i )
	{
	  ch = input_getc();
	  if ( ch == '\r')
	    ch = '\n';
	  buffer[i] = ch;
	  putbuf(buffer+i,1);
	}
      f->eax = i;
    }
  else
    {
      struct file* fp = map_find(&(thread_current()->file_list), fd);
      if (fp == NULL)
	f->eax = -1;
      else
	f->eax = file_read(fp, buffer, length);
    }
  
  return;
}

void
sys_write(int fd, const char *buffer, unsigned length, struct intr_frame* f)
{
  if(fd == STDIN_FILENO) 
    {
      f->eax = -1;
      return;
    }
  else if (fd == STDOUT_FILENO)
    {
      putbuf(buffer,length);
      f->eax = length;
    }
  else
    {
      struct file* fp = map_find(&(thread_current()->file_list),fd);
      if ( fp == NULL )
	{
	  f->eax = -1;
	  return;
	}
      f->eax = file_write( fp, buffer, length);
     }
  
   return;
}

void
sys_open(const char * filename, struct intr_frame* f)
{
  struct thread* ct = thread_current();
  struct file* fp = NULL;
  
  fp = filesys_open(filename);
  
  if( fp != NULL )
    {
      int result = map_insert(&ct->file_list, fp);
      f->eax = result;

      if ( result == -1 )  
	file_close(fp);
    }
  else
    f->eax = -1;

  return;
}

void
sys_create(const char* filename, unsigned size, struct intr_frame * f)
{
  f->eax = filesys_create(filename,size);
}

void
sys_close(int fd)
{
  
  struct file* fp = map_find(&(thread_current()->file_list),fd);
  if ( fp != NULL )
   {
    filesys_close(fp);
    map_remove(&(thread_current()->file_list),fd);
   }
}

void
sys_remove(const char* filename, struct intr_frame* f)
{
  f->eax = filesys_remove(filename);
}

void
sys_seek(int fd, unsigned position)
{
  struct file* fp = map_find(&(thread_current()->file_list),fd);
  if ( fp != NULL )
      file_seek(fp, position);
}

void
sys_tell(int fd, struct intr_frame* f)
{
  struct file* fp = map_find(&(thread_current()->file_list), fd);
  
  if ( fp != NULL )
    f->eax = file_tell(fp);
  else
    f->eax = -1;
}

void
sys_filesize(int fd, struct intr_frame* f)
{
  struct file* fp = map_find(&(thread_current()->file_list), fd);
  
  if ( fp != NULL )
    f->eax = file_length(fp);
  else
    f->eax = -1;
}
