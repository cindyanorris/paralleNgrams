
#define BINCOUNT 10000

typedef struct hashNodeT
{
   char * ngram;
   int * counts;
   float * likelihood;
   struct hashNodeT * next;
} hashNodeT;

typedef struct
{
   hashNodeT * hashNodeP;
   pthread_mutex_t nodeLock;
} binT;

typedef struct
{
   binT bin[BINCOUNT];
   //hashNodeT * bin[BINCOUNT];
   //pthread_mutex_t lock[BINCOUNT];
   int total;          //size of vocabulary
   int numAuthors;     //number of authors (classes)
   int * authorTotal;  //number of words for each author
   pthread_mutex_t tableLock;
} hashTableT;

hashTableT * createHashTable(int numAuthors);
void calculateLikelihoods(hashTableT * hashtable);
float getLikelihood(hashTableT * hashtable, char * ngram, int classId);
void insertNgramInHashTable(hashTableT * hashTable, char * ngram, int classId);
void printHashTable(hashTableT * hashtable);

