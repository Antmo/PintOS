#ifndef _PLIST_H_
#define _PLIST_H_


/* Place functions to handle a running process here (process list).
   
   plist.h : Your function declarations and documentation.
   plist.c : Your implementation.

   The following is strongly recommended:

   - A function that given process inforamtion (up to you to create)
     inserts this in a list of running processes and return an integer
     that can be used to find the information later on.

   - A function that given an integer (obtained from above function)
     FIND the process information in the list. Should return some
     failure code if no process matching the integer is in the list.
     Or, optionally, several functions to access any information of a
     particular process that you currently need.

   - A function that given an integer REMOVE the process information
     from the list. Should only remove the information when no process
     or thread need it anymore, but must guarantee it is always
     removed EVENTUALLY.
     
   - A function that print the entire content of the list in a nice,
     clean, readable format.
     
 */
#define LIST_SIZE 128

#include <stdbool.h>
#include "threads/synch.h"

struct pinfos;
typedef struct pinfos* value_p;
typedef int key_t;

struct pinfos {
  bool  garbage; //the fuck is this?
  int   parent_id; // I am the spawn of this fellow
  int   exit_status; // this is how I died
  char* name;
  bool  alive; // I am beyond the realm of the living, or am I ?
  bool  parent_alive; // I am an orphan, or just a melodramatic kid ?
  struct semaphore exit_status_available;
};

struct plist {
  value_p content[LIST_SIZE];
  struct lock phatlock;
};

void     plist_init       (struct plist*);
key_t    plist_insert     (struct plist*, value_p);
value_p  plist_find       (struct plist*, int);
value_p  plist_remove     (struct plist*, int);
bool     plist_alive      (struct plist*, int);
int      plist_get_status (struct plist*, int);


void     plist_for_each  (struct plist*, void (*exec)(int,value_p,int), int);
void     plist_remove_if (struct plist*, bool (*cond)(int,value_p,int), int);
//void     map_clear     (struct map*);

bool is_candidate(int, value_p, int);
void update_parent(int, value_p, int);


#endif
