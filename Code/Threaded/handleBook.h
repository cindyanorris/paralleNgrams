
typedef struct
{
   char * title;
   char * fullPath;
   char * contents;
   int classID;
   int size;
   char * current;
} bookT;

bookT * createBook(char * path, char * fname, int classID);
char * getNextNgram(bookT * book, char * ngramType, int ngramSize);
void freeBook(bookT * book);
void readContents(bookT * book, int startIdx, char * amount);
void normalize(bookT * book);
