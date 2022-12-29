///////////////////////////////////////////////////////////////////////////////
/////////////  The Header File for the Buffer Manager /////////////////////////
///////////////////////////////////////////////////////////////////////////////


#ifndef BUF_H
#define BUF_H

#include "db.h"
#include "page.h"
#include "new_error.h"

#define NUMBUF 20   
// Default number of frames, artifically small number for ease of debugging.

#define HTSIZE 7
// Hash Table size

//You should define the necessary classes and data structures for the hash table, and the queues for LSR, MRU, etc.
class   BufMgr;

// *****************************************************
class Frame_Descriptor {

    friend class BufMgr;

private:
    int    page_number;     // the page within file, or INVALID_PAGE if
    // the frame is empty.

    unsigned int pin_count;  // The pin count for the page in this frame

    int dirty;

    int hate;

    Frame_Descriptor() {
        page_number  = INVALID_PAGE;
        pin_count = 0;
        dirty = FALSE;
        hate=0;
    }

    ~Frame_Descriptor() {}

public:


    int total_pin_count() { return(pin_count); }
    int add_pin() { return(++pin_count); }
    void set_pin_zero(){pin_count = 0;}
    int remove_pin() {
        pin_count = (pin_count <= 0) ? 0 : pin_count - 1;
        return(pin_count);
    }
};

class hash_table
{
    friend class BufMgr;
private:



public:
    struct hash_pair
    {
        int page_number;
        int frameNo;
    };

    hash_pair pair[100];

    hash_table(){
        for (int i = 0; i < 100; i++)
        {
            pair[i].page_number = INVALID_PAGE;
            pair[i].frameNo = -1;
            /* code */
        }
    };
    ~hash_table(){

    };

};

//*******************ALL BELOW are purely local to buffer Manager********//
// class for maintaining information about buffer pool frame
// You should create enums for internal errors in the buffer manager.
enum bufErrCodes  {
    HASHTBLERROR,
    HASHNOTFOUND,
    BUFFEREXCEEDED,
    PAGENOTPINNED,
    BADBUFFER,
    PAGEPINNED,
    REPLACERERROR,
    BADBUFFRAMENO,
    PAGENOTFOUND,
    FRAMEEMPTY,
    HASHMEMORY,
    HASHDUPLICATEINSERT,
    HASHREMOVEERROR,
    QMEMORYERROR,
    QEMPTY,
    INTERNALERROR,
    BUFFERFULL,
    BUFMGRMEMORYERROR,
    BUFFERPAGENOTFOUND,
    BUFFERPAGENOTPINNED,
    BUFFERPAGEPINNED
};

class Replacer;

class BufMgr {

private: // fill in this area
    unsigned int    numBuffers;

    // fill in this area
    Frame_Descriptor      *frmeTable;      // An array of Descriptors one per frame.

    hash_table *hashTable;

    int *hate_frame;

    int h(PageId PageId_in_a_DB){
        return (PageId_in_a_DB%HTSIZE);
    };

public:
    Page* bufPool; // The actual buffer pool

    BufMgr (int numbuf, Replacer *replacer = 0); 
    // Initializes a buffer manager managing "numbuf" buffers.
	// Disregard the "replacer" parameter for now. In the full 
  	// implementation of minibase, it is a pointer to an object
	// representing one of several buffer pool replacement schemes.

    ~BufMgr();           // Flush all valid dirty pages to disk

    Status pinPage(PageId PageId_in_a_DB, Page*& page, int emptyPage=0);
        // Check if this page is in buffer pool, otherwise
        // find a frame for this page, read in and pin it.
        // also write out the old page if it's dirty before reading
        // if emptyPage==TRUE, then actually no read is done to bring
        // the page

    Status unpinPage(PageId globalPageId_in_a_DB, int dirty, int hate);
        // hate should be TRUE if the page is hated and FALSE otherwise
        // if pincount>0, decrement it and if it becomes zero,
        // put it in a group of replacement candidates.
        // if pincount=0 before this call, return error.

    Status newPage(PageId& firstPageId, Page*& firstpage, int howmany=1); 
        // call DB object to allocate a run of new pages and 
        // find a frame in the buffer pool for the first page
        // and pin it. If buffer is full, ask DB to deallocate 
        // all these pages and return error

    Status freePage(PageId globalPageId); 
        // user should call this method if it needs to delete a page
        // this routine will call DB to deallocate the page 

    Status flushPage(PageId pageid);
        // Used to flush a particular page of the buffer pool to disk
        // Should call the write_page method of the DB class

    Status flushAllPages();
	// Flush all pages of the buffer pool to disk, as per flushPage.

    /* DO NOT REMOVE THIS METHOD */    
    Status unpinPage(PageId globalPageId_in_a_DB, int dirty=FALSE)
        //for backward compatibility with the libraries
    {
      return unpinPage(globalPageId_in_a_DB, dirty, FALSE);
    }

    Status pinPage(PageId PageId_in_a_DB, Page*& page, int emptyPage, const char *filename);
    // Should be equivalent to the above pinPage()
    // Necessary for backward compatibility with project 1

    Status unpinPage(PageId globalPageId_in_a_DB, int dirty, const char *filename);
    // Should be equivalent to the above unpinPage()
    // Necessary for backward compatibility with project 1

    unsigned int getNumBuffers() const { return numBuffers; }
    // Get number of buffers

    unsigned int getCountUnpinnedBuffers();
    // Get number of unpinned buffers
};

#endif
