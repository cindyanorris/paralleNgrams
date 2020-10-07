#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "handleBook.h"
#include "usage.h"

static int findSize(const char *file_name);
static char * getNextNgramByte(bookT * book, int ngramSize);
static char * getNextNgramWord(bookT * book, int ngramSize);
static void allocateContents(bookT * book, int *startIdx, char * amount);

bookT * createBook(char * path, char * fname, int classID)
{
   bookT * newBook = (bookT *) malloc(sizeof(bookT));

   newBook->fullPath = (char *) malloc(strlen(fname) + strlen(path) + 1);
   strcpy(newBook->fullPath, path);
   strcpy(&(newBook->fullPath[strlen(path)]), fname);

   newBook->classID = classID;
   newBook->title = strdup(fname);
   return newBook;
}

void freeBook(bookT * book)
{
   free(book->fullPath);
   free(book->title);
   free(book->contents);
   free(book);
}

void allocateContents(bookT * book, int *startIdx, char * amount)
{
   int fileSize = findSize(book->fullPath);
   int indicatedAmount = atoi(amount);

   if (strcmp(amount, "ALL") == 0 || indicatedAmount == 0 || (indicatedAmount + (*startIdx)) > fileSize)
   {
      book->size = fileSize;
      (*startIdx) = 0;
   }
   else
   {
      book->size = indicatedAmount;
   }

   book->contents = (char *) malloc(sizeof(char) * book->size + 1);
   if (book->contents == NULL)
   {
      fprintf(stderr, "Malloc of %d bytes failed\n", book->size + 1);
      usage();
   }
   book->current = &(book->contents[0]);
}

void readContents(bookT * book, int startIdx, char * amount)
{
   int BUFSIZE = 2048;
   int amountRead = 0;
   allocateContents(book, &startIdx, amount);
   int contentSize = book->size;

   FILE *fp;
   int i = 0, n = 0; 
   char buff[startIdx];
   fp = fopen(book->fullPath, "r");
   //read and discard the first startIdx bytes;
   if (startIdx > 0) n = fread(buff, 1, startIdx, fp);
   if (n < startIdx)
   {
      printf("Unable to read %d bytes from file\n", startIdx);
      usage();
   }
   do
   {
      if (contentSize - amountRead < BUFSIZE) BUFSIZE = contentSize - amountRead;
      n = fread(&book->contents[i], 1, BUFSIZE, fp);
      i = i + n;
      amountRead += n;
   } while (n > 0 && i < book->size);
   book->contents[book->size] = '\0';
}

void normalize(bookT * book)
{
   long int inputIdx = 0, outputIdx = 0;
   long int fsize = book->size;
   int whiteSpace = 0;
   for (inputIdx = 0; inputIdx < fsize; inputIdx++)
   {
      char nxtChar = book->contents[inputIdx];
      if (isspace(nxtChar))
      {
         if (!whiteSpace) book->contents[outputIdx++] = ' ';
         whiteSpace = 1;
      } else
      {
         if (nxtChar >= 'A' && nxtChar <= 'Z') nxtChar = tolower(nxtChar);
         book->contents[outputIdx++] = nxtChar;
         whiteSpace = 0;
      }
   }
   if (outputIdx < fsize) book->contents[outputIdx] = '\0';
   book->size = outputIdx;
}

int findSize(const char * fileName)
{
   struct stat st;

   if (stat(fileName,&st)==0)
      return (st.st_size);
   else
      return -1;
}

char * getNextNgram(bookT * book, char * ngramType, int ngramSize)
{
   if (strcmp(ngramType, "BYTE") == 0)
      return getNextNgramByte(book, ngramSize);
   else if (strcmp(ngramType, "WORD") == 0)
      return getNextNgramWord(book, ngramSize);
   else
   {
      fprintf(stderr, "ngram type must be BYTE or WORD\n");
      usage();
   }
   return 0;
}

char * getNextNgramByte(bookT * book, int ngramSize)
{
   char ngram[ngramSize + 1];
   int i;
   char * ptr = book->current;
   book->current++;
   for (i = 0; i < ngramSize && *ptr != '\0'; i++)
   {
      ngram[i] = *ptr;
      ptr++;
   }
   if (i < ngramSize) return NULL;
   ngram[ngramSize] = '\0';
   return strdup(ngram);
}

char * getNextNgramWord(bookT * book, int ngramSize)
{
   int spaceCt = 0;
   char * ngramPtrBeginning = book->current;
   char * nxtNgramPtr;
   char * ngramPtr;

   //skip beginning whitepace and punctuation
   while (!isalnum(*ngramPtrBeginning) && (*ngramPtrBeginning) != '\0') ngramPtrBeginning++;

   //find the end of the ngram
   ngramPtr = ngramPtrBeginning;
   while ((*ngramPtr) != '\0' && spaceCt < ngramSize)
   {
      if  ((*ngramPtr) == ' ')
      {
         spaceCt++;
         if (spaceCt == 1) nxtNgramPtr = ngramPtr + 1;
      }
      ngramPtr++;
   }
   if (spaceCt < ngramSize) return NULL;

   ngramPtr--;

   //remove ending whitespace and punctuation
   while (!isalnum(*ngramPtr) && (ngramPtr > book->current)) ngramPtr--;
   ngramPtr++;
   char holdChar = *ngramPtr;
   (*ngramPtr) = '\0';
   char * nxtNgram = strdup(ngramPtrBeginning);
   (*ngramPtr) = holdChar;
   book->current = nxtNgramPtr;
   return nxtNgram;
}

