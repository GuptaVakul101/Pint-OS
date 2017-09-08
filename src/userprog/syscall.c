#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

static void
halt (struct intr_frame *f)
{
  printf ("system call!\n");
  thread_exit ();
}

static void
exit (struct intr_frame *f)
{
  void *esp = f->esp;
  esp += sizeof (int);

  int status = *((int *)esp);
  esp += sizeof (int);
  
  char *name = thread_current ()->name, *save;
  name = strtok_r (name, " ", &save);
  
  printf ("%s: exit(%d)\n", name, status);
  thread_exit ();
}

static void
exec (struct intr_frame *f)
{
  printf ("system call!\n");
  thread_exit ();
}

static void
wait (struct intr_frame *f)
{
  printf ("system call!\n");
  thread_exit ();
}

static void
create (struct intr_frame *f)
{
  printf ("system call!\n");
  thread_exit ();
}

static void
remove (struct intr_frame *f)
{
  printf ("system call!\n");
  thread_exit ();
}

static void
open (struct intr_frame *f)
{
  printf ("system call!\n");
  thread_exit ();
}

static void
filesize (struct intr_frame *f)
{
  printf ("system call!\n");
  thread_exit ();
}

static void
read (struct intr_frame *f)
{
  printf ("system call!\n");
  thread_exit ();
}

static void
write (struct intr_frame *f)
{
  void *esp = f->esp;
  esp += sizeof(int);

  int fd = *((int *)esp);
  esp += sizeof (int);

  const void *buffer = *((void **)esp);
  esp += sizeof (void *);

  unsigned size = *((unsigned *)esp);
  esp += sizeof (unsigned);
  
  if (fd == STDOUT_FILENO)
  {
    int i;
    for (i = 0; i<size; i++)
    {
      putchar (*((char *) buffer + i));
    }
  }
}

static void
seek (struct intr_frame *f)
{
  printf ("system call!\n");
  thread_exit ();
}

static void
tell (struct intr_frame *f)
{
  printf ("system call!\n");
  thread_exit ();
}

static void
close (struct intr_frame *f)
{
  printf ("system call!\n");
  thread_exit ();
}

static void
mmap (struct intr_frame *f)
{
  printf ("system call!\n");
  thread_exit ();
}

static void
munmap (struct intr_frame *f)
{
  printf ("system call!\n");
  thread_exit ();
}

static void
chdir (struct intr_frame *f)
{
  printf ("system call!\n");
  thread_exit ();
}

static void
mkdir (struct intr_frame *f)
{
  printf ("system call!\n");
  thread_exit ();
}

static void
readdir (struct intr_frame *f)
{
  printf ("system call!\n");
  thread_exit ();
}

static void
isdir (struct intr_frame *f)
{
  printf ("system call!\n");
  thread_exit ();
}

static void
inumber (struct intr_frame *f)
{
  printf ("system call!\n");
  thread_exit ();
}

static void (*syscalls []) (struct intr_frame *) =
  {
    halt,
    exit,
    exec,
    wait,
    create,
    remove,
    open,
    filesize,
    read,
    write,
    seek,
    tell,
    close,

    mmap,
    munmap,

    chdir,
    mkdir,
    readdir,
    isdir,
    inumber
  };

const int num_calls = sizeof (syscalls) / sizeof (syscalls[0]);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall"); 
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{  
  int syscall_num = *((int *) f->esp);

  if (syscall_num >= 0 && syscall_num < num_calls)
  {
    void (*function) (struct intr_frame *) = syscalls[syscall_num];
    function (f);
  }
  else
  {
    /* TODO:: Raise Exception */
    printf ("\nError, invalid syscall number.");
    thread_exit ();
  }
}
