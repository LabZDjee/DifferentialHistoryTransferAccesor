#include <stdio.h>
#include <string.h>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;

#define TRUE (!0)
#define FALSE (0)

/******************************************
 ******************************************
  Project Purpose:
  Proof of concept and test a reliable way to transfer values out an history with a limited capacity (meaning oldest events are destroyed by newest events when history is full)
  By transfer it must be understood, we can first get all the available history entries (from oldest to newest)
  As the history can have new elements added between transfers, subsequent transfers will only get the added element (still in the old to new order)
  If the number of added elements exceed the history capacity during a transfer, oldest elements will be lost
  This type of trandser is done through a so called 'differential accessor'
  
  API for this transfer
   initDifferentialAccessor():
    restarts the transfer considering all elements in the history as not already transferred
   getFromDifferentialAccessor(historyItem* pValue):
    get next element (from oldest) if any available
    if so, returns a non null value and affects *pValue with history item
    if nothing available (everything already transferred or history empty), returns null and does not change anything in *pValue
    
  Suggested ompilation: gcc -o0 -ggdb -Wall test.c -o test
  
  In this proof of concept, history is called a 'bucket'
  Unit tests are implemented to try out different type of scenarii:
   - when bucket is full or not
   - when it is flushed
   - when events are added while read
   - when events are lost because of a full unread bucket
 ******************************************
 ******************************************/ 


/******************************************
 this allows to track what is still not read
 ******************************************/ 
word diffAccessor=0; /* value which is from 0 (all already read) to bucketItemQty (maxed at bucketSize) */

/******************************************
  bucket simulation
 ******************************************/
/* capacity of bucket */
#define bucketSize (7) 

word bucket[bucketSize]; /* storage area */
word bucketItemQty=0; /* number of valid entries in bucket */
word bucketNewPayload=1; /* allows to trace insertions as this payload will regularly increase */

void flushBucket()
{
 bucketItemQty=0;
 /* this should be added for the differential accessor to work */
 diffAccessor=0;
}

void initBucket()
{
 flushBucket();
 bucketNewPayload=1;
}

void addItemInBucket()
{
 memmove((void*)(bucket+1), (void*)bucket,
  (bucketItemQty==bucketSize ? bucketSize-1 : bucketItemQty)*sizeof(*bucket));
 bucket[0]=bucketNewPayload++;
 if(bucketItemQty<bucketSize) {
  bucketItemQty++;
 }
 /* this should be added for the differential accessor to work */
 if(diffAccessor<bucketSize)
  diffAccessor++;
}

void printBucket()
{
 word i, value;
 printf("bucket: [");
 for(i=0; i<bucketItemQty; i++)
  printf("%04X%s", bucket[i], i<bucketItemQty-1 ? ", ":"");
 printf("]\n");
 for(i=0, value=bucketNewPayload-1; i<bucketItemQty; i++, value--)
  if(bucket[i] != value)
   printf("Warning: bucket is not consistent!!!\n");
}

/******************************************
  differential accessor to bucket API
 ******************************************/

/* repositionned accessor to read all avaliable elements */
void initDifferentialAccessor()
{
 diffAccessor=bucketItemQty;
}

/* get next element (from bottom), return !0 if ok and payloadRef (if not NULL) is assigned to payload value */ 
byte getFromDifferentialAccessor(word* payloadRef)
{
 if(diffAccessor>0 && bucketItemQty>0)
  {
   if(payloadRef!=NULL)
    *payloadRef = bucket[--diffAccessor];
   return TRUE;
  }
 return FALSE;
}

/******************************************
  unit tests
 ******************************************/

byte bVerbose=TRUE;

/* independant values to test consitency of API */
word numberOfInsertedItems=0;
word valueOfOldestItem;
 
 void decoratedAddItemInBucket()
 {
  if(numberOfInsertedItems==0) {
   valueOfOldestItem=bucketNewPayload;
  }
  if(bucketItemQty==bucketSize) {
   if(valueOfOldestItem==bucket[bucketItemQty-1])
    valueOfOldestItem=bucket[bucketItemQty-2];
  }
  if(numberOfInsertedItems<bucketSize) {
   numberOfInsertedItems++;
  }
  addItemInBucket();
  if(bVerbose)
   printf(" ~ value %04hX added to bucket / number of items: %hu\n", *bucket, bucketItemQty);
 }
 
 void decoratedFlushBucket(byte bInit)
 {
  numberOfInsertedItems=0;
  if(bInit)
   initBucket();
  else
   flushBucket();
  if(bVerbose)
   printf(" ~ flushed / number of items: %hu\n", bucketItemQty);
 }
 
void decoratedInitDifferentialAccessor()
{
 numberOfInsertedItems=bucketItemQty;
 if(bucketItemQty)
  valueOfOldestItem=bucket[bucketItemQty-1];
 initDifferentialAccessor();
 if(bVerbose)
  printf(" ~ differential accessor initialized / its value: %hu\n", diffAccessor);
}

int decoratedGetFromDifferentialAccessor(word testNumber, word testIteration)
{
 word value=0xffff;
 word prevBucketItemQty = bucketItemQty;
 char iterationStr[32]="";
 int result=getFromDifferentialAccessor(&value);
 if(testIteration>0) {
  sprintf(iterationStr, " (iteration %hu)", testIteration);
 }
 if(bVerbose) {
  if(result)
   printf(" ~ retrieved %04hX / diff. accessor: %hu\n", value, diffAccessor);
  else
   printf( " ~ tried to retrieve value and getFromDifferentialAccessor returned false\n");
 }
 if(result && prevBucketItemQty==0) {
  printf("FAILURE at test %hu%s: getFromDifferentialAccessor returned true despite no item was available\n", testNumber, iterationStr);
  return 0;
 }
 if(!result && numberOfInsertedItems>0) {
  printf("FAILURE at test %hu%s: getFromDifferentialAccessor returned false despite some item was available\n", testNumber, iterationStr);
  return 0;
 }
 if(!result && value!=0xffff) {
  printf("FAILURE at test %hu%s: getFromDifferentialAccessor returned false despite payload was assigned by reference\n", testNumber, iterationStr);
  return 0;
 }
 if(result && value==0xffff) {
  printf("FAILURE at test %hu%s: getFromDifferentialAccessor returned true and no payload was assigned by reference\n", testNumber, iterationStr);
  return 0;
 }
 if(result && value!=valueOfOldestItem) {
  printf("FAILURE at test %hu%s: getFromDifferentialAccessor returned true and assigned payload value is wrong (%04hX expected, %04hX assigned)\n",
         testNumber, iterationStr, valueOfOldestItem, value);
  return 0;
 }
 if(result) {
  numberOfInsertedItems--;
  valueOfOldestItem++;
 }
 return !0;
}
 
int unitTests()
{
 word i, test;
 printf("unit test %i: take all items from a full bucket\n", test=1);
 decoratedFlushBucket(TRUE);
 for(i=1; i<=bucketSize; i++) {
   decoratedAddItemInBucket();
 }
 for(i=1; i<=bucketSize+2; i++) {
  if(!decoratedGetFromDifferentialAccessor(test, i))
	return 0;
 }
 printf("unit test %i: initialize differential accessor and read again\n", test=2);
 decoratedInitDifferentialAccessor();
 for(i=1; i<=bucketSize+2; i++) {
  if(!decoratedGetFromDifferentialAccessor(test, i))
	return 0;
 }
 printf("unit test %i: take 3 added items from a full bucket\n", test=3);
 for(i=1; i<=3; i++) {
   decoratedAddItemInBucket();
 }
 for(i=1; i<=3; i++) {
  if(!decoratedGetFromDifferentialAccessor(test, i))
	return 0;
 }
 printf("unit test %i: take items just after they have been added from a full bucket\n", test=4);
 for(i=1; i<=9; i++) {
   decoratedAddItemInBucket();
   if(!decoratedGetFromDifferentialAccessor(test, i))
	return 0;
 }
 printf("unit test %i: add some items, then flushes => should get nothing\n", test=5);
 decoratedAddItemInBucket();
 decoratedFlushBucket(FALSE);
 if(!decoratedGetFromDifferentialAccessor(test, i))
	return 0;
 printf("unit test %i: add two items and get them\n", test=6);
 decoratedAddItemInBucket();
 decoratedAddItemInBucket();
 for(i=1; i<=3; i++)
  if(!decoratedGetFromDifferentialAccessor(test, i))
	return 0;
 printf("unit test %i: initialize differential differential accessor and read again\n", test=7);
 decoratedInitDifferentialAccessor();
 for(i=1; i<=3; i++)
  if(!decoratedGetFromDifferentialAccessor(test, i))
	return 0;
 printf("unit test %i: add far too many items and gets as much as possible\n", test=8);
 for(i=1; i<=bucketSize+2; i++) {
   decoratedAddItemInBucket();
 }
 for(i=1; i<=bucketSize+2; i++) {
  if(!decoratedGetFromDifferentialAccessor(test, i))
	return 0;
 }
 printf("SUCCESS: %hu tests passed\n", test);
 return !0;
}

int main(void)
{
 return unitTests();
}