#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "parseBook.h"
#include "usage.h"

static int findSize(const char *file_name);
static char * getNextNgramByte(bookT * book, int ngramSize);
static char * getNextNgramWord(bookT * book, int ngramSize);

bookT * createBook(char * path, char * fname, int class, int startIdx, char * amount)
{
   bookT * newBook = (bookT *) malloc(sizeof(bookT));

   newBook->fullPath = (char *) malloc(strlen(fname) + strlen(path) + 1);
   strcpy(newBook->fullPath, path);
   strcpy(&(newBook->fullPath[strlen(path)]), fname);

   newBook->class = class;
   newBook->title = strdup(fname);
   newBook->startIdx = startIdx;

   int fileSize = findSize(newBook->fullPath);
   int indicatedAmount = atoi(amount);
   if (strcmp(amount, "ALL") == 0 || indicatedAmount == 0 || (indicatedAmount + startIdx) > fileSize)
   {
      newBook->size = fileSize;
      newBook->startIdx = 0;
   }
   else
   {
      newBook->size = indicatedAmount;
   }

   newBook->contents = (char *) malloc(sizeof(char) * newBook->size + 1);
   if (newBook->contents == NULL)
   {
      fprintf(stderr, "Malloc of %d bytes failed\n", newBook->size + 1);
      usage();
   }
   newBook->current = &(newBook->contents[0]);
   return newBook;
}

void freeBook(bookT * book)
{
   free(book->fullPath);
   free(book->title);
   free(book->contents);
   free(book);
}

void readContents(bookT * book)
{
   int BUFSIZE = 2048;
   int amountRead = 0;
   int contentSize = book->size;
   int startIdx = book->startIdx;
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
   return NULL;
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

