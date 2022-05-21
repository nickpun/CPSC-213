//
// This is the solution to CPSC 213 Assignment 9.
// Do not distribute this code or any portion of it to anyone in any way.
// Do not remove this comment.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/errno.h>
#include <assert.h>
#include "queue.h"
#include "disk.h"
#include "uthread.h"

queue_t      pending_read_queue;
unsigned int sum = 0;

void interrupt_service_routine () {
  // TODO
  void* val;
  queue_dequeue(pending_read_queue,&val,NULL,NULL);
  uthread_unblock(val);
}

void* read_block (void* blocknov) {
  // TODO enqueue result, schedule read, and the update (correctly)
  int result;
  int* blockno = (int*)blocknov;
  queue_enqueue(pending_read_queue,uthread_self(),NULL,NULL);
  disk_schedule_read (&result, *blockno);
  uthread_block();
  sum += result;
  return NULL;
}

int main (int argc, char** argv) {

  // Command Line Arguments
  static char* usage = "usage: tRead num_blocks";
  int num_blocks;
  char *endptr;
  if (argc == 2)
    num_blocks = strtol (argv [1], &endptr, 10);
  if (argc != 2 || *endptr != 0) {
    printf ("argument error - %s \n", usage);
    return EXIT_FAILURE;
  }

  // Initialize
  uthread_init (1);
  disk_start (interrupt_service_routine);
  pending_read_queue = queue_create();

  // Sum Blocks
  // TODO
  uthread_t thread[num_blocks];
  int blockarray[num_blocks];

  for (int blockno = 0; blockno < num_blocks; blockno++) {
    blockarray[blockno]= blockno;
    thread[blockno] = uthread_create(read_block, &blockarray[blockno]);
  }
  for (int blockno = 0; blockno < num_blocks; blockno++) {
    uthread_join(thread[blockno],NULL);
  }
  printf ("%d\n", sum);
}

