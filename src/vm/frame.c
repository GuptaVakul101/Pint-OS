#include "threads/synch.h"
#include "threads/palloc.h"
#include "vm/frame.h"
#include "vm/page.h"
#include <malloc.h>

struct list frame_table;
struct lock frame_table_lock;

static void *frame_alloc (enum palloc_flags);

void
frame_table_init (void)
{
  list_init (&frame_table);
  lock_init (&frame_table_lock);
}

/* Unoptimized enhanced second-chance page replacement. */
static struct frame_table_entry *
get_victim_frame ()
{
  //4 classes 00 01 10 11 (Reference and dirty bits)
  return NULL;
}

static void
evict_frame (struct frame_table_entry *fte)
{
  struct spt_entry *spte = fte->spte;
  switch (spte->type){
  case FILE:
  case MMAP:
    //write to file if writable (write_to_disk)
    break;
  case CODE:
    //swap_out
    break;
  default:
    return false;
  }
}

void *
get_frame_for_page (enum palloc_flags flags, struct spt_entry *spte)
{
  if(spte == NULL)
    return NULL;
  
  if (flags & PAL_USER == 0)
    return NULL;

  void *frame = frame_alloc (flags);

  if (frame != NULL){
    add_to_frame_table (frame, spte);
    return frame;
  }
  else return NULL; /*Never Reached. */
}

static void
add_to_frame_table (void *frame, struct spt_entry *spte) {
  struct frame_table_entry *fte =
    (struct frame_table_entry *) malloc (sizeof (struct frame_table_entry));

  lock_acquire (&frame_table_lock);
  fte->frame = frame;
  fte->spte = spte;
  fte->t = thread_current ();
  list_push_back (&frame_table, &fte->elem);
  lock_release (&frame_table_lock);
}

/* Returns kernel virtual address of a kpage taken from user_pool. */
static void *
frame_alloc (enum palloc_flags flags)
{
  if (flags & PAL_USER == 0)
    return NULL;

  void *frame = palloc_get_page (flags);
  if (frame != NULL)
    return frame;
  else
  {
    //Try to get victim, evict it (3 cases (SWAP, MMAP, EXEC FILE)) (change sptes, change pd entries) and alloc
    return NULL;
  }
}

void
free_frame (void *frame)
{
  //Traverse list and find the frame entry corresponding to the frame address and remove it
//  list_remove (&frame->list_elem);
  palloc_free_page (frame);
}
