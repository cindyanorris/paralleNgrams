#include "usage.h"
#include "parseArgument.h"
#include "hashTable.h"
#include "parseBook.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

typedef struct
{
   float logProbability;
   float priorProbability;
   char * author;
   int classID;
   int count;  //number of texts in training data
} NaiveBayesT;

static int getMax(NaiveBayesT * naivebayes, int numAuthors);
static NaiveBayesT * createNaiveBayesArray(arrayT * authorArray, int maxClass);
static void performTraining(hashTableT * hashtable, NaiveBayesT * naivebayes, int numAuthors,
                     arrayT * trainingDataArray, char * path, char * ngramType,
                     int ngramSize, char * filePortion, int idxStart);
static int performTesting(hashTableT * hashtable, NaiveBayesT * naivebayes, int numAuthors, 
                   arrayT * testingDataArray, char * path, char * ngramType,
                   int ngramSize, char * filePortion, int idxStart);
static int largestClassID(arrayT * authorArray);
static void updateNaiveBayesCount(NaiveBayesT * naivebayes, int numAuthors, int class);

int main(int argc, char * argv[])
{ 
   int i, j;

   if (argc < 2 || access(argv[1], R_OK )) usage();
   objectT * jsonObj = buildJsonObject(argv[1]);

   //printJsonObject(jsonObj);
   objectT * inputObj = getMemberValueFromObject(jsonObj, "input", OBJECT);
   arrayT * authorArray = getMemberValueFromObject(inputObj, "authors", ARRAY);
   arrayT * trainingDataArray = getMemberValueFromObject(inputObj, "training data", ARRAY);
   arrayT * testingDataArray = getMemberValueFromObject(inputObj, "testing data", ARRAY);
   char * path = getMemberValueFromObject(inputObj, "path", STRING);
   char * ngramType = getMemberValueFromObject(inputObj, "ngram type", STRING); //byte or word
   int ngramSize = atoi(getMemberValueFromObject(inputObj, "ngram size", STRING)); 
   int trainingIdxStart = atoi(getMemberValueFromObject(inputObj, "training start index", STRING)); 
   int testingIdxStart = atoi(getMemberValueFromObject(inputObj, "testing start index", STRING)); 
   char * trainingfilePortion = getMemberValueFromObject(inputObj, "training portion", STRING);
   char * testingfilePortion = getMemberValueFromObject(inputObj, "testing portion", STRING);

   int maxClassID =  largestClassID(authorArray); 
   int numAuthors =  authorArray->size; 
   int numTrainingTxts = trainingDataArray->size; 
   int numTestingTxts = testingDataArray->size; 
   hashTableT * hashtable = createHashTable(maxClassID);

   NaiveBayesT * naivebayes = createNaiveBayesArray(authorArray, maxClassID + 1);

   //perform the training
   performTraining(hashtable, naivebayes, numAuthors, trainingDataArray,
                   path, ngramType, ngramSize, trainingfilePortion, trainingIdxStart);

   //calculate the prior probability
   for (j = 0; j < numAuthors; j++)
   {
      naivebayes[j].priorProbability = naivebayes[j].count / (float) numTrainingTxts;
      //printf("class = %d, probability = %f\n", naivebayes[j].classID,
      //        naivebayes[j].priorProbability);

   }

   //calculate the likelihood for word for each author (class)
   calculateLikelihoods(hashtable);
   //printHashTable(hashtable);

   printf("Training: %d texts, %s bytes/text, %s ngrams of size %d\n", 
          numTrainingTxts, trainingfilePortion, ngramType, ngramSize);
   printf("Testing: %d tests, %s bytes/test\n\n", numTestingTxts, testingfilePortion);
   //perform the testing
   int totalCorrect = performTesting(hashtable, naivebayes, numAuthors, testingDataArray, 
                                     path, ngramType, ngramSize, testingfilePortion, testingIdxStart);

   printf("\n%d out of %d correct: %.2f%%\n", totalCorrect, numTestingTxts,
          totalCorrect/(float)numTestingTxts * 100);
   return 0;
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

int getMax(NaiveBayesT * naivebayes, int numAuthors)
{
   int idx = -1, i;
   float max = 0;
   for (i = 0; i < numAuthors; i++)
   {
      if (idx == -1 || naivebayes[i].logProbability > max)
      {
         idx = i;
         max = naivebayes[i].logProbability;
      }
   }
   return idx;
}

NaiveBayesT * createNaiveBayesArray(arrayT * authorArray, int maxClassIDPlus1)
{
   int j;
   int numAuthors = authorArray->size;
   NaiveBayesT * naivebayes = (NaiveBayesT *) malloc(sizeof(NaiveBayesT) * maxClassIDPlus1);
   memset(naivebayes, 0, sizeof(NaiveBayesT) * maxClassIDPlus1);

   for (j = 0; j < numAuthors; j++)
   {
      valueT * authorV = getElementFromArray(authorArray, j, OBJECT);
      objectT * authorO = getValueFromValueT(authorV, OBJECT);
      char * author = getMemberValueFromObject(authorO, "name", STRING);
      int class  = atoi(getMemberValueFromObject(authorO, "class", STRING));
      naivebayes[j].author = strdup(author);
      naivebayes[j].classID = class;
      naivebayes[j].count = 0;
   }
   return naivebayes;
}

void updateNaiveBayesCount(NaiveBayesT * naivebayes, int numAuthors, int class)
{
   int j;
   for (j = 0; j < numAuthors; j++) 
   {
       if (naivebayes[j].classID == class)
       {
          naivebayes[j].count++;
          return;
       }
   }
   fprintf(stderr, "Unable to find class: %d\n", class);
   usage();
}
   

void performTraining(hashTableT * hashtable, NaiveBayesT * naivebayes, int numAuthors, 
                     arrayT * trainingDataArray, char * path, char * ngramType,
                     int ngramSize, char * filePortion, int idxStart)
{
   int i, j;
   int numTrainingTxts = trainingDataArray->size;

   for (i = 0; i < numTrainingTxts; i++)
   {
      valueT * authorV = getElementFromArray(trainingDataArray, i, OBJECT);
      objectT * authorO = getValueFromValueT(authorV, OBJECT);
      char * title = getMemberValueFromObject(authorO, "file", STRING);
      int class  = atoi(getMemberValueFromObject(authorO, "class", STRING));
      updateNaiveBayesCount(naivebayes, numAuthors, class);
      bookT * book = createBook(path, title, class, idxStart, filePortion);
      readContents(book);
      normalize(book);
      char * ngram;
      do
      {
         ngram = getNextNgram(book, ngramType, ngramSize);
         if (ngram) insertNgramInHashTable(hashtable, ngram, class);
      } while (ngram != NULL);
      freeBook(book);
   }
}  

int performTesting(hashTableT * hashtable, NaiveBayesT * naivebayes, int numAuthors, 
                   arrayT * testingDataArray, char * path, char * ngramType,
                   int ngramSize, char * filePortion, int idxStart)
{
   int i;
   int j;
   int totalCorrect = 0;
   int numTestingTxts = testingDataArray->size;

   for (i = 0; i < numTestingTxts; i++)
   {
      for (j = 0; j < numAuthors; j++)
      {
         naivebayes[j].logProbability = log(naivebayes[j].priorProbability);
/*
         printf("j: %d, class: %d, prob: %f, author: %s, count: %d\n",
                 j,
                 naivebayes[j].classID,
                 naivebayes[j].logProbability,
                 naivebayes[j].author,
                 naivebayes[j].count);
*/
      }

      valueT * testV = getElementFromArray(testingDataArray, i, OBJECT);
      objectT * testO = getValueFromValueT(testV, OBJECT);
      char * title = getMemberValueFromObject(testO, "file", STRING);
      int class  = atoi(getMemberValueFromObject(testO, "class", STRING));

      bookT * book = createBook(path, title, class, idxStart, filePortion);
      readContents(book);
      normalize(book);

      char * ngram;
      do
      {
         ngram = getNextNgram(book, ngramType, ngramSize);
         if (ngram)
         {
            for (j = 0; j < numAuthors; j++)
            {
               int classID = naivebayes[j].classID;
               float likelihood = getLikelihood(hashtable, ngram, classID);
               naivebayes[j].logProbability += log(likelihood);
            }
         }

      } while (ngram != NULL);
      freeBook(book);
      int idx = getMax(naivebayes, numAuthors);
      if (naivebayes[idx].classID == class) 
      {
         totalCorrect += 1;
         printf("%s classified as class: %d, author: %s\n", title,
                naivebayes[idx].classID, naivebayes[idx].author);
      } else
      {
         printf("***%s classified as class: %d, author: %s***\n", title,
                naivebayes[idx].classID, naivebayes[idx].author);
      }
   }
   return totalCorrect;
}
