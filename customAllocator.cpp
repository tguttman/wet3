//
// Created by tomer on 17/01/2025.
//

#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include "customAllocator.h"
#include <string.h>

using namespace std;
Block* blockList;
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
    Block *new_block = static_cast<Block*>(sbrk(0));
    void *requsted_memory = sbrk(size + sizeof(Block));
    if (requsted_memory == (void *)-1) {
        if (errno == ENOMEM) {
            Block* current = blockList;
            Block *next = current->next;
            while (current) {
                customFree(current + 1);
                current = next;
                next = current->next;
            }
            cerr << "<sbrk/brk error>: out of memory" << endl;
            exit(1);
        } /*else if (errno == EINVAL) {
            printf("Error: Invalid increment value (EINVAL)\n");
        } else {
            printf("Error: Unknown error occurred (%s)\n", strerror(errno));
        }*/
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
    auto *blockToFree = static_cast<Block *>(ptr) - 1;
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

    const size_t total_size = nmemb * size;
    void *ptr = customMalloc(total_size);
    if (!ptr) {
        return nullptr;
    }
    memset(ptr, 0, total_size);     // Fill the new memory with 0
    return ptr;
}

void* customRealloc(void* ptr, size_t size) {
    // Ptr is null: malloc
    if (!ptr) {
        if (size == 0) return nullptr;
        return customMalloc(size);
    }

    // Size is 0: free
    if (size == 0) {
        customFree(ptr);
        return ptr;     // Weird but that's what they asked
    }

    auto *block = (Block *)(ptr) - 1;

    // If the requested memory is at the same size of the old memory: do nothing
    if (block->size == size) {
        return ptr;
    }

    // Temporary memory to hold ptr content
    void *temp = customMalloc(size);
    memcpy(temp, ptr, size);

    customFree(ptr);    // Freeing ptr so customMalloc will see it as empty memory
    void *new_ptr = customMalloc(size);
    if (!new_ptr) {
        return nullptr;
    }
    memcpy(new_ptr, temp, size);
    customFree(temp);
    return new_ptr;

    /*
    // If new size is bigger: replace with a bigger size memory
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

    // If new size is smaller: replace with smaller size memory
    if (block->size > size) {
        customFree(ptr);    // Freeing ptr so customMalloc will see it as empty memory
        void *new_ptr = customMalloc(size);
        if (!new_ptr) {
            return nullptr;
        }
        memcpy(new_ptr, temp, size);
        customFree(temp);
        return new_ptr;
    }*/
}