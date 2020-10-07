#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

typedef struct
{
   void (*function)(void *);
   void *argument;
} threadpoolTaskT;

typedef struct
{
   pthread_mutex_t lock;
   pthread_cond_t notify;
   threadpoolTaskT * tasks;
   int taskSize;   //size of tasks array
   int taskCnt;    //number of tasks in array
   int taskIdx;    //index of next task
   int shutdown;   //set to 1 when a thread grabs the last task
   pthread_t * threads;
   int threadCnt;
} threadpoolT;

   typedef struct
   {
      int a;
      int b;
   } argumentT;

void task(void * argument)
{
   argumentT * args = (argumentT *) argument;
   printf("%d + %d = %d\n", args->a, args->b, args->a + args->b);
}

void * threadpoolThread(void * poolPtr)
{
   threadpoolT *pool = (threadpoolT *)poolPtr;

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

   return;
}

threadpoolT * threadpoolCreate(int numTasks, int numThreads)
{
   int i;
   threadpoolT * poolPtr = (threadpoolT *) malloc(sizeof(threadpoolT));
   if (poolPtr == NULL) return;
   poolPtr->tasks = (threadpoolTaskT *) malloc(sizeof(threadpoolTaskT) * numTasks);
   if (poolPtr->tasks == NULL) return;
   for (i = 0; i < numTasks; i++)
   {
      poolPtr->tasks[i].function = 0;
      poolPtr->tasks[i].argument = 0;
   }
   poolPtr->taskCnt = 0;
   poolPtr->taskSize = numTasks;
   poolPtr->taskIdx = 0;
   pthread_mutex_init(&(poolPtr->lock), NULL);
   pthread_cond_init(&(poolPtr->notify), NULL);

   poolPtr->threadCnt = 0;
   poolPtr->threads = (pthread_t *) malloc(sizeof(pthread_t) * numThreads);
   for (i = 0; i < numThreads; i++)
   {
      //create a thread to run the threadpoolThread function
      if (pthread_create(&(poolPtr->threads[i]), NULL, threadpoolThread, (void*)poolPtr) != 0) 
      {
          return NULL;
      }
      poolPtr->threadCnt++;
   }
   return poolPtr;
}

void * createArgument(int i, int j)
{
   argumentT * argP = (argumentT *) malloc(sizeof(argumentT));
   argP->a = i;
   argP->b = j;
   return argP;
}

void threadpoolAdd(threadpoolT * threadpool, void * func, void * arg)
{
   pthread_mutex_lock(&(threadpool->lock));
   threadpool->tasks[threadpool->taskCnt].function = func;
   threadpool->tasks[threadpool->taskCnt].argument = arg;
   threadpool->taskCnt++;
   pthread_mutex_unlock(&(threadpool->lock));
   pthread_cond_signal(&(threadpool->notify));
}

void threadpoolDestroy(threadpoolT * threadpool)
{
   int i;

   for (i = 0; i < threadpool->threadCnt; i++)
   {
      pthread_join(threadpool->threads[i], NULL);
   }
   pthread_mutex_destroy(&(threadpool->lock));
   pthread_cond_destroy(&(threadpool->notify));
}

int main()
{
   int numCPUs = sysconf(_SC_NPROCESSORS_ONLN);
   int i;
   printf("Number of logical CPUs is %d\n", numCPUs);
   threadpoolT * threadpool = threadpoolCreate(10, 4);
   for (i = 0; i < 10; i++)
   {
      void * args = createArgument(i, i+1);
      threadpoolAdd(threadpool, task, args);
   }
   threadpoolDestroy(threadpool);
}

