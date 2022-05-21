#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"

#define NUM_ITERATIONS 1000

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif

int resource_combination;
uthread_cond_t mp;
uthread_cond_t mt;
uthread_cond_t pt;

struct Agent {
  uthread_mutex_t mutex;
  uthread_cond_t  match;
  uthread_cond_t  paper;
  uthread_cond_t  tobacco;
  uthread_cond_t  smoke;
};

struct Agent* createAgent() {
  struct Agent* agent = malloc (sizeof (struct Agent));
  agent->mutex   = uthread_mutex_create();
  agent->paper   = uthread_cond_create (agent->mutex);
  agent->match   = uthread_cond_create (agent->mutex);
  agent->tobacco = uthread_cond_create (agent->mutex);
  agent->smoke   = uthread_cond_create (agent->mutex);
  return agent;
}

//
// TODO
// You will probably need to add some procedures and struct etc.
//

/**
 * You might find these declarations helpful.
 *   Note that Resource enum had values 1, 2 and 4 so you can combine resources;
 *   e.g., having a MATCH and PAPER is the value MATCH | PAPER == 1 | 2 == 3
 */
enum Resource            {    MATCH = 1, PAPER = 2,   TOBACCO = 4};
char* resource_name [] = {"", "match",   "paper", "", "tobacco"};

int signal_count [5];  // # of times resource signalled
int smoke_count  [5];  // # of times smoker with resource smoked

/**
 * This is the agent procedure.  It is complete and you shouldn't change it in
 * any material way.  You can re-write it if you like, but be sure that all it does
 * is choose 2 random resources, signal their condition variables, and then wait
 * wait for a smoker to smoke.
 */
void* agent (void* av) {
  struct Agent* a = av;
  static const int choices[]         = {MATCH|PAPER, MATCH|TOBACCO, PAPER|TOBACCO};
  static const int matching_smoker[] = {TOBACCO,     PAPER,         MATCH};
  
  uthread_mutex_lock (a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {
      int r = random() % 3;
      signal_count [matching_smoker [r]] ++;
      int c = choices [r];
      if (c & MATCH) {
        VERBOSE_PRINT ("match available\n");
        uthread_cond_signal (a->match);
      }
      if (c & PAPER) {
        VERBOSE_PRINT ("paper available\n");
        uthread_cond_signal (a->paper);
      }
      if (c & TOBACCO) {
        VERBOSE_PRINT ("tobacco available\n");
        uthread_cond_signal (a->tobacco);
      }
      VERBOSE_PRINT ("agent is waiting for smoker to smoke\n");
      uthread_cond_wait (a->smoke);
    }
  uthread_mutex_unlock (a->mutex);
  return NULL;
}

void signal_correct_smoker() {
    if(resource_combination == MATCH + PAPER) {
        uthread_cond_signal(mp);
    } else if(resource_combination == MATCH + TOBACCO) {
        uthread_cond_signal(mt);
    } else if(resource_combination == PAPER + TOBACCO) {
        uthread_cond_signal(pt);
    }
}

void* add_match(void* av) {
    struct Agent* a = av;
    uthread_mutex_lock(a->mutex);
    while(1) {
        uthread_cond_wait(a->match);
        resource_combination += MATCH;
        signal_correct_smoker();
    }
    uthread_mutex_unlock(a->mutex);
}

void* add_paper(void* av) {
    struct Agent* a = av;
    uthread_mutex_lock(a->mutex);
    while(1) {
        uthread_cond_wait(a->paper);
        resource_combination += PAPER;
        signal_correct_smoker();
    }
    uthread_mutex_unlock(a->mutex);
}

void* add_tobacco(void* av) {
    struct Agent* a = av;
    uthread_mutex_lock(a->mutex);
    while(1) {
        uthread_cond_wait(a->tobacco);
        resource_combination += TOBACCO;
        signal_correct_smoker();
    }
    uthread_mutex_unlock(a->mutex);
}

void* match(void* av) {
    struct Agent* a = av;
    uthread_mutex_lock(a->mutex);
    while(1) {
        uthread_cond_wait(pt);
        resource_combination = 0;
        smoke_count[MATCH]++;
        uthread_cond_signal(a->smoke);
    }
    uthread_mutex_unlock(a->mutex);
}

void* paper(void* av) {
    struct Agent* a = av;
    uthread_mutex_lock(a->mutex);
    while(1) {
        uthread_cond_wait(mt);
        resource_combination = 0;
        smoke_count[PAPER]++;
        uthread_cond_signal(a->smoke);
    }
    uthread_mutex_unlock(a->mutex);
}

void* tobacco(void* av) {
    struct Agent* a = av;
    uthread_mutex_lock(a->mutex);
    while(1) {
        uthread_cond_wait(mp);
        resource_combination = 0;
        smoke_count[TOBACCO]++;
        uthread_cond_signal(a->smoke);
    }
    uthread_mutex_unlock(a->mutex);
}

int main (int argc, char** argv) {
  uthread_init (7);
  struct Agent*  a = createAgent();
  // TODO
  resource_combination = 0;
  mp = uthread_cond_create(a->mutex);
  mt = uthread_cond_create(a->mutex);
  pt = uthread_cond_create(a->mutex);
  uthread_create(add_match,a);
  uthread_create(add_paper,a);
  uthread_create(add_tobacco,a);
  uthread_create(match,a);
  uthread_create(paper,a);
  uthread_create(tobacco,a);
  uthread_join (uthread_create (agent, a), 0);
  assert (signal_count [MATCH]   == smoke_count [MATCH]);
  assert (signal_count [PAPER]   == smoke_count [PAPER]);
  assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);
  assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);
  printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
          smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);
}