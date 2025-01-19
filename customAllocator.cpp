//
// Created by tomer on 17/01/2025.
//

#include <stdio.h>
#include <iostream>
#include "customAllocator.h"

using namespace std;

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


void customFree(void* ptr) {
    if (!ptr) {
        cerr << "<free error>: passed null pointer" << endl;
        return;
    }

    auto *blockToFree = static_cast<Block *>(ptr - 1);
    blockToFree->free = true;

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