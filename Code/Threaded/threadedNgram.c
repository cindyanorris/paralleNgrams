#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "usage.h"
#include "parseArgument.h"
#include "hashTable.h"
#include "handleBook.h"
#include "threadPool.h"

typedef struct
{
   float probability;    //used to hold prior and final probabilities
   char * author;        //name of author
   int classID;          //number of class (unique per author)
   int count;            //number of texts in training data
} classDataT;

typedef struct
{
   int classID;          //class id of test
   char * title;         //title of test
   classDataT * final;   //one per author
} testDataT;

typedef struct
{
   int numAuthors;        //number of classes/authors
   classDataT * prior;    //one per class/author
   int numTestingTxts;    //number of tests
   testDataT * testData;  //one per test
} naiveBayesT;

static int getMax(classDataT * finalProbabilities, int numAuthors);
static naiveBayesT * createNaiveBayesData(objectT * inputObj);
static void performTraining(hashTableT * hashtable, naiveBayesT * naiveBayes, objectT * inputObj);
static void performTesting(hashTableT * hashtable, naiveBayesT * naivebayes, objectT * inputObj);
static int largestClassID(arrayT * authorArray);
static void updatePriorProbabilityCount(naiveBayesT * naivebayes, int class);
static void printParameters(objectT * inputObj);
static void calculatePriorProbability(naiveBayesT * naivebayes, int numTrainingTxts);
static int checkCorrect(naiveBayesT * naivebayes);

int main(int argc, char * argv[])
{ 
   int i, j;

   struct timespec start, finish;
   double elapsed;
   clock_gettime(CLOCK_MONOTONIC, &start);

   if (argc < 2 || access(argv[1], R_OK )) usage();
   objectT * jsonObj = buildJsonObject(argv[1]);
   //printJsonObject(jsonObj);
   
   objectT * inputObj = getMemberValueFromObject(jsonObj, "input", OBJECT);
   arrayT * authorArray = getMemberValueFromObject(inputObj, "authors", ARRAY);
   arrayT * trainingDataArray = getMemberValueFromObject(inputObj, "training data", ARRAY);
   arrayT * testingDataArray = getMemberValueFromObject(inputObj, "testing data", ARRAY);

   printParameters(inputObj);

   int maxClassID =  largestClassID(authorArray); 
   int numAuthors =  authorArray->size; 
   int numTrainingTxts = trainingDataArray->size; 
   int numTestingTxts = testingDataArray->size; 

   hashTableT * hashtable = createHashTable(maxClassID);
   naiveBayesT * naivebayes = createNaiveBayesData(inputObj);

   //perform the training
   performTraining(hashtable, naivebayes, inputObj);

   //training incremented the counts for each author
   //now calculate the prior probability
   calculatePriorProbability(naivebayes, numTrainingTxts);

   //calculate the likelihood for word for each author (class)
   calculateLikelihoods(hashtable);
   //printHashTable(hashtable);

   //perform the testing
   performTesting(hashtable, naivebayes, inputObj); 

   //see how many are correct
   int totalCorrect = checkCorrect(naivebayes);
   printf("\n%d out of %d correct: %.2f%%\n", totalCorrect, numTestingTxts,
          totalCorrect/(float)numTestingTxts * 100);

   clock_gettime(CLOCK_MONOTONIC, &finish);
   elapsed = (finish.tv_sec - start.tv_sec);
   elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
   printf("threadedNgram took %f seconds to execute \n", elapsed);

   return 0;
}

void calculatePriorProbability(naiveBayesT * naivebayes, int numTrainingTxts)
{
   int j;
   int numAuthors = naivebayes->numAuthors;
   for (j = 0; j < numAuthors; j++)
   {
      naivebayes->prior[j].probability = naivebayes->prior[j].count / (float) numTrainingTxts;
      //printf("class = %d, probability = %f\n", naivebayes->prior[j].classID,
      //        naivebayes->prior[j].probability);
   }
}

void printParameters(objectT * inputObj)
{
   arrayT * trainingDataArray = getMemberValueFromObject(inputObj, "training data", ARRAY);
   arrayT * testingDataArray = getMemberValueFromObject(inputObj, "testing data", ARRAY);
   char * ngramType = getMemberValueFromObject(inputObj, "ngram type", STRING); //byte or word
   int ngramSize = atoi(getMemberValueFromObject(inputObj, "ngram size", STRING)); 
   char * trainingfilePortion = getMemberValueFromObject(inputObj, "training portion", STRING);
   char * testingfilePortion = getMemberValueFromObject(inputObj, "testing portion", STRING);

   printf("Training: %d texts, %s bytes/text, %s ngrams of size %d\n", 
          trainingDataArray->size, trainingfilePortion, ngramType, ngramSize);
   printf("Testing: %d tests, %s bytes/text\n\n", testingDataArray->size, testingfilePortion);
}

int largestClassID(arrayT * authorArray)
{
   int i;
   int len = authorArray->size;
   int maxClass = 0;
   for (i = 0; i < len; i++)
   {
      valueT * authorV = getElementFromArray(authorArray, i, OBJECT);
      objectT * authorO = getValueFromValueT(authorV, OBJECT);
      int class  = atoi(getMemberValueFromObject(authorO, "class", STRING));
      if  (maxClass < class) maxClass = class;
   }
   return maxClass;
}

int getMax(classDataT * final, int numAuthors)
{
   int idx = -1, i;
   float max = 0;
   for (i = 0; i < numAuthors; i++)
   {
      if (idx == -1 || final[i].probability > max)
      {
         idx = i;
         max = final[i].probability;
      }
   }
   return idx;
}

static naiveBayesT * createNaiveBayesData(objectT * inputObj)
{
   int i, j;

   arrayT * authorArray = getMemberValueFromObject(inputObj, "authors", ARRAY);
   arrayT * testingDataArray = getMemberValueFromObject(inputObj, "testing data", ARRAY);

   int numAuthors = authorArray->size;
   int numTestingTxts = testingDataArray->size;
   naiveBayesT * naivebayes = (naiveBayesT *) malloc(sizeof(naiveBayesT));
   naivebayes->numAuthors = authorArray->size;

   naivebayes->prior = (classDataT *) malloc(sizeof(classDataT) * numAuthors);
   for (j = 0; j < numAuthors; j++)
   {
      memset(&naivebayes->prior[j], 0, sizeof(classDataT));
      valueT * authorV = getElementFromArray(authorArray, j, OBJECT);
      objectT * authorO = getValueFromValueT(authorV, OBJECT);
      char * author = getMemberValueFromObject(authorO, "name", STRING);
      int class  = atoi(getMemberValueFromObject(authorO, "class", STRING));
      naivebayes->prior[j].author = strdup(author);
      naivebayes->prior[j].classID = class;
   }
   
   naivebayes->testData = (testDataT *) malloc(sizeof(testDataT) * numTestingTxts);
   naivebayes->numTestingTxts = numTestingTxts;

   for (i = 0; i < numTestingTxts; i++)
   {
      naivebayes->testData[i].final = (classDataT *) malloc(sizeof(classDataT) * numAuthors);
      memset(naivebayes->testData[i].final, 0, sizeof(classDataT) * numAuthors);
      for (j = 0; j < numAuthors; j++)
      {
         naivebayes->testData[i].final[j].author = naivebayes->prior[j].author;
         naivebayes->testData[i].final[j].classID = naivebayes->prior[j].classID;
      } 
      valueT * testV = getElementFromArray(testingDataArray, i, OBJECT);
      objectT * testO = getValueFromValueT(testV, OBJECT);
      char * title = getMemberValueFromObject(testO, "file", STRING);
      int classID  = atoi(getMemberValueFromObject(testO, "class", STRING));
      naivebayes->testData[i].classID = classID;
      naivebayes->testData[i].title = title;

   }
   return naivebayes;
}

void updatePriorProbabilityCount(naiveBayesT * naivebayes, int class)
{
   int j;
   for (j = 0; j < naivebayes->numAuthors; j++) 
   {
       if (naivebayes->prior[j].classID == class)
       {
          naivebayes->prior[j].count++;
          return;
       }
   }
   fprintf(stderr, "Unable to find class: %d\n", class);
   usage();
}

#define MAXARG 6
#define BOOKP 0
#define NGRAMTYPE 1
#define NGRAMSIZE 2
#define HASHTABLE 3
#define CLASS 4
#define PROBABILITIES 4
#define NUMAUTHORS 5
typedef struct
{
   void * args[MAXARG];  //array of 6 pointer size arguments
} argumentT;

argumentT * createArgument(void * args[MAXARG])
{
   int i;
   argumentT * argP = (argumentT *) malloc(sizeof(argumentT));
   for (i = 0; i < MAXARG; i++) argP->args[i] = args[i];
   return argP;
}

void threadTrainingTask(void * argument)
{
   argumentT * arg = (argumentT *) argument;
   bookT * book = arg->args[BOOKP];
   char * ngramType = arg->args[NGRAMTYPE];
   int ngramSize = (int) (long) arg->args[NGRAMSIZE];
   hashTableT * hashtable = arg->args[HASHTABLE];
   int class = (int) (long) arg->args[CLASS];

   normalize(book);
   char * ngram;
   do
   {
      ngram = getNextNgram(book, ngramType, ngramSize);
      if (ngram) insertNgramInHashTable(hashtable, ngram, class);
   } while (ngram != NULL);
   freeBook(book);
}

void performTraining(hashTableT * hashtable, naiveBayesT * naivebayes, objectT * inputObj)
{
   int i;
   arrayT * trainingDataArray = getMemberValueFromObject(inputObj, "training data", ARRAY);
   char * path = getMemberValueFromObject(inputObj, "path", STRING);
   char * ngramType = getMemberValueFromObject(inputObj, "ngram type", STRING); //byte or word
   int ngramSize = atoi(getMemberValueFromObject(inputObj, "ngram size", STRING));
   int idxStart = atoi(getMemberValueFromObject(inputObj, "training start index", STRING));
   char * filePortion = getMemberValueFromObject(inputObj, "training portion", STRING);
   int numTrainingTxts = trainingDataArray->size;

   int numCPUs = sysconf(_SC_NPROCESSORS_ONLN);
   if (numCPUs > numTrainingTxts) numCPUs = numTrainingTxts;

   bookT ** books = (bookT **) malloc(sizeof(bookT *) * numTrainingTxts);
   threadPoolT * threadPool = threadPoolCreate(numTrainingTxts, numCPUs); 

   for (i = 0; i < numTrainingTxts; i++)
   {
      valueT * authorV = getElementFromArray(trainingDataArray, i, OBJECT);
      objectT * authorO = getValueFromValueT(authorV, OBJECT);
      char * title = getMemberValueFromObject(authorO, "file", STRING);
      int class  = atoi(getMemberValueFromObject(authorO, "class", STRING));
      updatePriorProbabilityCount(naivebayes, class);
      books[i] = createBook(path, title, class);
      readContents(books[i], idxStart, filePortion);
      void * args[MAXARG] = {(void *) books[i],         (void *) ngramType, 
                             (void *) (long) ngramSize, (void *) hashtable, 
                             (void *) (long) class,     (void *) 0}; 
      void * argument = createArgument(args);
      threadPoolAdd(threadPool, threadTrainingTask, argument);
   }
   threadPoolDestroy(threadPool);
   free(books);
}  

void threadTestingTask(void * argument)
{
   int j;
   argumentT * arg = (argumentT *) argument;
   bookT * book = arg->args[BOOKP];
   char * ngramType = arg->args[NGRAMTYPE];
   int ngramSize = (int) (long) arg->args[NGRAMSIZE];
   hashTableT * hashtable = arg->args[HASHTABLE];
   classDataT * final = arg->args[PROBABILITIES];
   int numAuthors = (int) (long) arg->args[NUMAUTHORS];

   normalize(book);
   char * ngram;
   do
   {
      ngram = getNextNgram(book, ngramType, ngramSize);
      if (ngram)
      {
         for (j = 0; j < numAuthors; j++)
         {
            int classID = final[j].classID;
            float likelihood = getLikelihood(hashtable, ngram, classID);
            final[j].probability += log(likelihood);
         }
      }

   } while (ngram != NULL);
   freeBook(book);
}

void performTesting(hashTableT * hashtable, naiveBayesT * naivebayes, objectT * inputObj)
{
   int i, j, totalCorrect = 0;

   arrayT * testingDataArray = getMemberValueFromObject(inputObj, "testing data", ARRAY);
   char * path = getMemberValueFromObject(inputObj, "path", STRING);
   char * ngramType = getMemberValueFromObject(inputObj, "ngram type", STRING); //byte or word
   int ngramSize = atoi(getMemberValueFromObject(inputObj, "ngram size", STRING));
   int idxStart = atoi(getMemberValueFromObject(inputObj, "testing start index", STRING));
   char * filePortion = getMemberValueFromObject(inputObj, "testing portion", STRING);
   int numTestingTxts = testingDataArray->size;
   bookT ** books = (bookT **) malloc(sizeof(bookT *) * numTestingTxts);

   int numCPUs = sysconf(_SC_NPROCESSORS_ONLN);
   if (numCPUs > numTestingTxts) numCPUs = numTestingTxts;
   threadPoolT * threadPool = threadPoolCreate(numTestingTxts, numCPUs); 

   for (i = 0; i < numTestingTxts; i++)
   {
      for (j = 0; j < naivebayes->numAuthors; j++)
      {
         naivebayes->testData[i].final[j].probability = log(naivebayes->prior[j].probability);
      }
      char * title = naivebayes->testData[i].title;
      int classID = naivebayes->testData[i].classID;

      books[i] = createBook(path, title, classID);
      readContents(books[i], idxStart, filePortion);

      void * args[MAXARG] = {(void *) books[i],         (void *) ngramType, 
                             (void *) (long) ngramSize, (void *) hashtable, 
                             (void *) naivebayes->testData[i].final, (void *) (long) naivebayes->numAuthors};
      void * argument = createArgument(args);
      
      threadPoolAdd(threadPool, threadTestingTask, argument);
   }

   threadPoolDestroy(threadPool);
   free(books);
}

int checkCorrect(naiveBayesT * naivebayes)
{
   int i, totalCorrect = 0;

   for (i = 0; i < naivebayes->numTestingTxts; i++)
   {
      int idx = getMax(naivebayes->testData[i].final, naivebayes->numAuthors);
      if (naivebayes->testData[i].final[idx].classID == naivebayes->testData[i].classID) 
      {
         totalCorrect += 1;
         printf("%s classified as class: %d, author: %s\n", naivebayes->testData[i].title,
                naivebayes->testData[i].final[idx].classID, naivebayes->testData[i].final[idx].author);
      } else
      {
         printf("***%s classified as class: %d, author: %s***\n", naivebayes->testData[i].title,
                naivebayes->testData[i].final[idx].classID, naivebayes->testData[i].final[idx].author);
      }
   }
   return totalCorrect;
}
