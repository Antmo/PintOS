#include <stddef.h>
#include "flist.h"
#include <stdlib.h>
#include "threads/malloc.h"
#include <string.h>

void 
map_init(struct map* m)
{
  unsigned i;
  for(i = 0; i < MAP_SIZE; ++i)
    {
      m->content[i] = NULL;
    }
  return;
}

int
map_insert(struct map* m, value_t t)
{
  unsigned i;
  for(i = RESERVED; i < MAP_SIZE; ++i)
    {
      if(m->content[i] == NULL)
	{
	  m->content[i] = t;
	  return i;
	}
    }

  return -1;
}

value_t 
map_find(struct map* m, int i)
{
  if(i >= 0 && i < MAP_SIZE)
    return m->content[i];
  else
    return NULL;
}

value_t
map_remove(struct map* m, int i)
{
  if(i >= 0 && i < MAP_SIZE && m->content[i] != NULL)
    {
      value_t temp = m->content[i];
      
      m->content[i] = NULL;
      file_close(m->content[i]);

      return temp;
    }
  else
      return NULL;
}

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
