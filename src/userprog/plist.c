#include <stddef.h>

#include "plist.h"

#include <stddef.h>
#include <stdlib.h>
#include "threads/malloc.h"
#include <string.h>

void 
plist_init(struct plist* m)
{
  unsigned i;
  for(i = 0; i < LIST_SIZE; ++i)
    {
      m->content[i] = NULL;
    }
  return;
}

key_t
plist_insert(struct plist* m, value_p t)
{
  lock_acquire(&m->phatlock); /* LOCK */

  unsigned i;
  
  for(i = 0; i < LIST_SIZE; ++i)
    {
      if(m->content[i] == NULL)
	{
	  //allocate memory for the struct (never cast return of malloc in c I learnt from stackoverflow.com)
	  m->content[i]         = malloc(sizeof(struct pinfos));
	  //need to allocate space for the char * as well should be of size 16 chars I think
	  m->content[i]->name   = malloc(sizeof(char)*16);
	  
	  strlcpy(m->content[i]->name,t->name,strlen(t->name));
	  m->content[i]->parent_id = t->parent_id;
	  m->content[i]->alive = t->alive;
	  m->content[i]->parent_alive = t->parent_alive;
	  m->content[i]->exit_status = t->exit_status;
	  
	  m->content[i]->exit_status_available = t->exit_status_available;
	  sema_init(&m->content[i]->exit_status_available, 0 );
	  lock_release(&m->phatlock); /* LOCK */
	  return i;
	}
    }
  lock_release(&m->phatlock); /* LOCK */
  return -1;
}

value_p 
plist_find(struct plist* m, int i)
{
  lock_acquire(&m->phatlock); /* LOCK */

  if(i >= 0 && i < LIST_SIZE)
    {
      lock_release(&m->phatlock); /* LOCK */
      return m->content[i];
    }

  lock_release(&m->phatlock); /* LOCK */
  return NULL;
}

value_p
plist_remove(struct plist* m, int i)
{
  lock_acquire(&m->phatlock); /* LOCK */
  
  if(i >= 0 && i < LIST_SIZE && m->content[i] != NULL)
    {
      value_p temp = m->content[i];
      free(m->content[i]->name);
      free(m->content[i]);

      m->content[i] = NULL;

      lock_release(&m->phatlock); /* LOCK */
      return temp;
    }

  lock_release(&m->phatlock); /* LOCK */
  return NULL;
}

bool
plist_alive(struct plist* m, int i)
{
  lock_acquire(&m->phatlock); /* LOCK */

  if(m->content[i] != NULL)
    {
      lock_release(&m->phatlock); /* LOCK */
      return m->content[i]->alive;
    }
  
  lock_release(&m->phatlock); /* LOCK */
  return NULL;
}

int
plist_get_status(struct plist* m, int i)
{
  lock_acquire(&m->phatlock); /* LOCK */
  int status = m->content[i]->exit_status;
  lock_release(&m->phatlock);
  return status;
}

/*
void
map_for_each(struct map* m, void (*exec)(int,value_t,int), int aux)
{
  unsigned i;
  for(i = 0; i < MAP_SIZE; ++i)
    {
      if(m->content[i] != NULL)
	exec(i, m->content[i], aux);
    }
}

void
map_remove_if(struct map* m, bool (*cond)(int,value_t,int), int aux)
{
  unsigned i;
  for(i = 0; i < MAP_SIZE; ++i)
    { 
      if ( cond(i, m->content[i], aux) )
	map_remove(m,i);
    }
}

void
map_clear(struct map* m)
{
  unsigned i;
  for ( i = 0; i < MAP_SIZE; ++i )
    map_remove(m,i);
}
*/
