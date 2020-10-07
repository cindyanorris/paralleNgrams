#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include "parseArgument.h"
#include "hashTable.h"

static unsigned int hashNgram(char * ngram);
static hashNodeT * createHashNode(char * ngram, int numAuthors, hashNodeT * next);
static int insertNgramInBin(char * ngram, binT * bin, int class, int numAuthors);
static hashNodeT * getHashNodeFromBin(char * ngram, binT * binPtr);
static binT * getBin(hashTableT * table, char * ngram);

hashTableT * createHashTable(int numAuthors)
{
   int i;  
   hashTableT * hashTable = (hashTableT *) malloc(sizeof(hashTableT));
   
   for (i = 0; i < BINCOUNT; i++)
   {
      hashTable->bin[i].hashNodeP = NULL;
      pthread_mutex_init(&(hashTable->bin[i].nodeLock), NULL);
   }
   hashTable->numAuthors = numAuthors;
   hashTable->total = 0;
   pthread_mutex_init(&(hashTable->tableLock), NULL);
   hashTable->authorTotal = (int *) malloc(sizeof(int) * numAuthors);
   for (i = 0; i < numAuthors; i++)
   {
      hashTable->authorTotal[i] = 0;
   }
   return hashTable;
}

void calculateLikelihoods(hashTableT * hashtable)
{
   int i, j;
   for (j = 0; j < BINCOUNT; j++)
   {
      hashNodeT * nodePtr = hashtable->bin[j].hashNodeP;
      while (nodePtr != NULL)
      {
         for (i = 0; i < hashtable->numAuthors; i++)
         {
            nodePtr->likelihood[i] = (nodePtr->counts[i] + 1)/(float)(hashtable->authorTotal[i] 
                                    + hashtable->total + 1);
         }
         nodePtr = nodePtr->next;
      }
   }
}

binT * getBin(hashTableT * hashtable, char * ngram)
{
   int idx = hashNgram(ngram) % BINCOUNT;
   return &hashtable->bin[idx];
}

float getLikelihood(hashTableT * hashtable, char * ngram, int classId)
{
   binT * binPtr = getBin(hashtable, ngram);
   hashNodeT * nodePtr = getHashNodeFromBin(ngram, binPtr);
   if (nodePtr != NULL)
      return nodePtr->likelihood[classId - 1];
   else
      return 1/(float)(hashtable->authorTotal[classId - 1] + hashtable->total + 1);

}

hashNodeT * createHashNode(char * ngram, int numAuthors, hashNodeT * next)
{
   int i;
   hashNodeT * newHashNode = (hashNodeT *) malloc(sizeof(hashNodeT));
   newHashNode->ngram = ngram;
   newHashNode->counts = (int *) malloc(sizeof(int) * numAuthors);
   newHashNode->likelihood = (float *) malloc(sizeof(float) * numAuthors);
   for (i = 0; i < numAuthors; i++)
   {
      newHashNode->likelihood[i] = 0; 
      newHashNode->counts[i] = 0;
   }
   newHashNode->next = next;
   return newHashNode;
}

hashNodeT * getHashNodeFromBin(char * ngram, binT * binPtr)
{
   hashNodeT * nodePtr = binPtr->hashNodeP;
   while (nodePtr != NULL)
   { 
      if (strcmp(nodePtr->ngram, ngram) == 0)
         return nodePtr;
      nodePtr = nodePtr->next;
   }
   return nodePtr;
}

int insertNgramInBin(char * ngram, binT * bin, int class, int numAuthors)
{
   hashNodeT * nodePtr = getHashNodeFromBin(ngram, bin);
   if (nodePtr != NULL)
   {
      nodePtr->counts[class - 1]++;
      return 0;
   }
   hashNodeT * newNode  = createHashNode(ngram, numAuthors, bin->hashNodeP);
   newNode->counts[class - 1] = 1;
   bin->hashNodeP = newNode;
   return 1;
}

void insertNgramInHashTable(hashTableT * hashTable, char * ngram, int class)
{
   int idx = hashNgram(ngram) % BINCOUNT;
   //lock bin
   pthread_mutex_lock(&(hashTable->bin[idx].nodeLock));
   binT * binPtr = &hashTable->bin[idx];
   int newNgram = insertNgramInBin(ngram, binPtr, class, hashTable->numAuthors);
   //unlock bin
   pthread_mutex_unlock(&(hashTable->bin[idx].nodeLock));
   //lock table
   pthread_mutex_lock(&(hashTable->tableLock));
   if (newNgram)
   {
      hashTable->total++;    //number of unique words in hashtable
   }
   hashTable->authorTotal[class - 1]++;  //total number of words used by author
   //unlock table
   pthread_mutex_unlock(&(hashTable->tableLock));
}   

unsigned int hashNgram(char * ngram)
{
   int hash = 0;
   int i = 0;
   while (ngram[i])
   {
      hash = 31 * hash + ngram[i];
      i++;
   }
   return hash;
}

void printHashTable(hashTableT * hashtable)
{
   int i, j, count = 0;
   hashNodeT * nodeP;

   for (i = 0; i < BINCOUNT; i++)
   {
      nodeP = hashtable->bin[i].hashNodeP;
      if (nodeP == NULL) continue;
      while (nodeP != NULL)
      {
         printf("%d:", i);
         printf(" %6s: ", nodeP->ngram);
         count++;
         for (j = 0; j < hashtable->numAuthors; j++)
         {
            printf("id: %2d, #: %3d, p: %.4f ", j + 1, nodeP->counts[j], 
                   nodeP->likelihood[j]);
         }
         printf("\n");
         nodeP = nodeP->next;
      }
   }

   printf("Size of vocabulary: %d = %d\n", hashtable->total, count);
   for (j = 0; j < hashtable->numAuthors; j++) 
   {
      printf("Class %d - #words: %d\n", j + 1, hashtable->authorTotal[j]); 
   }
   
}
