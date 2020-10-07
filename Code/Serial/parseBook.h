
typedef struct
{
   char * title;
   char * fullPath;
   char * contents;
   int class;
   int size;
   int startIdx;
   char * current;
} bookT;

bookT * createBook(char * path, char * fname, int class, int idxStart, char * amount);
char * getNextNgram(bookT * book, char * ngramType, int ngramSize);
void freeBook(bookT * book);
void readContents(bookT * book);
void normalize(bookT * book);
