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
	  m->content[i]->garbage = t->garbage;

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
  int status;
  if(m->content[i] != NULL)
    status = m->content[i]->exit_status;
  else
    status = -1;
  lock_release(&m->phatlock);
  return status;
}

bool
plist_is_child(struct plist* m, int child, int parent)
{
  lock_acquire(&m->phatlock);
  bool result;
  if ( m->content[child] == NULL )
    result = false;
  else if( m->content[child]->parent_id == parent && m->content[child]->parent_alive )
    result = true;
  else
    result = false;

  lock_release(&m->phatlock);

  return result;
}

void
plist_for_each(struct plist* m, void (*exec)(int,value_p,int), int aux)
{
  unsigned i;
  for(i = 0; i < LIST_SIZE; ++i)
    {
      lock_acquire(&m->phatlock);
      if(m->content[i] != NULL)
	exec(i, m->content[i], aux);
      lock_release(&m->phatlock);
    }
}


void
plist_remove_if(struct plist* m, bool (*cond)(int,value_p,int), int aux)
{
 
  unsigned i;
  for(i = 0; i < LIST_SIZE; ++i)
    {
      lock_acquire(&m->phatlock); 
      if ( m->content[i] != NULL && cond(i, m->content[i], aux) )
	{
	  lock_release(&m->phatlock);
	  plist_remove(m,i);
	  continue;
	}
      lock_release(&m->phatlock);
    }
}

bool
is_candidate(int i, value_p p, int aux)
{
  if( !(p->parent_alive || p->alive) )
    return true;

  return false;
}

/*
 * Danger! This function is supposed to be used ONLY if 
 * parent (aux) is dead AND parent (aux) is still in the list.
 * If that is not the case aux could potentially be a new process
 * with own children not at all related to the original aux
 */
void
update_parent(int i, value_p p, int aux)
{
  if( p->parent_id == aux )
    p->parent_alive = false;
  
  if( !(p->alive && p->parent_alive) )
    p->garbage = true;

  if( i == aux )
    p->alive = false;
}

void
print_list(struct plist* p)
{
  lock_acquire(&p->phatlock);

  int i;

  printf("# TOP\n");
  printf("PID\tPARENTID\tALIVE\tPARENT_ALIVE\tEXIT_STATUS\tNAME\n");
    for(i = 0; i < LIST_SIZE; ++i)
      {
	if(p->content[i] == NULL)
	  continue;
	
	printf("%d\t%d\t\t%d\t%d\t\t%d\t\t%s\n",
	       i, 
	       p->content[i]->parent_id, 
	       p->content[i]->alive,
	       p->content[i]->parent_alive,
	       p->content[i]->exit_status,
	       p->content[i]->name );
      }
    lock_release(&p->phatlock);
}

void
set_exit_status(struct plist* p, int status, int pid)
{
  lock_acquire(&p->phatlock);
  p->content[pid]->exit_status = status;
  lock_release(&p->phatlock);
}

/*
void
map_clear(struct map* m)
{
  unsigned i;
  for ( i = 0; i < MAP_SIZE; ++i )
    map_remove(m,i);
}
*/
