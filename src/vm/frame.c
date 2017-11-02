#include "threads/synch.h"
#include "threads/palloc.h"
#include "vm/frame.h"
#include "vm/page.h"
#include <malloc.h>
#include <list.h>
#include "userprog/pagedir.h"
#include <bitmap.h>
#include "vm/swap.h"

struct list frame_table;
struct lock frame_table_lock;

static void clear_frame_entry (struct frame_table_entry *);
static void *frame_alloc (enum palloc_flags);
static bool evict_frame (struct frame_table_entry *);

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
  /* 4 classes (low priority) 00 01 10 11 (high priority) 
     (Reference (higher priority) and Dirty bits).
     We choose a frame in the order of priority given above 
     and use FIFO order to resolve conflicts.
     
     In phase 1 we write all dirty pages except that 
     of the code type.
     
     In phase 2 we assume all pages are non dirty (those of 
     code might be dirty but if they are not accessed they 
     will be victim (since phase 2 only occurs if all 
     frames were dirty)) (also some FILE or MMAP might not 
     have been written properly and write_to_disk would 
     have returned false, these are also dirty in phase 2).
     
     If even after phase 2, none of the frames have been 
     evicted that means all were dirty and accessed,
     just evict any (chose first based on FIFO). */
  
  /* Phase 1: Remove Dirty */
  struct list_elem *e;
  for (e = list_begin (&frame_table);
       e != list_end (&frame_table);
       e = list_next (e))
  {
    struct frame_table_entry *fte =
      list_entry (e, struct frame_table_entry, elem);
    bool is_dirty = pagedir_is_dirty (fte->t->pagedir,
                                      fte->frame);
    bool is_accessed = pagedir_is_accessed (fte->t->pagedir,
                                            fte->frame);

    if (fte->spte->type != CODE)
    {
      if (is_dirty)
      {
        if (write_to_disk (fte->spte))
          pagedir_set_dirty (fte->t->pagedir, fte->frame, false);
      }
      else if (!is_accessed)
        return fte;
    }
    else
    {
      if (!is_dirty && !is_accessed)
        return fte;
    }
  }

  /* Phase 2: Remove Accessed */
  for (e = list_begin (&frame_table);
       e != list_end (&frame_table);
       e = list_next (e))
  {
    struct frame_table_entry *fte =
      list_entry (e, struct frame_table_entry, elem);
    bool is_dirty = pagedir_is_dirty (fte->t->pagedir,
                                      fte->frame);
    bool is_accessed = pagedir_is_accessed (fte->t->pagedir,
                                            fte->frame);

    if ((!is_dirty || fte->spte->type == CODE) && !is_accessed)
      return fte;
    else /*Accessed or (Dirty (FILE or MMAP)). */
      pagedir_set_accessed (fte->t->pagedir, fte->frame, false);
  }

  return list_entry (list_front (&frame_table),
                     struct frame_table_entry, elem);
}

static bool
evict_frame (struct frame_table_entry *fte)
{
  struct spt_entry *spte = fte->spte;
  size_t idx;
  switch (spte->type){
  case FILE:
  case MMAP:

    /* Given frame to evict of type FILE or MMAP will 
       never be dirty. */

    if (pagedir_is_dirty (fte->t->pagedir, fte->frame))
        if (!write_to_disk (spte))
          return false;

    spte->frame = NULL;
    clear_frame_entry (fte);
    
    break;
  case CODE:

    idx = swap_out (spte);
    if (idx == BITMAP_ERROR)
      return false;

    spte->idx = idx;
    spte->is_in_swap = true;
    spte->frame = NULL;
    
    clear_frame_entry (fte);

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
    if (list_empty (&frame_table))
      PANIC ("palloc_get_page returned NULL when frame table empty.");

    /* We are assuming palloc_get_page failed as 
       frame table is full. Thus there will be a frame 
       that can be evicted in the table and thus fte will 
       always be non NULL. */
    
    struct frame_table_entry *fte = get_victim_frame ();
    if (evict_frame (fte)){
      frame = palloc_get_page (flags);
      ASSERT (frame != NULL);
      return frame;
    }
    else
      return NULL;
  }
}

void
free_frame (void *frame)
{
  struct frame_table_entry *fte;
  struct list_elem *e;
  for (e = list_begin (&frame_table);
       e != list_end (&frame_table);
       e = list_next (e))
  {
    fte = list_entry (e, struct frame_table_entry, elem);
    if (fte->frame == frame)
    {
      list_remove (&fte->elem);
      break;
    }
  }
  palloc_free_page (frame);
}

static void
clear_frame_entry (struct frame_table_entry *fte)
{
  list_remove (&fte->elem);
  pagedir_clear_page (fte->t->pagedir, fte->frame);
  palloc_free_page (fte->frame);
  free (fte);
}
