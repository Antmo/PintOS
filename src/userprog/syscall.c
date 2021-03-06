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
#include "userprog/plist.h"

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

  //  if(!verify_fix_length(esp, sizeof(esp)) || !is_user_vaddr(esp))
  //    {
      //      sys_exit(-1, f);
      //      return;
      
  //    }

  //  if(!verify_fix_length(esp, 4) || !verify_fix_length(esp+1, argc[*esp]*4))
  //    process_exit(-1);

  if(!verify_fix_length(esp, sizeof(esp)))
    {
      process_exit(-1);
      thread_exit();
    }
  if(!verify_fix_length( esp+1, argc[*esp]*4 )) /* Magic ... actually exec missing fails without this ... */
    {
        process_exit(-1);
        thread_exit();
    }
  
  switch ( *esp ) {
  case SYS_HALT:
    sys_halt();
    break;
  case SYS_EXIT:
    sys_exit(esp[1], f);
    break;
  case SYS_EXEC:
    sys_exec((char*)esp[1], f);
    break;
  case SYS_PLIST:
    sys_plist();
    break;
  case SYS_SLEEP:
    sys_sleep(esp[1]);
    break;
  case SYS_WAIT:
    sys_wait(esp[1], f);
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
    printf ("# Executed an unknown system call!\n");
    printf ("# Stack top + 0: %d\n", esp[0]);
    printf ("# Stack top + 1: %d\n", esp[1]);
      
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
 
  /* close all open files in the open filelist */
  map_clear(&(tid->file_list));

  f->eax = status;

  process_exit(status); /* Update process list */
  thread_exit();
}

void
sys_sleep(int millis)
{
  timer_sleep(millis);
}

void
sys_wait(int pid, struct intr_frame* f)
{
  if (plist_is_child(&PROCESS_LIST,pid,thread_current()->pid)) 
    f->eax = process_wait(pid);
  else
    f->eax = -1;
      /* && PROCESS_LIST.content[pid]->ret_available )*/
  //  else
  //    sys_exit(-1,f);
}

void
sys_exec(const char* command_line, struct intr_frame* f)
{
  char* buf = command_line;
  int i;

  //  for(i = 0; i < sizeof(command_line); ++i)
  //    {
  //      if(buf == NULL || !is_user_vaddr(buf) || 
  //	 pagedir_get_page(thread_current()->pagedir, buf) == NULL )
  //	{
  //	  process_exit(-1);
  //	  thread_exit();
  //	}
  //      ++buf;
  //    }

  if(!verify_variable_length(command_line)) /* variable eller fixed?*/
    sys_exit(-1, f);
  else
    f->eax = process_execute(command_line);
}

void
sys_plist(void)
{
  process_print_list();
}

/*
 * Reads up to length characters and stores them in @buffer
 */
void
sys_read(int fd, char *buffer, unsigned length, struct intr_frame* f)
{
  //  if( !verify_fix_length(buffer,length) 
  //      || is_kernel_vaddr(buffer) || is_kernel_vaddr(buffer + length) )
  //    sys_exit(-1, f);

  if(!verify_fix_length(buffer, sizeof(buffer)))
    {
      //    process_exit(-1);
      //      thread_exit;
      sys_exit(-1, f);
    }

  char* buf = buffer;
  /* I don't know what I'm doing ...*/
  int i;
  for(i = 0; i < length; ++i)
    {
      if(buf == NULL || !is_user_vaddr(buf) || 
	 pagedir_get_page(thread_current()->pagedir, buf) == NULL )
	{
	  process_exit(-1);
	  thread_exit();
	}
      ++buf;
    }
  //  if(!is_user_vaddr(buffer+length-1))
  //     sys_exit(-1,f);
     
  //  if(!is_user_vaddr(f+3))
  //  sys_exit(-1,f); 
     
  if (fd == STDOUT_FILENO || fd == NULL)
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
  if( !verify_fix_length(buffer, sizeof(buffer)) )
    {
      //      sys_exit(-1, f);
      process_exit(-1);
      thread_exit();
    }
  if(fd == STDIN_FILENO /*|| buffer == NULL */) 
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
  if(!verify_variable_length(filename))
    sys_exit(-1, f);

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
  if(filename == NULL)
    sys_exit(-1, f);

  if( !verify_variable_length(filename) )
    sys_exit(-1, f);  
  else
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
  if(!verify_variable_length(filename))
    sys_exit(-1, f);
  else
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
