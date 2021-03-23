// ==============================================================================
/**
 * bf-alloc.c
 *
 * A _best-fit_ heap allocator.  This allocator uses a _doubly-linked free list_
 * from which to allocate the best fitting free block.  If the list does not
 * contain any blocks of sufficient size, it uses _pointer bumping_ to expand
 * the heap.
 **/
// ==============================================================================



// ==============================================================================
// INCLUDES

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include "safeio.h"
// ==============================================================================



// ==============================================================================
// TYPES AND STRUCTURES

/** The header for each allocated object. */
typedef struct header {

  /** Pointer to the next header in the list. */
  struct header* next;

  /** Pointer to the previous header in the list. */
  struct header* prev;

  /** The usable size of the block (exclusive of the header itself). */
  size_t         size;

  /** Is the block allocated or free? */
  bool           allocated;

} header_s;
// ==============================================================================



// ==============================================================================
// MACRO CONSTANTS AND FUNCTIONS

/** The system's page size. */
#define PAGE_SIZE sysconf(_SC_PAGESIZE)

/**
 * Macros to easily calculate the number of bytes for larger scales (e.g., kilo,
 * mega, gigabytes).
 */
#define KB(size)  ((size_t)size * 1024)
#define MB(size)  (KB(size) * 1024)
#define GB(size)  (MB(size) * 1024)

/** The virtual address space reserved for the heap. */
#define HEAP_SIZE GB(2)

/** Given a pointer to a header, obtain a `void*` pointer to the block itself. */
#define HEADER_TO_BLOCK(hp) ((void*)((intptr_t)hp + sizeof(header_s)))

/** Given a pointer to a block, obtain a `header_s*` pointer to its header. */
#define BLOCK_TO_HEADER(bp) ((header_s*)((intptr_t)bp - sizeof(header_s)))
// ==============================================================================


// ==============================================================================
// GLOBALS

/** The address of the next available byte in the heap region. */
static intptr_t free_addr  = 0;

/** The beginning of the heap. */
static intptr_t start_addr = 0;

/** The end of the heap. */
static intptr_t end_addr   = 0;

/** The head of the free list. */
static header_s* free_list_head = NULL;

static header_s* allocated_list_head = NULL;

/**
// ==============================================================================



// ==============================================================================
/*
  The initialization method.  If this is the first use of the heap, initialize it.
 */

void init () {

  // Only do anything if there is no heap region (i.e., first time called).
  if (start_addr == 0) {

    DEBUG("Trying to initialize");
    
    // Allocate virtual address space in which the heap will reside. Make it
    // un-shared and not backed by any file (_anonymous_ space).  A failure to
    // map this space is fatal.
    void* heap = mmap(NULL,
		      HEAP_SIZE,
		      PROT_READ | PROT_WRITE,
		      MAP_PRIVATE | MAP_ANONYMOUS,
		      -1,
		      0);
    if (heap == MAP_FAILED) {
      ERROR("Could not mmap() heap region");
    }

    // Hold onto the boundaries of the heap as a whole.
    start_addr = (intptr_t)heap;
    end_addr   = start_addr + HEAP_SIZE;
    free_addr  = start_addr;

    // DEBUG: Emit a message to indicate that this allocator is being called.
    DEBUG("bf-alloc initialized");

  }

} // init ()
// ==============================================================================


// ==============================================================================
/**
 * Allocate and return `size` bytes of heap space.  Specifically, search the
 * free list, choosing the _best fit_.  If no such block is available, expand
 * into the heap region via _pointer bumping_.
 *
 * \param size The number of bytes to allocate.
 * \return A pointer to the allocated block, if successful; `NULL` if unsuccessful.
 */
void* malloc (size_t size) {
  //initialize the heap
  init();
  //Special case: if the number of bytes to allocate is 0, return NULL. 
  if (size == 0) {
    return NULL;
  }

  //initialize the current ponter, and the best one
  header_s* current = free_list_head;
  header_s* best    = NULL;

  //while we have non-null block, chech for the best one
  while (current != NULL) {
    // if current block is allocated return error
    if (current->allocated) {
      ERROR("Allocated block on free list", (intptr_t)current);
    }
    //if the best one so far is null, and the size is smaller or equal than current size, or if best one so far is not null, and the size is smaller or equal than the size of current block, and the size of the current is smaller or equal than the best size, make best = current
    if ( (best == NULL && size <= current->size) ||
	 (best != NULL && size <= current->size && current->size < best->size) ) {
      best = current;
    }
    // if we have the best block and it's size is equal to the size of block, break
    if (best != NULL && best->size == size) {
      break;
    }
    // move the pointer to the next block
    current = current->next;
    
  }
  // initialize the new block pointer
  void* new_block_ptr = NULL;
  // if the best one is not null
  if (best != NULL) {

    // if the previous to best block is null, it means best is actually the head of the list
    if (best->prev == NULL) {
      free_list_head   = best->next;
    } else {
      // point the previous header in the list to the next header
      best->prev->next = best->next;
    }
    //is bext header is not null, point the previous pointerr of the next heder to the previous header
    if (best->next != NULL) {
      best->next->prev = best->prev;
    }

    // deatch the best from any blocks, so no other block has pointer to it
    best->prev = NULL;
    best->next = NULL;
    // indicate that the best is already allocated
    best->allocated = true;
    // get the ponter to the new block, from the header
    new_block_ptr   = HEADER_TO_BLOCK(best);
    
  } else {
    //double word allignment
    size_t res = (32 - (size_t)free_addr % 16 - sizeof(header_s) % 16) % 16;
    free_addr += res;
    // make the header pointer point to the free block address
    header_s* header_ptr = (header_s*)free_addr;
    // get the ponter to the new block, from the header
    new_block_ptr = HEADER_TO_BLOCK(header_ptr);

    // detach header_ptr from other blocks, so that there no other pointers to it
    header_ptr->next      = NULL;
    header_ptr->prev      = NULL;
   
    // equate the size of the header pointed block to the size
    header_ptr->size      = size;
     // indicate that the best is already allocated
    header_ptr->allocated = true;
    //increase the size of the block
    intptr_t new_free_addr = (intptr_t)new_block_ptr + size;
    // if the new increased address is bigger than the set end address
    if (new_free_addr > end_addr) {
      //double word allignment
      free_addr -= res;
      
      return NULL;

    } else {
      // make the size of free address be equal to the new increased address
      free_addr = new_free_addr;

    }

  }

  // adding to allocated list
  header_s* header_ptr = BLOCK_TO_HEADER(new_block_ptr);

  //allocated_list_head
  header_ptr->next = allocated_list_head;
  //if the header of allocated list is not null, i.e it it is pointing to some block
  if(allocated_list_head != NULL){
    //make header point to the allocated list header
    allocated_list_head->prev = header_ptr;
  }
  allocated_list_head = header_ptr;

  return new_block_ptr;

} // malloc()
// ==============================================================================



// ==============================================================================
/**
 * Deallocate a given block on the heap.  Add the given block (if any) to the
 * free list.
 *
 * \param ptr A pointer to the block to be deallocated.
 */
void free (void* ptr) {
  // special case if we have to free up the no-memory space
  if (ptr == NULL) {
    return;
  }

  // get the pointer to the block from its header
  header_s* header_ptr = BLOCK_TO_HEADER(ptr);
  // if the block we get isn't allocated return error
  if (!header_ptr->allocated) {
    ERROR("Double-free: ", (intptr_t)header_ptr);
  }
  //if there is a block after our current one
  if(header_ptr->next != NULL){
    // make the next block previous pointer point to the block before our current one
    header_ptr->next->prev = header_ptr->prev;   
  }
  // if there exist a block before our current one
  if(header_ptr->prev != NULL){
    // make the next pointer of the previous block, point to the next block after our current one
     header_ptr->prev->next = header_ptr->next;
  }
  else{
    // make the next block the head of the allocated list
    allocated_list_head = header_ptr->next; 
  }

  // make the header point to the next block, which is free_list_head
  header_ptr->next = free_list_head;
  // make the free_list_head the new header
  free_list_head   = header_ptr;
  // make the header_ptr previous pointer equal to null (because it's the header) 
  header_ptr->prev = NULL;
  // if the next block after the header is not null
  if (header_ptr->next != NULL) {
    // make the next block's previous pointer point to the header
    header_ptr->next->prev = header_ptr;
  }
  // indicate that the block is not allocated (i.e freed)
  header_ptr->allocated = false;

} // free()
// ==============================================================================



// ==============================================================================
/**
 * Allocate a block of `nmemb * size` bytes on the heap, zeroing its contents.
 *
 * \param nmemb The number of elements in the new block.
 * \param size  The size, in bytes, of each of the `nmemb` elements.
 * \return      A pointer to the newly allocated and zeroed block, if successful;
 *              `NULL` if unsuccessful.
 */
void* calloc (size_t nmemb, size_t size) {

  // Allocate a block of the requested size.
  size_t block_size    = nmemb * size;
  void*  new_block_ptr = malloc(block_size);

  // If the allocation succeeded, clear the entire block.
  if (new_block_ptr != NULL) {
    memset(new_block_ptr, 0, block_size);
  }

  return new_block_ptr;
  
} // calloc ()
// ==============================================================================



// ==============================================================================
/**
 * Update the given block at `ptr` to take on the given `size`.  Here, if `size`
 * fits within the given block, then the block is returned unchanged.  If the
 * `size` is an increase for the block, then a new and larger block is
 * allocated, and the data from the old block is copied, the old block freed,
 * and the new block returned.
 *
 * \param ptr  The block to be assigned a new size.
 * \param size The new size that the block should assume.
 * \return     A pointer to the resultant block, which may be `ptr` itself, or
 *             may be a newly allocated block.
 */
void* realloc (void* ptr, size_t size) {

  // Special case: If there is no original block, then just allocate the new one
  // of the given size.
  if (ptr == NULL) {
    return malloc(size);
  }

  // Special case: If the new size is 0, that's tantamount to freeing the block.
  if (size == 0) {
    free(ptr);
    return NULL;
  }

  // Get the current block size from its header.
  header_s* header_ptr = BLOCK_TO_HEADER(ptr);

  // If the new size isn't an increase, then just return the original block as-is.
  if (size <= header_ptr->size) {
    return ptr;
  }

  // The new size is an increase.  Allocate the new, larger block, copy the
  // contents of the old into it, and free the old.
  void* new_block_ptr = malloc(size);
  if (new_block_ptr != NULL) {
    memcpy(new_block_ptr, ptr, header_ptr->size);
    free(ptr);
  }
    
  return new_block_ptr;
  
} // realloc()
// ==============================================================================
