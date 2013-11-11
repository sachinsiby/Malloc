/*
 * This implementation replicates the implicit list implementation
 * provided in the textbook
 * "Computer Systems - A Programmer's Perspective"
 * Blocks are never coalesced or reused.
 * Realloc is implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
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

#define NUM_BINS 66

void* heap_listp = NULL;
void* HeapStart = NULL;
void* MemStart = NULL;

size_t HeapSize = 0;

void* extend_heap_init(size_t);

void printSegList()
{
   int i;
   for(i =0; i< NUM_BINS; i++)
   {
       int label = (i+1)*16;
       void* binPtr = HeapStart + WSIZE*i;
       printf("(%p) %d->",binPtr,label);
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
       printf("NULL\n");
   }
}



/**********************************************************
 * mm_init
 * Initialize the heap.
 * Stores address to different segregations.
 * Extends heap by NUM_BINS
 **********************************************************/
 int mm_init(void)
 {
	// ADD COMMENTS HERE
	int i;	
	
	HeapStart = extend_heap_init((size_t)NUM_BINS);
	if(!HeapStart)
	{
		return -1;
	}
	
	for(i=0; i<NUM_BINS; i++)
	{
		void* binPtr = HeapStart + WSIZE*i;
		PUT(binPtr,NULL);
	}

	MemStart = HeapStart + WSIZE*NUM_BINS;
	return 0;
 }


 int testmm_init()
 {
	
	int i = 0;
	printf("Initial Heapsize is %zu\n",HeapSize);
	for (i=0; i < NUM_BINS; i++)
	{
		void* binPtr = HeapStart + WSIZE* i;		
	if(GET(binPtr))
		{
			printf("Error initializing %d\n",i);
		}
		else
		{
			printf("Correct initializing %d\n",i);
		}
	
	}
	printf("Final Heapsize is %zu\n",HeapSize);
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

    else if (prev_alloc && !next_alloc) { /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        return (bp);
    }

    else if (!prev_alloc && next_alloc) { /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        return (PREV_BLKP(bp));
    }

    else {            /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)))  +
            GET_SIZE(FTRP(NEXT_BLKP(bp)))  ;
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));
        return (PREV_BLKP(bp));
    }
}

void *extend_heap_init(size_t words) {
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignments */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
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

    bp = bp + WSIZE;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));                // free block header
    PUT(FTRP(bp), PACK(size, 0));                // free block footer
    //PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));        // new epilogue header
	
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

int getIndex(size_t adjustedSize)
{
    //Finds index in global segregated list
    if (adjustedSize/DSIZE < NUM_BINS)
    {
        return adjustedSize/DSIZE - 1;
    } 

    // If it's in the last bin, it can be any size > NUM_BINS*16

    return NUM_BINS-1;
}
/**********************************************************
 * mm_free
 * Free the block and coalesce with neighbouring blocks
 **********************************************************/
void mm_free(void *blockPointer)
{
    if(blockPointer == NULL){
      return;
    }

    size_t adjustedSize = GET_SIZE(HDRP(blockPointer));
    PUT(HDRP(blockPointer), PACK(adjustedSize,0));
    PUT(FTRP(blockPointer), PACK(adjustedSize,0));
    //coalesce(blockPointer);

    printf("Size of block is %zu\n",adjustedSize);
    int currIndex = getIndex(adjustedSize);
    void* baseFromIndex = HeapStart + currIndex*WSIZE;
    printf("location is %p\n",baseFromIndex);
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


void testFree()
{
    
    int* a = (int*)mm_malloc(sizeof(int*));
    int* b = (int*)mm_malloc(sizeof(int*));

    void* c = mm_malloc(128);

    mm_free(a);
    printSegList();
    printf("Freeed a\n");

    mm_free(b);
    printSegList();
    printf("Freed b\n");

    mm_free(c);
    printSegList();
    printf("Adjusted size of 128 is %zu\n",getAdjustedSize(128));
    printf("Freed c\n");

    printf("Index of 128 is %d\n",getIndex(128));

}




/**********************************************************
 * mm_malloc
 * Allocate a block of size bytes.
 * The type of search is determined by find_fit
 * The decision of splitting the block, or not is determined
 *   in place(..)
 * If no block satisfies the request, the heap is extended
 **********************************************************/
void *getBestFit(void* baseOfIndex,size_t adjustedSize,int currIndex);
void *extendHeapAndAlloc(size_t adjustedSize);
void markAssigned(void* assignedBlock, size_t adjustedSize, int currIndex);
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
        place(assignedBlock, adjustedSize);
        return assignedBlock;
	}

    currIndex--;

	markAssigned(assignedBlock, adjustedSize,currIndex);
    return assignedBlock;
}

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
		printf("extend heap failed\n");
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
		printf("extend heap failed\n");
		return;
	}
    
    printSegList();

    char* test = mm_malloc(mallocRequest);
    printf("Size of allocated chunk is %zu\n",GET_SIZE(HDRP(test)));
    printSegList();
}



 /* getBestFit
 *
 *
 **********************************************************/
void *getBestFit(void* baseOfIndex,size_t adjustedSize,int currIndex)
{


	if (currIndex < NUM_BINS-1)
	{
		//Returns head of LinkedList 
        return GET(baseOfIndex);
	    
    }
	else
	{
		//Find Best fit in the last bin
		void *currentNode = GET(baseOfIndex);
		void *bestNode = NULL;
		size_t minSize = 0;		

		while(currentNode)
		{
			if((adjustedSize < GET_SIZE(HDRP(currentNode))) && ((GET_SIZE(HDRP(currentNode)) < minSize)||!bestNode))
			{
				minSize = GET_SIZE(HDRP(currentNode));
				bestNode = currentNode;
			}
			
			//Gets next node
			currentNode = GET(currentNode+WSIZE);
		}
		return bestNode;
	}
} 

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
		printf("extend heap failed\n");
		return;
	}

    //testIndex = getBestFit(baseOfIndex,adjustedSize2,currIndex);
    
    printSegList();

    if(testIndex == bp)
    {
        //markAssigned(bp,adjustedSize2);
    }
    else if(testIndex == bp2)
    {
       // markAssigned(testIndex,adjustedSize2,currIndex);
    }
    else
    {
        printf("Failure , test is %zu, givenP is %zu \n",testIndex,bp);
    }
    printf("____________________________________\n");

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

/*******************************************
 *markAssigned
 *Sets block as assigned in Header and Footer
 *Removes block from appropriate free list
***********************************************/
void markAssigned(void* assignedBlock,size_t adjustedSize,int currIndex)
{
    place(assignedBlock,adjustedSize);

    void* prev = GET(assignedBlock);
    void* next = GET(assignedBlock+WSIZE);

    if(prev!=NULL)
    {
        GET(prev+WSIZE) = next;
    }
    
    if(next!=NULL)
    {
        GET(next) = prev;
    }
    
    //If assigned Node is a head, we have to have change value of head in global segregated list
    if(prev==NULL)
    {
        void* baseOfIndex = currIndex*WSIZE + HeapStart;
        PUT(baseOfIndex,next);
    }

}



/**********************************************************
 * mm_realloc
 * Implemented simply in terms of mm_malloc and mm_free
 *********************************************************/
void *mm_realloc(void *ptr, size_t size)
{
    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0){
      mm_free(ptr);
      return NULL;
    }
    /* If oldptr is NULL, then this is just malloc. */
    if (ptr == NULL)
      return (mm_malloc(size));

    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;

    /* Copy the old data. */
    copySize = GET_SIZE(HDRP(oldptr));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

/**********************************************************
 * mm_check
 * Check the consistency of the memory heap
 * Return nonzero if the heap is consistant.
 *********************************************************/
int mm_check(void){
  return 1;
}
