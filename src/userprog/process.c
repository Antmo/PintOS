#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "userprog/gdt.h"      /* SEL_* constants */
#include "userprog/process.h"
#include "userprog/load.h"
#include "userprog/pagedir.h"  /* pagedir_activate etc. */
#include "userprog/tss.h"      /* tss_update */
#include "filesys/file.h"
#include "threads/flags.h"     /* FLAG_* constants */
#include "threads/thread.h"
#include "threads/vaddr.h"     /* PHYS_BASE */
#include "threads/interrupt.h" /* if_ */

/* Headers not yet used that you may need for various reasons. */
#include "threads/synch.h"
#include "threads/malloc.h"
#include "lib/kernel/list.h"

#include "userprog/flist.h"
#include "userprog/plist.h"

/* HACK defines code you must remove and implement in a proper way */
//#define HACK

struct plist PROCESS_LIST; /* Global process list */ 
 
/* This function is called at boot time (threads/init.c) to initialize
 * the process subsystem. */
void process_init(void)
{  
  plist_init(&PROCESS_LIST);
  lock_init(&PROCESS_LIST.phatlock);
}

/* This function is currently never called. As thread_exit does not
 * have an exit status parameter, this could be used to handle that
 * instead. Note however that all cleanup after a process must be done
 * in process_cleanup, and that process_cleanup are already called
 * from thread_exit - do not call cleanup twice! */
void process_exit(int status)
{
  struct plist* p = &PROCESS_LIST;
  int pid = thread_current()->pid;

  set_exit_status(p, status, pid);
}

/* Print a list of all running processes. The list shall include all
 * relevant debug information in a clean, readable format. */
void process_print_list()
{
  struct plist* p = &PROCESS_LIST;
  print_list(p);
}


struct parameters_to_start_process
{
  char* command_line;
  struct semaphore sema;
  bool load;
  int par_id;
};

static void
start_process(struct parameters_to_start_process* parameters) NO_RETURN;

/* Starts a new proccess by creating a new thread to run it. The
   process is loaded from the file specified in the COMMAND_LINE and
   started with the arguments on the COMMAND_LINE. The new thread may
   be scheduled (and may even exit) before process_execute() returns.
   Returns the new process's thread id, or TID_ERROR if the thread
   cannot be created. */
int
process_execute (const char *command_line) 
{
  char debug_name[64];
  int command_line_size = strlen(command_line) + 1;
  tid_t thread_id = -1;
  int  process_id = -1;

  /* LOCAL variable will cease existence when function return! */
  struct parameters_to_start_process arguments;

  debug("%s#%d: process_execute(\"%s\") ENTERED\n",
        thread_current()->name,
        thread_current()->tid,
        command_line);

  /* Init semaphore & load variable*/
  sema_init(&arguments.sema, 0);
  arguments.load = false;

  /* Store parents(this) thread id, used in child process later*/
  arguments.par_id = thread_current()->pid;

  /* COPY command line out of parent process memory */
  arguments.command_line = malloc(command_line_size);
  strlcpy(arguments.command_line, command_line, command_line_size);


  strlcpy_first_word (debug_name, command_line, 64);

      
  /* SCHEDULES function `start_process' to run (LATER) */
  thread_id = thread_create (debug_name, PRI_DEFAULT,
                             (thread_func*)start_process, &arguments);
  
  /* If thread_create was successful, wait for child process */

  if(thread_id != -1)
    {
      //      printf("before sema up proc exefh\n");
      sema_down(&arguments.sema);
      //printf("after semadown process_execute\n");
      if(arguments.load)
	process_id = arguments.par_id;
      else
	process_id = -1;
    }
  /* is not really the parent id anymore lol just convenient */
  

  /* AVOID bad stuff by turning off. YOU will fix this! */
  //power_off();
  
  
  /* WHICH thread may still be using this right now? */
  /* - da child thread */  

  free(arguments.command_line);

  debug("%s#%d: process_execute(\"%s\") RETURNS %d\n",
        thread_current()->name,
        thread_current()->tid,
        command_line, process_id);

  /* Check if child process was successful */
  //  if(!arguments.load)
  //    process_id = -1;

  /* MUST be -1 if `load' in `start_process' return false */
  return process_id;
}

/* A thread function that loads a user process and starts it
   running. */
static void
start_process (struct parameters_to_start_process* parameters)
{
  /* The last argument passed to thread_create is received here... */
  struct intr_frame if_;
  bool success;
  char file_name[64];
  strlcpy_first_word (file_name, parameters->command_line, 64);
  
  debug("%s#%d: start_process(\"%s\") ENTERED\n",
        thread_current()->name,
        thread_current()->tid,
        parameters->command_line);
  
  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;


  success = load (file_name, &if_.eip, &if_.esp);

  parameters->load = success;

  debug("%s#%d: start_process(...): load returned %d\n",
        thread_current()->name,
        thread_current()->tid,
        success);
      
  /* Insert the newly created process(thread) in the system-wide file list */

  struct pinfos temp;
  temp.parent_id = parameters->par_id;
  temp.exit_status = -1;
  temp.name = thread_current()->name;
  temp.alive = true;
  temp.parent_alive = true;

  /* debug("#before inserting into PROCESS_LIST\n"); */ 
  thread_current()->pid = plist_insert( &PROCESS_LIST,&temp );
  /* debug("#after inserting into PROCESS_LIST\n"); */
  
  parameters->par_id = thread_current()->pid;

  /* Check if list was full, we do not allow more than LIST_SIZE processes to run concurrently */
  if(thread_current()->pid == -1)
    {
      success = false;
      parameters->load = false;
      debug("# ERROR: System-wide file list was full, killing thread\n");
    }
  
  if (success)
  {
    if_.esp = setup_main_stack(parameters->command_line, (void *) PHYS_BASE);

    /* dump_stack ( PHYS_BASE + 15, PHYS_BASE - if_.esp + 16 ); */
  }
  /* Might be something wrong here, 
     if threads execute in a specific order 
     we crash in exec-corrupt / exec missing */  
  debug("%s#%d: start_process(\"%s\") DONE\n",
        thread_current()->name,
        thread_current()->tid,
        parameters->command_line);
  

  /* If load fail, quit. Load may fail for several reasons.
     Some simple examples:
     - File doeas not exist
     - File do not contain a valid program
     - Not enough memory
  */
  if ( ! success )
  {
    sema_up(&parameters->sema);
    // printf("after semaup startproc\n");
    thread_exit ();
  }
  
  /* Start the user process by simulating a return from an interrupt,
     implemented by intr_exit (in threads/intr-stubs.S). Because
     intr_exit takes all of its arguments on the stack in the form of
     a `struct intr_frame', we just point the stack pointer (%esp) to
     our stack frame and jump to it. */

  /* Allow parent process to continue */
  sema_up(&parameters->sema);
  //printf("after sema up start proc\n");
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}

/* Wait for process `child_id' to die and then return its exit
   status. If it was terminated by the kernel (i.e. killed due to an
   exception), return -1. If `child_id' is invalid or if it was not a
   child of the calling process, or if process_wait() has already been
   successfully called for the given `child_id', return -1
   immediately, without waiting.

   This function will be implemented last, after a communication
   mechanism between parent and child is established. */
int
process_wait (int child_id)
{
  struct plist* p = &PROCESS_LIST;
  int status = -1;
  struct thread *cur = thread_current ();
  int pid = cur->pid;
  
    
  debug("%s#%d: process_wait(%d) ENTERED\n",
        cur->name, cur->tid, child_id);
  
  /* Yes! You need to do something good here ! */
  /* is this really our child ? */
  if( p->content[child_id] == NULL ) /* trying to fix wait twice ...*/
    return status;
  if( plist_is_child(p, child_id, pid) )
    {
      sema_down(&p->content[child_id]->exit_status_available);
      status = plist_get_status(p,child_id);
      plist_remove(p, child_id);
    }
  

  debug("%s#%d: process_wait(%d) RETURNS %d\n",
        cur->name, cur->tid, child_id, status);
 
  return status;
}

/* Free the current process's resources. This function is called
   automatically from thread_exit() to make sure cleanup of any
   process resources is always done. That is correct behaviour. But
   know that thread_exit() is called at many places inside the kernel,
   mostly in case of some unrecoverable error in a thread.

   In such case it may happen that some data is not yet available, or
   initialized. You must make sure that nay data needed IS available
   or initialized to something sane, or else that any such situation
   is detected.
*/
  
void
process_cleanup (void)
{
  struct thread*  cur = thread_current ();
  struct plist*   p   = &PROCESS_LIST;
  uint32_t*       pd  = cur->pagedir;
  int             pid = cur->pid;
  int             status = plist_get_status(p, pid);
  
  
  debug("%s#%d: process_cleanup() ENTERED\n", cur->name, cur->tid);
  
  /* Later tests DEPEND on this output to work correct. You will have
   * to find the actual exit status in your process list. It is
   * important to do this printf BEFORE you tell the parent process
   * that you exit.  (Since the parent may be the main() function,
   * that may sometimes poweroff as soon as process_wait() returns,
   * possibly before the prontf is completed.)
   */

  /* Don't move this ... wtf */
  printf("%s: exit(%d)\n", thread_name(), status);

  /* Clear the file list */
  map_clear(&(cur->file_list));

  if ( pid != -1 ) /* How to deal with threads that are not processes! */
    {
      /* Let parent process know we're done */  
      plist_for_each(p,&update_parent, pid);
      sema_up(&p->content[pid]->exit_status_available);
      plist_remove_if(p,&is_candidate, pid);
      
    }
  //printf("%s: pid: %d\n", thread_name(), pid);

  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */
  if (pd != NULL) 
    {
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
    }
  //  process_print_list();
  debug("%s#%d: process_cleanup() DONE with status %d\n",
        cur->name, cur->tid, status);
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}

/*
 * Code used when setting up the process stack
 */

struct main_args
{
  void (*ret)(void);
  int argc;
  char** argv;
};

/* Return true if 'c' is fount in the c-string 'd'
 * NOTE: 'd' must be a '\0'-terminated c-string
 */
bool exists_in(char c, const char* d)
{
  int i = 0;
  while (d[i] != '\0' && d[i] != c)
    ++i;
  return (d[i] == c);
}

/* Return the number of words in 'buf'. A word is defined as a
 * sequence of characters not containing any of the characters in
 * 'delimeters'.
 * NOTE: arguments must be '\0'-terminated c-strings
 */
int count_args(const char* buf, const char* delimeters)
{
  int i = 0;
  bool prev_was_delim;
  bool cur_is_delim = true;
  int argc = 0;

  while (buf[i] != '\0')
  {
    prev_was_delim = cur_is_delim;
    cur_is_delim = exists_in(buf[i], delimeters);
    argc += (prev_was_delim && !cur_is_delim);
    ++i;
  }
  return argc;
}

/* Replace calls to STACK_DEBUG with calls to printf. All such calls
 * easily removed later by replacing with nothing. */
#define STACK_DEBUG(...)// printf(__VA_ARGS__)

void* setup_main_stack(const char* command_line, void* stack_top)
{
  /* Variable "esp" stores an address, and at the memory loaction
   * pointed out by that address a "struct main_args" is found.
   * That is: "esp" is a pointer to "struct main_args" */
  struct main_args* esp;
  int argc = 0;
  int total_size;
  int line_size;
  /* "cmd_line_on_stack" and "ptr_save" are variables that each store
   * one address, and at that address (the first) char (of a possible
   * sequence) can be found. */
  char* cmd_line_on_stack;
  char* ptr_save;
  
  /* calculate the bytes needed to store the command_line */
  line_size = strlen(command_line)+1;
  STACK_DEBUG("# line_size = %d\n", line_size);

  /* round up to make it even divisible by 4 */
  if( line_size%4 )
    line_size += 4 - line_size%4;
  STACK_DEBUG("# line_size (aligned) = %d\n", line_size);

  /* calculate how many words the command_line contain */
  argc = count_args(command_line, " ");
  STACK_DEBUG("# argc = %d\n", argc);

  /* calculate the size needed on our simulated stack */
  total_size = line_size + 4*(argc + 4);
  STACK_DEBUG("# total_size = %d\n", total_size);
  
  /* calculate where the final stack top will be located */
  esp = (struct main_args *)((int)stack_top - total_size);

  /* setup return address and argument count */
  esp->ret = 0x00; // Or NULL?
  esp->argc = argc;

  /* calculate where in the memory the argv array starts */
  esp->argv = (char**)((int)esp + 12);

  /* calculate where in the memory the words is stored */
  cmd_line_on_stack = (char*)((int)esp->argv + (argc+1)*4 );

  /* copy the command_line to where it should be in the stack */
  /* build argv array and insert null-characters after each word */
  
  char* token;
  char* cmd_copy = (char*)command_line;
  int i = 0;

  for (token = strtok_r (cmd_copy, " ", &ptr_save); token != NULL;
       token = strtok_r (NULL, " ", &ptr_save))
    {
      strlcpy(cmd_line_on_stack, token, strlen(token)+1);
      
      esp->argv[i] = cmd_line_on_stack;

      cmd_line_on_stack += strlen(token);
      strlcpy(cmd_line_on_stack,"\0",1);
      ++cmd_line_on_stack;
      ++i;
    }
  
  esp->argv[argc] = NULL;
  return esp; /* the new stack top */
}



