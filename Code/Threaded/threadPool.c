#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "threadPool.h"

void * threadPoolThread(void * poolPtr)
{
   threadPoolT *pool = (threadPoolT *)poolPtr;

   for (;;) 
   {
      /* Lock must be taken to wait on conditional variable */
      pthread_mutex_lock(&(pool->lock));

      /* Wait on condition variable, check for spurious wakeups.
         When returning from pthread_cond_wait(), we own the lock. */
      while ((!pool->shutdown) && (pool->tasks[pool->taskIdx].function == NULL)) 
      {
         pthread_cond_wait(&(pool->notify), &(pool->lock));
      }

      if (pool->shutdown) break;

      void (* function)(void *) = pool->tasks[pool->taskIdx].function;
      void * argument = pool->tasks[pool->taskIdx].argument;
     
      pool->taskIdx++;
      if (pool->taskIdx == pool->taskSize) pool->shutdown = 1;

      /* Unlock */
      pthread_mutex_unlock(&(pool->lock));

      /* Get to work */
      ((*function)(argument));
   }
   pthread_mutex_unlock(&(pool->lock));
   pthread_exit(NULL);

   return 0;
}

threadPoolT * threadPoolCreate(int numTasks, int numThreads)
{
   int i;
   threadPoolT * poolPtr = (threadPoolT *) malloc(sizeof(threadPoolT));
   if (poolPtr == NULL) return 0;
   poolPtr->tasks = (threadPoolTaskT *) malloc(sizeof(threadPoolTaskT) * numTasks);
   if (poolPtr->tasks == NULL) return 0;
   for (i = 0; i < numTasks; i++)
   {
      poolPtr->tasks[i].function = 0;
      poolPtr->tasks[i].argument = 0;
   }
   poolPtr->taskCnt = 0;
   poolPtr->taskSize = numTasks;
   poolPtr->taskIdx = 0;
   poolPtr->shutdown = 0;
   pthread_mutex_init(&(poolPtr->lock), NULL);
   pthread_cond_init(&(poolPtr->notify), NULL);

   poolPtr->threadCnt = 0;
   poolPtr->threads = (pthread_t *) malloc(sizeof(pthread_t) * numThreads);
   for (i = 0; i < numThreads; i++)
   {
      //create a thread to run the threadPoolThread function
      if (pthread_create(&(poolPtr->threads[i]), NULL, threadPoolThread, (void*)poolPtr) != 0) 
      {
          return NULL;
      }
      poolPtr->threadCnt++;
   }
   return poolPtr;
}

void threadPoolAdd(threadPoolT * threadPool, void * func, void * arg)
{
   pthread_mutex_lock(&(threadPool->lock));
   threadPool->tasks[threadPool->taskCnt].function = func;
   threadPool->tasks[threadPool->taskCnt].argument = arg;
   threadPool->taskCnt++;
   pthread_mutex_unlock(&(threadPool->lock));
   pthread_cond_signal(&(threadPool->notify));
}

void threadPoolDestroy(threadPoolT * threadPool)
{
   int i;

   for (i = 0; i < threadPool->threadCnt; i++)
   {
      pthread_join(threadPool->threads[i], NULL);
   }
   pthread_mutex_destroy(&(threadPool->lock));
   pthread_cond_destroy(&(threadPool->notify));
   free(threadPool->threads);
   free(threadPool->tasks);
   free(threadPool);
}

