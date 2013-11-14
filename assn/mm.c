/*
 * This uses the segregated first fit algorithm to find blocks from
 * the free list.
 * The binning strategy used is to have ranges of sizes for large
 * sized free blocks and direct mapping for smaller sized blocks.
 * Blocks are coalesced and split accordingly
 * Realloc is implemented directly using mm_malloc and mm_free.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Mallocers",
    /* First member's full name */
    "Sachin Siby",
    /* First member's email address */
    "sachinsiby@gmail.com",
    /* Second member's full name (leave blank if none) */
    "Pranoy De",
    /* Second member's email address (leave blank if none) */
    "pranoyde@gmail.com"
};

/*************************************************************************
 * Basic Constants and Macros
 * You are not required to use these macros but may find them helpful.
 *************************************************************************/
#define WSIZE       sizeof(void *)            /* word size (bytes) */
#define DSIZE       (2 * WSIZE)            /* doubleword size (bytes) */
#define CHUNKSIZE   (1<<7)      /* initial heap size (bytes) */
#define OVERHEAD	DSIZE
#define MAX(x,y) ((x) > (y)?(x) :(y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)          (*(uintptr_t *)(p))
#define PUT(p,val)      (*(uintptr_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)     (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p)    (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)        ((char *)(bp) - WSIZE)
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define BIN_SIZE 64

//#define NUM_BINS 66
#define NUM_BINS 33


/******* Function Headers*********************/

void *getBestFit(void* baseOfIndex,size_t adjustedSize,int currIndex);
void *extendHeapAndAlloc(size_t adjustedSize);
void updateOH(void* blockPointer,size_t adjustedSize);

/*******Linked List functions******************/
void removeFromFreeList(void* blockPointer);
void addFreeList(void* blockPointer);


/* Global variables*/
void* heap_listp = NULL;
void* HeapStart = NULL;
void* MemStart = NULL;



size_t HeapSize = 0;

void* extend_heap_init(size_t);


/*  printSegList
	Helper function to print segregated list
*/
void printSegList()
{
    int i;
    for(i =0; i< NUM_BINS; i++)
    {
        int label = (i+1)*16;
        void* binPtr = HeapStart + WSIZE*i;
        if(binPtr)
        {
            void* currentNode = GET(binPtr);
            while(currentNode)
            {
                printf("%p-->",currentNode);
                fflush(stdout);
                currentNode = GET(currentNode+WSIZE);
            }
        }
    }
}


/**********************************************************
 * mm_init
 * Initialize the heap.
 * Stores address to different segregations.
 * Extends heap by NUM_BINS
 **********************************************************/

 int mm_init(void){
		
		int i;
		
		if((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
         	return -1;
     	PUT(heap_listp, 0);                         // alignment padding
     	PUT(heap_listp + (1 * WSIZE), PACK(OVERHEAD, 1));   // prologue header
    	PUT(heap_listp + (2 * WSIZE), PACK(OVERHEAD, 1));   // prologue footer
     	PUT(heap_listp + (3 * WSIZE), PACK(0, 1));    // epilogue header
     	heap_listp += DSIZE;
     	
     	void* temp;
     	int segListSize = NUM_BINS*WSIZE;
     	int segListAlloc = DSIZE * ((segListSize + (OVERHEAD) + (DSIZE - 1))/DSIZE);
     	size_t words = segListAlloc/WSIZE;
     	
     	int size = (words % 2 ) ? (words + 1) * WSIZE: words * WSIZE;
     	
     	  if ( (temp = mem_sbrk(size)) == (void *)-1 )
                return NULL;
     	
     	PUT(HDRP(temp), PACK(size, 1));                // free block header
        PUT(FTRP(temp), PACK(size, 1));                // free block footer
        PUT(HDRP(NEXT_BLKP(temp)), PACK(0, 1));        // new epilogue header
 		
     	
     	HeapStart = temp;
     	
     	for(i=0; i<NUM_BINS;i++)
     	{
      		 PUT(HeapStart+i*WSIZE, NULL);
      	}
     	
     	return 0;
}


int testmm_init()
{

    int i = 0;
    ////printf("Initial Heapsize is %zu\n",HeapSize);
    for (i=0; i < NUM_BINS; i++)
    {
        void* binPtr = HeapStart + WSIZE* i;        
        if(GET(binPtr))
        {
            ////printf("Error initializing %d\n",i);
        }
        else
        {
            ////printf("Correct initializing %d\n",i);
        }

    }
    ////printf("Final Heapsize is %zu\n",HeapSize);
}



/**********************************************************
 * coalesce
 * Covers the 4 cases discussed in the text:
 * - both neighbours are allocated
 * - the next block is available for coalescing
 * - the previous block is available for coalescing
 * - both neighbours are available for coalescing
 **********************************************************/
void *coalesce(void *bp)
{

	
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
	
	

    if (prev_alloc && next_alloc) {       /* Case 1 */
		
        return bp;
    }
	
    else if ((prev_alloc) && (!next_alloc)) { /* Case 2 */
		
		size_t nextSize = GET_SIZE(HDRP(NEXT_BLKP(bp)));


		
		size_t newSize;		
		void* bpNext = NEXT_BLKP(bp);		
		newSize = size + nextSize;		

		
		removeFromFreeList(bpNext);		
		
		//Update OH of final block
		PUT(HDRP(bp), PACK(newSize, 0));
        PUT(FTRP(bp), PACK(newSize, 0));
	
		
        return (bp);
    }

    else if ((!prev_alloc) && (next_alloc)) { /* Case 3 */
		
		size_t prevSize = GET_SIZE(FTRP(PREV_BLKP(bp)));
		
		size_t newSize;
		
        void* bpPrev = PREV_BLKP(bp);

		newSize = size + prevSize;
	
		
		removeFromFreeList(bpPrev);	

		//Update OH of final block
		PUT(FTRP(bp), PACK(newSize, 0));
        PUT(HDRP(bpPrev), PACK(newSize, 0));
	 

		return (bpPrev);
    }

    else{            /* Case 4 */
		

		size_t prevSize = GET_SIZE(HDRP(PREV_BLKP(bp)));
		void* bpPrev = PREV_BLKP(bp);

		size_t nextSize = GET_SIZE(HDRP(NEXT_BLKP(bp)));
		void* bpNext = NEXT_BLKP(bp);

		size_t newSize = size + prevSize + nextSize;
		
		//remove all from free list
		removeFromFreeList(bpPrev);	
		removeFromFreeList(bpNext);	
	
		//Update OH of final block
		PUT(HDRP(PREV_BLKP(bp)), PACK(newSize,0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(newSize,0));
	
        return (bpPrev);
    }
}

void* split (void* mainBlock, size_t adjustedSize);

/*
 *Unit test for coalesce
 *
*/
void testCoalesce()
{

	void* a = mm_malloc(288);
	////printf("This - %p, Prev-%p\n",a, PREV_BLKP(a));
	mm_free(a);	
	////printf("After Free This - %p, Prev-%p\n",a, PREV_BLKP(a));

	////printf("Before split\n");
	printSegList();
	//a = split(a,64);

	//void* b = mm_malloc(b);
	//////printf("After split\n");
	//printSegList();	

	//////printf("Before coalesce\n");
	//a = coalesce(a);
	//printSegList();
	//addToFreeList(a);

	//////printf("After coalesce\n");
	//printSegList();

}



void addToFreeList(void* blockPointer)
{

	size_t adjustedSize = GET_SIZE(HDRP(blockPointer));
			
    int currIndex = getIndex(adjustedSize);
    void* baseFromIndex = HeapStart + currIndex*WSIZE;
    
    //////printf("location is %p\n",baseFromIndex);
    void* head = GET(baseFromIndex);

    //Change head in global segregated list
    PUT(baseFromIndex, blockPointer);

    //Change previous and next
    PUT(blockPointer+WSIZE, head);
    PUT(blockPointer, NULL);

    if(head)
    {
        PUT(head,blockPointer);
    }

	
}




void *extend_heap_init(size_t words) {
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignments */
    //size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    size = words * WSIZE;
    if ( (bp = mem_sbrk(size)) == (void *)-1 )
        return NULL;

    HeapSize = HeapSize + size;
    return bp;
}


/**********************************************************
 * extend_heap
 * Extend the heap by "words" words, maintaining alignment
 * requirements of course. Free the former epilogue block
 * and reallocate its new header
 **********************************************************/
void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignments */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ( (bp = mem_sbrk(size)) == (void *)-1 )
        return NULL;

   // bp = bp + WSIZE;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));                // free block header
    PUT(FTRP(bp), PACK(size, 0));                // free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));        // new epilogue header

    /*Increment global counter*/
    HeapSize = HeapSize + size;

    /* Coalesce if the previous block was free */
    //return coalesce(bp);
    return bp;
}


/**********************************************************
 * find_fit
 * Traverse the heap searching for a block to fit asize
 * Return NULL if no free blocks can handle that size
 * Assumed that asize is aligned
 **********************************************************/
void * find_fit(size_t asize)
{
    void *bp;
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
        {
            return bp;
        }
    }
    return NULL;
}

/**********************************************************
 * place
 * Mark the block as allocated
 **********************************************************/
void place(void* bp, size_t asize)
{

    /* Get the current block size */
    size_t bsize = GET_SIZE(HDRP(bp));

    /* Set allocated value to "used" */
    PUT(HDRP(bp), PACK(bsize, 1));
    PUT(FTRP(bp), PACK(bsize, 1));
}


size_t getAdjustedSize(size_t size)
{
    /* Adjust block size to include overhead and alignment reqs. */
    size_t asize;    
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1))/ DSIZE);

    return asize;
}


int getIndex(size_t size){
			//FINDS THE INDEX IN THE SEG LIST
			int index = -100;
			
			if(size<=BIN_SIZE)
				index= size/DSIZE -1;
				
			else if (size > BIN_SIZE && size <= 2*BIN_SIZE)
				index=16;
			else if (size > 2*BIN_SIZE && size <= 4*BIN_SIZE)
				index=17;
			else if (size > 4*BIN_SIZE && size <= 8*BIN_SIZE)
				index=18;
			else if (size > 8*BIN_SIZE && size <= 12*BIN_SIZE)
				index=19;
			else if (size > 12*BIN_SIZE && size <= 16*BIN_SIZE)
				index=20;
			else if (size > 16*BIN_SIZE && size <= 20*BIN_SIZE)
				index=21;
			else if (size > 20*BIN_SIZE && size <= 24*BIN_SIZE)
				index=22;
			else if (size > 24*BIN_SIZE && size <= 28*BIN_SIZE)
				index=23;
			else if (size > 28*BIN_SIZE && size <= 32*BIN_SIZE)
				index=24;
			else if (size > 32*BIN_SIZE && size <= 36*BIN_SIZE)
				index=25;
			else if (size > 36*BIN_SIZE && size <= 40*BIN_SIZE)
				index=26;
			else if (size > 40*BIN_SIZE && size <= 60*BIN_SIZE)
				index=27;
			else if (size > 60*BIN_SIZE && size <= 120*BIN_SIZE)
				index=28;
			else if (size > 120*BIN_SIZE && size <=200*BIN_SIZE)
				index=29;
			else if (size > 200*BIN_SIZE && size <=400*BIN_SIZE)
				index=30;
			else if (size > 400*BIN_SIZE && size <= 4000*BIN_SIZE)
				index=31;
			else
				index=32;
				
			return index;
	}
			

/**********************************************************
 * mm_free
 * Free the block and coalesce with neighbouring blocks
 **********************************************************/
void mm_free(void *blockPointer)
{
   
    //////printf("Entering free\n");
    if(blockPointer == NULL){
        return;
    }

    size_t adjustedSize = GET_SIZE(HDRP(blockPointer));

    ////printf("Size to free is %zu\n",adjustedSize);
    PUT(HDRP(blockPointer), PACK(adjustedSize,0));
    PUT(FTRP(blockPointer), PACK(adjustedSize,0));
	////printf ("Before coalescing %zu \n", GET_SIZE(HDRP(blockPointer)));
    blockPointer = coalesce(blockPointer);
	//////printf ("AFTER COALESCING BIATCHES ............................\n");
	//////printf ("After coalescing %zu \n", GET_SIZE(HDRP(blockPointer)));
	addToFreeList(blockPointer);
	
}


/*
 *Unit test for free()
 *
*/
void testFree()
{

    int* a = (int*)mm_malloc(sizeof(int*));
    int* b = (int*)mm_malloc(sizeof(int*));

    void* c = mm_malloc(128);

    mm_free(a);
    printSegList();
    ////printf("Freeed a\n");

    mm_free(b);
    printSegList();
    ////printf("Freed b\n");

    mm_free(c);
    printSegList();
    ////printf("Adjusted size of 128 is %zu\n",getAdjustedSize(128));
    ////printf("Freed c\n");

    ////printf("Index of 128 is %d\n",getIndex(128));

}




/**********************************************************
 * mm_malloc
 * Allocate a block of size bytes.
 * The type of search is determined by find_fit
 * The decision of splitting the block, or not is determined
 *   in place(..)
 * If no block satisfies the request, the heap is extended
 **********************************************************/


void *mm_malloc(size_t size)
{

    size_t adjustedSize; /* adjusted block size */
    size_t extendsize; /* amount to extend heap if no fit */
    char * bp;
    int currIndex = 0;
    void* baseOfIndex;
    char* assignedBlock = NULL;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Search the free list for a fit */

    adjustedSize = getAdjustedSize(size);
    currIndex = getIndex(adjustedSize); 

    while((!assignedBlock) && (currIndex < NUM_BINS))
    {
        baseOfIndex = HeapStart + currIndex*WSIZE;
        if(GET(baseOfIndex))
        {
            assignedBlock = getBestFit(baseOfIndex,adjustedSize,currIndex);
        }
        currIndex++;
    }   

    if(!assignedBlock)
    {
        assignedBlock = extendHeapAndAlloc(adjustedSize);
        if(assignedBlock==NULL)
        {
            return NULL;
        }
		//return split(assignedBlock,adjustedSize);
        place(assignedBlock, adjustedSize);
        return assignedBlock;
    }


    place(assignedBlock, adjustedSize);

    return assignedBlock;
}

/*
 *Unit test for malloc()
 *
 */



void testMalloc()
{

    size_t adjustedSize = 1000000;
    size_t adjustedSize2 = 10000;

    size_t mallocRequest = 100;    

    char* bp;
    char* bp2;
    void* baseOfIndex;
    void* testIndex;

    bp = extend_heap(adjustedSize/WSIZE);
    if(bp)
    {
        int currIndex = getIndex(adjustedSize);
        baseOfIndex = currIndex*WSIZE + HeapStart;
        PUT(baseOfIndex,bp);
    }
    else
    {

        return;
    }

    bp2 = extend_heap(adjustedSize2/WSIZE);
    if(bp2)
    {
        int currIndex = getIndex(adjustedSize2);
        baseOfIndex = currIndex*WSIZE + HeapStart;
        PUT(GET(baseOfIndex)+WSIZE,bp2);
        PUT(bp2,GET(baseOfIndex));
    }
    else
    {
        return;
    }

    printSegList();

    char* test = mm_malloc(mallocRequest);
    printSegList();
}




void* split (void* mainBlock, size_t adjustedSize)
{
    
    size_t mainSize = GET_SIZE(HDRP(mainBlock));

	

	removeFromFreeList(mainBlock);
  
    if(mainSize -adjustedSize<32)
    {
		
        return mainBlock;
    }
    

	if(mainSize-adjustedSize>=32)
	{
		size_t remSize = mainSize - adjustedSize;
		size_t wantedPayLoad = adjustedSize - DSIZE;
		
		void* wantBlock = mainBlock;
		void* remBlock = mainBlock + wantedPayLoad + DSIZE;



		/*PUT(remBlock-WSIZE,PACK(remSize,0));
		PUT(remBlock - DSIZE,PACK(adjustedSize,0));
		PUT(FTRP(wantBlock),PACK(remSize,0));
		PUT(HDRP(wantBlock),PACK(adjustedSize,0));*/
		
		updateOH(wantBlock,adjustedSize);
		updateOH(remBlock,remSize);

		
		//addToFreeList(wantBlock);
		addToFreeList(remBlock);

		
		return wantBlock;
	}
	
	
}

/*
 *Unit test for split()
 *
 */

void testSplit()
{
    //Allocate one 64 bit chunk
    //void* a = mm_malloc(10000000000);
    void* a = mm_malloc(112);

    //void* b = mm_malloc(112);

    //Free it
    mm_free(a);
    //mm_free(b);

    //Printe Free List
    printSegList();

    
    
    split(a,4096);

    
    printSegList();
}


void updateOH(void* blockPointer,size_t adjustedSize)
{
    /* Set allocated value to "unused" */

	
    PUT(HDRP(blockPointer), PACK(adjustedSize, 0));
    PUT(FTRP(blockPointer), PACK(adjustedSize, 0));
}


void removeFromFreeList(void* blockPointer)
{
    if(blockPointer==NULL)
    {
        return; 
    }

    void* prev = GET(blockPointer);
    void* next = GET(blockPointer+WSIZE);
	
	
    
	if(prev!=NULL)
    {
		
        GET(prev+WSIZE) = next;
    }

    if(next!=NULL)
    {
		
        GET(next) = prev;
    }

    
    if(prev==NULL)
    {
        //calculate currIndex
        size_t size = GET_SIZE(HDRP(blockPointer));
        int currIndex = getIndex(size);
        void* baseOfIndex = currIndex*WSIZE + HeapStart;
        PUT(baseOfIndex,next);
    }

}

void addFreeList(void* blockPointer)
{
    mm_free(blockPointer);
}

/***********************************************************
 * Re written implementation of find_fit()
 * Uses first fit to check each bin for the required size block
 * getBestFit
 *
 **********************************************************/
void *getBestFit(void* baseOfIndex,size_t adjustedSize,int currIndex)
{


	void* currentHead = GET(baseOfIndex);
	
		while(currentHead)
		{
			if(adjustedSize <=GET_SIZE(HDRP(currentHead)))			
			{
				void* splitPointer = split(currentHead,adjustedSize);
				return splitPointer;
			}
			currentHead = GET(currentHead+WSIZE);
		}	

	return NULL;


	
} 
/***********************************************************
 *Unit Test for getBestFit()
 ***********************************************************/


void testGetBestFit()
{
    size_t adjustedSize = 32;
    size_t adjustedSize2 = 32;

    char* bp;
    char* bp2;
    void* baseOfIndex;
    void* testIndex;

    bp = extend_heap(adjustedSize/WSIZE);
    if(bp)
    {
        int currIndex = getIndex(adjustedSize);
        baseOfIndex = currIndex*WSIZE + HeapStart;
        PUT(baseOfIndex,bp);
    }
    else
    {
        printf("extend heap failed\n");
        return;
    }

    bp2 = extend_heap(adjustedSize2/WSIZE);
    if(bp2)
    {
        int currIndex = getIndex(adjustedSize2);
        baseOfIndex = currIndex*WSIZE + HeapStart;
        void* head = GET(baseOfIndex);

        PUT(head+WSIZE,bp2);
        PUT(bp2,head);
        PUT(bp2+WSIZE,NULL);
    }
    else
    {
       
        return;
    }

   
    printSegList();
}

void *extendHeapAndAlloc(size_t adjustedSize)
{
    /*If block not found in free list extend the heap*/
    size_t extendsize;
    extendsize = MAX(adjustedSize, CHUNKSIZE);

    char* bp; //block pointer
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    return bp;
}

/******************************************************************* 
 * mm_realloc()
 * More efficient than previous implementation due to coalescing with
 * next block and splitting in case of excess space.
 ********************************************************************/
void *mm_realloc(void *ptr, size_t size)
	{	

		if (size == 0){
			mm_free(ptr);
			return NULL;
		}

	
    // if old is null, this is the same as malloc

	if(ptr==NULL)
		return (mm_malloc(size));

		void* oldptr = ptr;
		void* newptr;
		size_t oldSize = GET_SIZE(HDRP(oldptr));
		size_t asize = size + DSIZE;
		//diff between new and old size is 32
		if(asize < oldSize && (oldSize -(asize)) > 4*WSIZE){
			size = getAdjustedSize(size);

			//SPLIT THE BLOCK

			size_t remSize =  GET_SIZE(HDRP(ptr)) - size;
		    size_t wantedPayLoad = size - DSIZE;
		
		    void* wantBlock = ptr;
		    void* remBlock = ptr  + wantedPayLoad + DSIZE;
			updateOH(ptr,size);
			updateOH(remBlock,remSize);
            place(ptr,size);
			addToFreeList(remBlock);
			return ptr;
		}
		//if we cant split then just return oldptr
		else if (asize < oldSize && !((oldSize-(asize))>(4*WSIZE)))
				return oldptr;
		else {
			size = getAdjustedSize(size);
			void* next_block = NEXT_BLKP(ptr);

			int totalSize = GET_SIZE(HDRP(ptr)) + GET_SIZE(HDRP(next_block));

			//if block is free
			if(!GET_ALLOC(HDRP(next_block)))
			{

				if (totalSize >= size){
					//coaleasce with next block only
					
					removeFromFreeList(NEXT_BLKP(ptr));
					PUT(HDRP(ptr), PACK( totalSize, 0));
					PUT(FTRP(ptr), PACK( totalSize, 0));

					if(totalSize - size > 4*WSIZE){
					//split if possible
						 size_t remSize =  totalSize - size;
		                 size_t wantedPayLoad = size - DSIZE;
		
		                 void* wantBlock = ptr;
		                 void* remBlock =ptr  + wantedPayLoad + DSIZE;
						 updateOH(ptr,size);
						 updateOH(remBlock,remSize);
                         place(ptr,size);
					     addToFreeList(remBlock);
						 return wantBlock;
					}
				  else {
				  //could not split so just return the block
					  place(ptr,size);
					  return ptr;
				  }
          
               }


			}
		}
		
		// coalescing does not give enough size, so need to memcpy instead

           newptr = mm_malloc(size);
			if (newptr ==NULL)
				return NULL;
			oldSize = GET_SIZE(HDRP(oldptr));
			if(size < oldSize)
				oldSize=size;
			memcpy(newptr, oldptr, oldSize);
            mm_free(oldptr);
			return newptr;
    }

/**********************************************************
 * mm_check
 * Check the consistency of the memory heap
 * Return nonzero if the heap is consistant.
 *********************************************************/
 
/****** Heap checker helper header****************/
int isInFreeList(void* bp, int currIndex);

int mm_check()
{

	//Is every block in the free list marked as free?

	int currIndex;

	for(currIndex=0;currIndex< NUM_BINS; currIndex++)
	{
		
		void* baseFromIndex = HeapStart + currIndex*WSIZE;
		void* head = GET(baseFromIndex);

		while(head)
		{
			if (GET_ALLOC(HDRP(head))!=0)
			{
				return -1;
			}

			size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(head)));
			size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(head)));
			
			if(prev_alloc==0 || next_alloc==0)
			{	
				//escaped coalescing
				printf("Escaped coalescing?\n");
				return -1;
			}
            
            if((head < HeapStart) || (head > (HeapStart+HeapSize)))
            {
                //Not a valid heap address
                printf("Not a valid heap address?\n");
                return -1;
            }
			head = GET(head);
		}
	}

	//Is every free block actually in the free list?
	void* memStart = HeapStart + (NUM_BINS+1)*WSIZE;
	while(memStart)
	{
        size_t alloc = GET_ALLOC(HDRP(memStart));
        if(!alloc)
		{
            currIndex = getIndex(memStart);
            if( isInFreeList(memStart,currIndex)==-1)
		    {
		    	printf("Every flee block in free_list?\n");
			    return -1;	
		    }
        }
		memStart += GET_SIZE(HDRP(memStart));
	}

    return 0;
}
int isInFreeList(void* bp,int currIndex)
{
	
	void* baseFromIndex = HeapStart + currIndex*WSIZE;
	void* head = GET(baseFromIndex);
		while(head)
		{
			if(bp==head)
				return 1;
			head = GET(head);
		}	
		
	return -1;
}
