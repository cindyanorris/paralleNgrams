#include <stdlib.h>
#include <stdio.h>

void usage()
{
   fprintf(stderr, "Usage: serialNgram <filename>\n");
   fprintf(stderr, "       <filename> - name of text file that contains json to specify how to run experiment\n");
   exit(1);
}
