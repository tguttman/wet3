//
// Created by tomer on 17/01/2025.
//

#include <stdio.h>
#include <iostream>
#include "customAllocator.h"

#include <string.h>

using namespace std;

// Finds free memory in the block list
Block *findFreeMemory(Block **last, size_t size) {
    Block *current = blockList;
    while (current) {
        if (current->free && current->size >= size) {
            return current;
        }
        *last = current;
        current = current->next;
    }

    return nullptr;
}

// Adds memory in the end of the block list
Block *moreMemory(Block *last, size_t size) {
    Block *new_block = sbrk(0);
    void *requsted_memory = sbrk(size + sizeof(Block));
    if (requsted_memory == (void *)-1) {
        return nullptr;
    }
    if (last) {
        last->next = new_block;
    }
    new_block->size = size;
    new_block->next = nullptr;
    new_block->free = false;
    return new_block;
}

// Returns pointer to memory in the requested size.
// New_block contains the information about the memory.
void* customMalloc(size_t size) {
    if(size <= 0){
        cerr << "<malloc error>: passed nonpositive size" << endl;
        return nullptr;
    }

    size = ALIGN_TO_MULT_OF_4(size);
    Block *new_block;

    if(!blockList){     // In case it's the first allocation
        new_block = moreMemory(NULL, size);
        if(!new_block) {
            return nullptr;
        }
        blockList = new_block;
    }
    else {      // If it's not the first allocation
        Block *last_block = blockList;
        new_block = findFreeMemory(&last_block, size);
        if(!new_block) {    // If the list is full, get more memory
            new_block = moreMemory(last_block, size);
            if(!new_block) {
                return nullptr;
            }
        }
        else {      // If an empty block was found in the blockList
            new_block->free = false;
        }
    }

    return (new_block + 1);     // Pointer to the memory after new_block
}

// Frees memory
void customFree(void* ptr) {
    if (!ptr) {
        cerr << "<free error>: passed null pointer" << endl;
        return;
    }

    // Pointer to the information block
    auto *blockToFree = static_cast<Block *>(ptr - 1);
    blockToFree->free = true;

    if (!blockToFree->next) {   // If it's the last block in the list
        sbrk(-1 * (sizeof(Block) + blockToFree->size));
    }
    else {      // If the freed memory is the center of the list
        Block* current = blockList;
        while (current) {
            if (current->free && current->next && current->next->free) {
                current->size += sizeof(Block) + current->next->size;
                current->next = current->next->next;
            } else {
                current = current->next;
            }
        }
    }
}

// Allocating 'nmemb' units of memory in the size of 'size'
void* customCalloc(size_t nmemb, size_t size) {
    if (nmemb <= 0 || size <= 0) {
        cerr << "<calloc error>: passed nonpositive value" << endl;
        return nullptr;
    }

    size_t total_size = nmemb * size;
    void *ptr = customMalloc(total_size);
    if (!ptr) {
        return nullptr;
    }
    memset(ptr, 0, total_size);     // Fill the new memory with 0
    return ptr;
}

void* customRealloc(void* ptr, size_t size) {
    if (!ptr) {
        return customMalloc(size);
    }

    // Size is 0: free
    if (size == 0) {
        customFree(ptr);
        return nullptr;
    }

    auto *block = (Block *)(ptr - 1);

    // If the requested memory is at the same size of the old memory: do nothing
    if (block->size == size) {
        return ptr;
    }

    // If new size is bigger: replace with a bigger size pointer
    if (block->size < size) {
        // * * * we should copy ptr content to temp, free ptr, and search for memory
        //at the requested size * * * //
        void *new_ptr = customMalloc(size);
        if (!new_ptr) {
            return nullptr;
        }
        memcpy(new_ptr, ptr, block->size);
        customFree(ptr);
        return new_ptr;
    }

    // If new size is smaller: replace with smaller size pointer
    if (block->size > size) {
        void *new_ptr = customMalloc(size);
        if (!new_ptr) {
            return nullptr;
        }
        memcpy(new_ptr, ptr, size);
        customFree(ptr);
        return new_ptr;
    }
}