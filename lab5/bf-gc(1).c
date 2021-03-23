// ==============================================================================
/**
 * bf-gc.c
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

#include "gc.h"
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

  /** Whether the block has been visited during reachability analysis. */
  bool           marked;

  /** A map of the layout of pointers in the object. */
  gc_layout_s*   layout;

} header_s;

/** A link in a linked stack of pointers, used during heap traversal. */
typedef struct ptr_link {

  /** The next link in the stack. */
  struct ptr_link* next;

  /** The pointer itself. */
  void* ptr;

} ptr_link_s;
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

/** The head of the allocated list. */
static header_s* allocated_list_head = NULL;

/** The head of the root set stack. */
static ptr_link_s* root_set_head = NULL;


// ==============================================================================



// ==============================================================================
/**
 * Push a pointer onto root set stack.
 *
 * \param ptr The pointer to be pushed.
 */
void rs_push (void* ptr) {

  // Make a new link.
  ptr_link_s* link = malloc(sizeof(ptr_link_s));
  if (link == NULL) {
    ERROR("rs_push(): Failed to allocate link");
  }

  // Have it store the pointer and insert it at the front.
  link->ptr    = ptr;
  link->next   = root_set_head;
  root_set_head = link;
  
} // rs_push ()
// ==============================================================================



// ==============================================================================
/**
 * Pop a pointer from the root set stack.
 *
 * \return The top pointer being removed, if the stack is non-empty;
 *         <code>NULL</code>, otherwise.
 */
void* rs_pop () {

  // Grab the pointer from the link...if there is one.
  if (root_set_head == NULL) {
    return NULL;
  }
  void* ptr = root_set_head->ptr;

  // Remove and free the link.
  ptr_link_s* old_head = root_set_head;
  root_set_head = root_set_head->next;
  free(old_head);

  return ptr;
  
} // rs_pop ()
// ==============================================================================



// ==============================================================================
/**
 * Add a pointer to the _root set_, which are the starting points of the garbage
 * collection heap traversal.  *Only add pointers to objects that will be live
 * at the time of collection.*
 *
 * \param ptr A pointer to be added to the _root set_ of pointers.
 */
void gc_root_set_insert (void* ptr) {

  rs_push(ptr);
  
} // root_set_insert ()
// ==============================================================================



// ==============================================================================
/**
 * The initialization method.  If this is the first use of the heap, initialize it.
 */

void gc_init () {

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

} // gc_init ()
// ==============================================================================


// ==============================================================================
// COPY-AND-PASTE YOUR PROJECT-4 malloc() HERE.
//
//   Note that you may have to adapt small things.  For example, the `init()`
//   function is now `gc_init()` (above); the header is a little bit different
//   from the Project-4 one; my `allocated_list_head` may be a slightly
//   different name than the one you used.  Check the details.
void* gc_malloc (size_t size) {
  gc_init();
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

}

 // gc_malloc ()
// ==============================================================================



// ==============================================================================
// COPY-AND-PASTE YOUR PROJECT-4 free() HERE.
//
//   See above.  Small details may have changed, but the code should largely be
//   unchanged.
void gc_free (void* ptr) {

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

  
} // gc_free ()
// ==============================================================================



// ==============================================================================
/**
 * Allocate and return heap space for the structure defined by the given
 * `layout`.
 *
 * \param layout A descriptor of the fields
 * \return A pointer to the allocated block, if successful; `NULL` if unsuccessful.
 */

void* gc_new (gc_layout_s* layout) {

  // Get a block large enough for the requested layout.
  void*     block_ptr  = gc_malloc(layout->size);
  header_s* header_ptr = BLOCK_TO_HEADER(block_ptr);

  // Hold onto the layout for later, when a collection occurs.
  header_ptr->layout = layout;
  
  return block_ptr;
  
} // gc_new ()
// ==============================================================================



// ==============================================================================
/**
 * Traverse the heap, marking all live objects.
 */

void mark () {
  while(root_set_head!=NULL){
    void *ptr = rs_pop();
    if(ptr==NULL){

      continue;

    }
    header_s* cur = BLOCK_TO_HEADER(ptr);

     if(cur->marked == true ){
       continue;
     }
     cur->marked = true;
     gc_layout_s* layout = cur->layout;
     void **new_p;
     void **new_pp;
     //void **block = HEADER_TO_BLOCK(cur);
     for(int i = 0; i < layout->num_ptrs; i++){
       new_p = ptr +layout->ptr_offsets[i];
       new_pp = *new_p;
       rs_push(new_pp);
       //rs_push(*(block + layout->ptr_offsets[i]));
       }  
  }
}
 
 // mark ()
// ==============================================================================



// ==============================================================================
/**
 * Traverse the allocated list of objects.  Free each unmarked object;
 * unmark each marked object (preparing it for the next sweep.
 */

void sweep () {


  header_s* cur = allocated_list_head;
  while(cur!=NULL){
    header_s *nxt = cur->next;
    if(cur->marked == false){
      gc_free(HEADER_TO_BLOCK(cur));
    }
    else{
      cur->marked=false;
    }
    cur = nxt;
   
}
  

} // sweep ()
// ==============================================================================



// ==============================================================================
/**
 * Garbage collect the heap.  Traverse and _mark_ live objects based on the
 * _root set_ passed, and then _sweep_ the unmarked, dead objects onto the free
 * list.  This function empties the _root set_.
 */

void gc () {

  // Traverse the heap, marking the objects visited as live.
  mark();

  // And then sweep the dead objects away.
  sweep();

  // Sanity check:  The root set should be empty now.
  assert(root_set_head == NULL);
  
} // gc ()
// ==============================================================================
