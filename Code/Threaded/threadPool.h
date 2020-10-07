typedef struct
{
   void (*function)(void *);
   void *argument;
} threadPoolTaskT;

typedef struct
{
   pthread_mutex_t lock;
   pthread_cond_t notify;
   threadPoolTaskT * tasks;
   int taskSize;   //size of tasks array
   int taskCnt;    //number of tasks in array
   int taskIdx;    //index of next task
   int shutdown;   //set to 1 when a thread grabs the last task
   pthread_t * threads;
   int threadCnt;
} threadPoolT;

void * threadPoolThread(void * poolPtr);
threadPoolT * threadPoolCreate(int numTasks, int numThreads);
void threadPoolAdd(threadPoolT * threadPool, void * func, void * arg);
void threadPoolDestroy(threadPoolT * threadPool);

