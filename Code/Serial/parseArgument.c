#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parseArgument.h"
#include "usage.h"

#define NONE 0
#define OPENBRACE 1
#define CLOSEDBRACE 2
#define OPENBRACKET 3
#define CLOSEDBRACKET 4
#define QUOTE 5
#define COLON 6
#define COMMA 7 

static int findSize(const char * fileName);
static void readContents(char * fileName, char * contents);
static char * getNextToken(char * contents, int * i);
static void jsonError(char * errMsg);
static objectT * buildObject(char * contents, int * idx);
static objectT * createObject();
static int isType(char * token);
static char * getString(char * token);
static memberT * createMember(char * token);
static arrayT * buildArray(char * contents, int * idx);
static arrayT * createArray(int numEles);
static valueT * buildValue(char * contents, int * idx);
static void printValue(valueT * value);
static void printObject(objectT * obj);
static void printArray(arrayT * array);
static int lineNum = 1;

objectT * buildJsonObject(char * fileName)
{
  objectT * jsonObj;
  int fileSize, i;
  //read the entire file
  if ((fileSize = findSize(fileName)) == -1)
  {
     fprintf(stderr, "findSize error: %s\n", fileName);
     usage();
  }
  char * contents = (char *) malloc(fileSize + 1);
  if (contents == NULL)
  {
      fprintf(stderr, "Malloc of %d bytes failed\n", fileSize + 1);
      usage();
  }
  readContents(fileName, contents);
  i = 0;
  char * token;
  lineNum = 1;
  token = getNextToken(contents, &i);
  if (isType(token) != OPENBRACE) jsonError("json error: '{' expected");

  jsonObj = buildObject(contents, &i);
  return jsonObj;

}

void * getMemberValueFromObject(objectT * obj, char * string, int type)
{
   memberT * member = obj->member;
   while (member != NULL)
   {
      if (strcmp(member->string, string) == 0 && member->value->type == type) 
         return (void *) member->value->value;
      member = member->nextMember;
   }
   jsonError("Unable to find desired member");
   return 0;
}

void *  getValueFromValueT(valueT * value, int type)
{
   if (value->type != type)
      jsonError("Unable to find desired type within valueT");
   return value->value;
}

valueT * getElementFromArray(arrayT * arr, int index, int type)
{
   valueT * element = arr->elements[index];
   if (element->type != type)
      jsonError("Unable to find desired element type");
   return element;
}

objectT * createObject()
{
  objectT * newObj = (objectT *) malloc(sizeof(objectT));
  newObj->member = NULL;
  return newObj;
}

memberT * createMember(char * token)
{
   memberT * newMem = (memberT *) malloc(sizeof(memberT));
   newMem->string = getString(token);
   newMem->value = NULL;
   newMem->nextMember = NULL;
   return newMem;
}

int countElements(char * contents, int i)
{
   int openbracket = 1;
   int countcomma = 1;
   int openbrace = 0;
   int elementCount = 0;
   i++;
   while (openbracket)
   {
      if (contents[i] == ',' && countcomma) elementCount++;
      if (contents[i] == '{')
      {
          openbrace++;
          countcomma = 0;
      }
      if (contents[i] == '[')
      {
         openbracket++;
         countcomma = 0;
      }
      if (contents[i] == ']') openbracket--; 
      if (contents[i] == '}') openbrace--; 
      if (openbrace == 0 && openbracket == 1) countcomma = 1;
      i++;
   }
   return elementCount + 1;
}

arrayT * createArray(int numEles)
{
   arrayT * newArray = (arrayT *) malloc(sizeof(arrayT));
   newArray->elements = (valueT **) malloc(sizeof(valueT *) * numEles);
   newArray->size = numEles;
   return newArray;
}

arrayT * buildArray(char * contents, int *idx)
{
   int numEles = countElements(contents, *idx);
   arrayT * newArray = createArray(numEles);
   int i;
   char * token;
   newArray->size = numEles;
   for (i = 0; i < numEles; i++)
   {
      newArray->elements[i] = buildValue(contents, idx);
      //read comma or closing bracket
      token = getNextToken(contents, idx);
      if (isType(token) != COMMA && isType(token) != CLOSEDBRACKET)
         jsonError("json error: comma or closed bracket expected");
   }
   return newArray;
}

valueT * buildValue(char * contents, int * idx)
{
   valueT * newValue = (valueT *) malloc(sizeof(valueT));
   char * token = getNextToken(contents, idx);
   if (isType(token) == QUOTE)
   {
      newValue->value = getString(token);
      newValue->type = STRING;
   } else if (isType(token) == OPENBRACE)
   {
      newValue->type = OBJECT;
      newValue->value = buildObject(contents, idx);
   } else if (isType(token) == OPENBRACKET)
   {
      newValue->type = ARRAY;
      newValue->value = buildArray(contents, idx);
   }
   return newValue;
}

objectT * buildObject(char * contents, int * idx)
{
  objectT * newObj = createObject();
  memberT ** memberP = &(newObj->member);
  char * token;
  token = getNextToken(contents, idx);
  while (isType(token) != CLOSEDBRACE)
  {
     (*memberP) = createMember(token);
     token = getNextToken(contents, idx);
     if (isType(token) != COLON) jsonError("json error: ':' expected");
     (*memberP)->value = buildValue(contents, idx);
     memberP = &((*memberP)->nextMember);
     token = getNextToken(contents, idx);
     if (isType(token) == COMMA) token = getNextToken(contents, idx);
  }
  return newObj; 
}

//strip out the quotes
char * getString(char * token)
{
   int i;
   for (i = 1; token[i] != '"'; i++)
      token[i - 1] = token[i];
   token[i - 1] = 0;
   return token;
}

int isType(char * token)
{
   if (strcmp(token, "{") == 0)
      return OPENBRACE;
   else if (strcmp(token, "[") == 0)
      return OPENBRACKET;
   else if (token[0] == '"') 
      return QUOTE;
   else if (strcmp(token, "}") == 0)
      return CLOSEDBRACE;
   else if (strcmp(token, "]") == 0)
      return CLOSEDBRACKET;
   else if (strcmp(token, ":") == 0)
      return COLON;
   else if (strcmp(token, ",") == 0)
      return COMMA;
   else
      return NONE;
}

void jsonError(char * errMsg)
{
   fprintf(stderr, "line number %d: %s\n", lineNum, errMsg);
   usage();
}

int isdelimiter(char aChar)
{
   return (aChar == ',' || aChar == '{' || aChar == '}' ||
           aChar == ']' || aChar == '[' || aChar == ':');
}

char * getNextToken(char * contents, int * idx)
{
   static int lasttoken = NONE;
   char buffer[100];
   int j = 0, currtoken;
   int i = (*idx);
   while (contents[i] != '\0' && isspace(contents[i])) 
   {
      if (contents[i] == '\n') lineNum++;
      i++;
   }
   if (isdelimiter(contents[i]))
   {
      buffer[0] = contents[i++];
      buffer[1] = '\0';
   } else if (contents[i] != '"')
   {
      jsonError("json error: quote expected");
   } else
   {
      buffer[j++] = contents[i++]; //get opening quote
      while (contents[i] != '\0' && contents[i] != '"')
      {
         buffer[j++] = contents[i++];
      }
      buffer[j++] = contents[i++]; //get closing quote
      buffer[j] = '\0';
   }
   (*idx) = i;
   currtoken = isType(buffer); 
   if ((currtoken == CLOSEDBRACE || currtoken == CLOSEDBRACKET) && (lasttoken == COMMA))
      jsonError("json error: comma before closing brace or bracket");
   lasttoken = currtoken;   
   return strdup(buffer);
}


int findSize(const char * fileName)
{
   struct stat st;

   if (stat(fileName, &st) == 0)
      return (st.st_size);
   else
      return -1;
}

void readContents(char * fileName, char * contents)
{
   FILE *fp;
   int i = 0, n;
   int BUFFER_SIZE = 2048;
   char buff[BUFFER_SIZE];
   contents[0] = '\0';
   char * str;

   fp = fopen(fileName, "r");
   do
   {
        str = fgets(buff, BUFFER_SIZE, fp);
        if (str != NULL && str[0] != '#')
        {
           strcat(contents, str);
        }
   } while (str != NULL);
   //printf("%s\n", contents);
}

void printJsonObject(objectT * jsonObj)
{
   if (jsonObj == NULL)
   {
      jsonError("jsonObj is undefined");
   }
   printObject(jsonObj);
}

void printValue(valueT * value)
{
   if (value->type == STRING)
      printf("\"%s\"", (char *) value->value);
   else if (value->type == OBJECT)
      printObject((objectT *) value->value);
   else if (value->type == ARRAY)
      printArray((arrayT *) value->value);
}

void printArray(arrayT * array)
{
   int size = array->size;
   int i;
   printf("\n[\n");
   for (i = 0; i < size; i++)
   {
      printValue(array->elements[i]);
      if (i < size - 1) printf(", ");
   }
   printf("\n]");
}

void printObject(objectT * obj)
{
   printf("\n{\n");
   memberT * mptr = obj->member;
   while (mptr != NULL)
   {
      printf("\"%s\" : ", mptr->string);
      printValue(mptr->value);
      mptr = mptr->nextMember;
      if (mptr != NULL) printf(", ");
      printf("\n");
   }
   printf("}");
}




