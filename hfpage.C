#include <iostream>
#include <stdlib.h>
#include <memory.h>

#include "hfpage.h"
#include "heapfile.h"
#include "buf.h"
#include "db.h"


// **********************************************************
// page class constructor

void HFPage::init(PageId pageNo)
{
    this->slotCnt = 0;      //已使用的槽数量，注意是倒着数的
    this->curPage = pageNo;     //当前页的页码,由形参初始化
    this->prevPage = INVALID_PAGE;      //前向指针,前一页页码,默认为-1
    this->nextPage = INVALID_PAGE;      //后向指针,后一页页码,默认为-1
    this->usedPtr = MAX_SPACE - DPFIXED;        //data[]中第一个使用的字节的偏移量
    this->freeSpace = MAX_SPACE - DPFIXED + sizeof(slot_t);     //data[]里一共有多少空闲空间
    this->slot[0].length = EMPTY_SLOT;
    this->slot[0].offset = 0;
}

// **********************************************************
// dump page utlity
void HFPage::dumpPage()
{
    int i;

    cout << "dumpPage, this: " << this << endl;
    cout << "curPage= " << curPage << ", nextPage=" << nextPage << endl;
    cout << "usedPtr=" << usedPtr << ",  freeSpace=" << freeSpace
         << ", slotCnt=" << slotCnt << endl;
   
    for (i=0; i < slotCnt; i++) {
        cout << "slot["<< i <<"].offset=" << slot[i].offset
             << ", slot["<< i << "].length=" << slot[i].length << endl;
    }
}

// **********************************************************
PageId HFPage::getPrevPage()
{
    // fill in the body
    return this->prevPage;      //直接返回prevPage参数
}

// **********************************************************
void HFPage::setPrevPage(PageId pageNo)
{
    // fill in the body
    this->prevPage = pageNo;        //为prevPage赋值
}

// **********************************************************
void HFPage::setNextPage(PageId pageNo)
{
    // fill in the body
    this->nextPage = pageNo;        //为nextPage赋值
}

// **********************************************************
PageId HFPage::getNextPage()
{
    // fill in the body
    return this->nextPage;      //直接返回prevPage参数
}

// **********************************************************
// Add a new record to the page. Returns OK if everything went OK
// otherwise, returns DONE if sufficient space does not exist
// RID of the new record is returned via rid parameter.
// 接收要写的record的指针和长度，将写进page里的record对应的rid写进参数返回
Status HFPage::insertRecord(char* recPtr, int recLen, RID& rid)
{
    // 可用空间小于该记录，返回done
    if (HFPage::available_space() < recLen) {
        return DONE;
    }

    //从头开始查找，i_slot定位到第一个空槽，若定位到末尾的位置，则已用slotCnt加一
    int i_slot = 0;
    for (i_slot = 0; i_slot < this->slotCnt; i_slot++) {
        if (slot[i_slot].length == EMPTY_SLOT)
            break;
    } 
    if (i_slot == slotCnt) slotCnt++;

    //slot倒着生长，
    usedPtr = usedPtr - recLen;
    slot[i_slot].offset = usedPtr;
    slot[i_slot].length = recLen;

    memcpy(data + usedPtr, recPtr, recLen);

    freeSpace -= recLen;

    rid.pageNo = curPage;
    rid.slotNo = i_slot;

    return OK;
}

// **********************************************************
// Delete a record from a page. Returns OK if everything went okay.
// Compacts remaining records but leaves a hole in the slot array.
// Use memmove() rather than memcpy() as space may overlap.
Status HFPage::deleteRecord(const RID& rid)
{
    // fill in the body
    if (slotCnt == 0 || rid.slotNo < 0 || rid.slotNo >= slotCnt || slot[rid.slotNo].length == EMPTY_SLOT)
        return FAIL;

    int offset = slot[rid.slotNo].offset;
    int length = slot[rid.slotNo].length;
    memmove(data + usedPtr + length, data + usedPtr, offset - usedPtr);
    usedPtr += length;

    slot[rid.slotNo].length = EMPTY_SLOT;
    for (int i = slotCnt - 1; i >= 0; i--) {
        if (slot[i].length != EMPTY_SLOT) {
            break;
        }
        slotCnt--;
    }

    if (rid.slotNo < slotCnt) {
        for (int i = rid.slotNo + 1; i < slotCnt; i++) {
            slot[i].offset += length;
        }
    }

    freeSpace += length;

    return OK;
}

// **********************************************************
// returns RID of first record on page
Status HFPage::firstRecord(RID& firstRid)
{
    // fill in the body
    if (empty()) {
        return DONE;
    }

    if (slotCnt == 0)
        return FAIL;

    bool hasRecord = false;

    for (int i = 0; i < slotCnt; i++) {
        if (slot[i].length != EMPTY_SLOT) {
            firstRid.slotNo = i;
            firstRid.pageNo = curPage;
            hasRecord = true;
            break;
        }
    }

    if (!hasRecord) {
        return DONE;
    }

    return OK;
}

// **********************************************************
// returns RID of next record on the page
// returns DONE if no more records exist on the page; otherwise OK
Status HFPage::nextRecord (RID curRid, RID& nextRid)
{
    if (curRid.pageNo != curPage) {
        return FAIL;
    }

    if (empty()) {
        return FAIL;
    }

    bool foundNext = false;

    for (int i = curRid.slotNo + 1; i < slotCnt; i++) {
        if (slot[i].length != EMPTY_SLOT) {
            nextRid.slotNo = i;
            nextRid.pageNo = curPage;
            foundNext = true;
            break;
        }
    }

    if (foundNext) {
        return OK;
    }

    return DONE;
}

// **********************************************************
// returns length and copies out record with RID rid
Status HFPage::getRecord(RID rid, char* recPtr, int& recLen)
{
    // fill in the body
    if (rid.pageNo != curPage || slotCnt == 0 || rid.slotNo < 0 || rid.slotNo >= slotCnt || slot[rid.slotNo].length == EMPTY_SLOT) {
        return FAIL;
    }

    for (int i = 0; i < slot[rid.slotNo].length; i++) {
        *recPtr = data[slot[rid.slotNo].offset + i];
        recPtr++;
    }

    recLen = slot[rid.slotNo].length;

    return OK;
}

// **********************************************************
// returns length and pointer to record with RID rid.  The difference
// between this and getRecord is that getRecord copies out the record
// into recPtr, while this function returns a pointer to the record
// in recPtr.
Status HFPage::returnRecord(RID rid, char*& recPtr, int& recLen)
{
    if (rid.pageNo != curPage || slotCnt == 0 || rid.slotNo < 0 || rid.slotNo >= slotCnt || slot[rid.slotNo].length == EMPTY_SLOT) {
        return FAIL;
    }

    recPtr = &data[this->slot[rid.slotNo].offset];
    recLen = slot[rid.slotNo].length;

    return OK;
}

// **********************************************************
// Returns the amount of available space on the heap file page
int HFPage::available_space(void)
{
    if (slotCnt == 0) {
        return freeSpace - sizeof(slot_t);
    }
    return freeSpace - slotCnt * sizeof(slot_t);
}

// **********************************************************
// Returns 1 if the HFPage is empty, and 0 otherwise.
// It scans the slot directory looking for a non-empty slot.
bool HFPage::empty(void)
{
    for (int i = 0; i < this->slotCnt; i++) {
        if (slot[i].length != EMPTY_SLOT) {
            return 0;
        }
    }
    return 1;
}



