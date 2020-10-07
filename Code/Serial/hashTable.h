
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
   hashNodeT * bin[BINCOUNT];
   int total;          //size of vocabulary
   int numAuthors;     //number of authors (classes)
   int * authorTotal;  //number of words for each author
} hashTableT;

hashTableT * createHashTable(int numAuthors);
void calculateLikelihoods(hashTableT * hashtable);
float getLikelihood(hashTableT * hashtable, char * ngram, int classId);
void insertNgramInHashTable(hashTableT * hashTable, char * ngram, int classId);
void printHashTable(hashTableT * hashtable);

