typedef struct
{
   void * value;
   int type;   //string, array, object
} valueT;

typedef struct memberT
{
   char * string;
   valueT * value;
   struct memberT * nextMember;
} memberT;

typedef struct
{
   memberT * member;
} objectT;

typedef struct
{
   valueT ** elements; //array of pointers to valueTs
   int size; //size of array
} arrayT;

#define STRING 10
#define ARRAY 11
#define OBJECT 12 

objectT * buildJsonObject(char * fileName);
void printJsonObject();
void * getMemberValueFromObject(objectT *, char * string, int type);
valueT * getElementFromArray(arrayT * arr, int index, int type);
void * getValueFromValueT(valueT * value, int type);
