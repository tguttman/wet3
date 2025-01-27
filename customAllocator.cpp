//
// Created by tomer on 17/01/2025.
//

#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include "customAllocator.h"
#include <string.h>
#include <errno.h>
#include <cassert>

using namespace std;

Block_t* blockList = nullptr;
Block_t* lastBlock = nullptr;
Slice_t* heap = nullptr;
ReadersWritersLock heap_lock;
size_t num_slices = 0;

void print_blockList() {
    Block_t* current = blockList;
    cout << "-------blockList status:--------" << endl;
    int i = 1;
    while (current) {
        cout << "Block " << i << ": ";
        current->print();
        current = current->next;
        i++;
    }
}

void addBlock(Block_t** list, Block_t* new_block) {
    new_block->next = *list;
    *list = new_block;
    if (!new_block->next) 
        lastBlock = new_block;
    
}

void removeBlock(Block_t** list, Block_t* block) {
    Block_t** current = list;
    while (*current && *current != block) {
        current = &(*current)->next;
    }
    if (*current) {
        *current = block->next;
        if (!block->next) {
            blockList = (current == list) ? nullptr : *current;
        }
    }
}

Block_t* findFreeBlock(Block_t* list, size_t size) {
    while (list) {
        if (list->free && list->size >= size) {
            return list;
        }
        list = list->next;
    }
    return nullptr;
}

Block_t* allocateBlock(size_t size) {
    Block_t* new_block = (Block_t*)sbrk(sizeof(Block_t));
    if (new_block == (Block_t*)-1) {
        cerr << "<sbrk/brk error>: out of memory" << endl;
        return nullptr;
    }
    new_block->size = size;
    new_block->free = false;
    new_block->memory = sbrk(size);
    if (new_block->memory == (void*)-1) {
        sbrk(-sizeof(Block_t));
        cerr << "<sbrk/brk error>: out of memory" << endl;
        return nullptr;
    }
    new_block->next = nullptr;
    return new_block;
}

// Returns pointer to memory in the requested size.
// New_block contains the information about the memory.
void* customMalloc(size_t size) {
    if (size <= 0) {
        cerr << "<malloc error>: passed nonpositive size" << endl;
        return nullptr;
    }

    size = ALIGN_TO_MULT_OF_4(size);
    Block_t* new_block;

    if (!blockList) { // In case it's the first allocation
        new_block = allocateBlock(size);
        if (!new_block) {
            return nullptr;
        }
        blockList = new_block;
        lastBlock = new_block;
    } else { // If it's not the first allocation
        new_block = findFreeBlock(blockList, size);
        if (!new_block) { // If the list is full, get more memory
            new_block = allocateBlock(size);
            if (!new_block) {
                return nullptr;
            }
            addBlock(&blockList, new_block);
        } else { // If an empty block was found in the blockList
            if (new_block->size > size) {
                Block_t* remaining_block = (Block_t*)sbrk(sizeof(Block_t));
                if (remaining_block == (Block_t*)-1) {
                    cerr << "<sbrk/brk error>: out of memory" << endl;
                    return nullptr;
                }
                remaining_block->size = new_block->size - size;
                remaining_block->free = true;
                remaining_block->memory = (char*)new_block->memory + size;
                remaining_block->next = new_block->next;
                new_block->size = size;
                new_block->next = remaining_block;
                if (!remaining_block->next) {
                    lastBlock = remaining_block;
                }
            }
            new_block->free = false;
        }
    }
    return new_block->memory; // Pointer to the allocated memory
}

// Thread-safe heap allocation in a circular manner
void* customMTMalloc(size_t size) {
    if(size <= 0){
        cerr << "<malloc error>: passed nonpositive size" << endl;
        return nullptr;
    }

    size = ALIGN_TO_MULT_OF_4(size);
    Block_t *new_block;

    heap_lock.read_lock();
    static Slice_t* current_slice = heap;
    for (size_t i = 0; i < num_slices; ++i) {
        if (mutex_trylock(&current_slice->lock) == 0) {
            new_block = findFreeBlock(current_slice->block_list, size);
            if (new_block) {
                if (new_block->size > size) {
                    Block_t* remaining_block = (Block_t*)sbrk(sizeof(Block_t));
                    if (remaining_block == (Block_t*)-1) {
                        cerr << "<sbrk/brk error>: out of memory" << endl;
                        return nullptr;
                    }
                    remaining_block->size = new_block->size - size;
                    remaining_block->free = true;
                    remaining_block->memory = (char*)new_block->memory + size;
                    remaining_block->next = new_block->next;
                    new_block->size = size;
                    new_block->next = remaining_block;
                    if (!remaining_block->next) {
                        lastBlock = remaining_block;
                    }
                }
                new_block->free = false;
                mutex_unlock(&current_slice->lock);
                heap_lock.read_unlock();
                return new_block->memory;
            }
            mutex_unlock(&current_slice->lock);
        }
        current_slice = current_slice->next ? current_slice->next : heap;
    }
    heap_lock.read_unlock();

    heap_lock.write_lock();
    Slice_t* new_slice = (Slice_t*)sbrk(sizeof(Slice_t));
    if (new_slice == (Slice_t*)-1) {
        cerr << "<sbrk/brk error>: out of memory" << endl;
        heap_lock.write_unlock();
        return nullptr;
    }
    *new_slice = Slice_t();
    new_slice->next = heap;
    heap = new_slice;
    num_slices++;
    mutex_lock(&new_slice->lock);
    heap_lock.write_unlock();

    new_block = new_slice->block_list;
    new_block->free = false;
    mutex_unlock(&new_slice->lock);
    return new_block->memory;
}

Block_t *findBlock(void *ptr) {
    Block_t* current = blockList;
    while (current) {
        if (current->memory == ptr) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

// Frees memory
void customFree(void* ptr) {
    if (!ptr) {
        cerr << "<free error>: passed null pointer" << endl;
        return;
    }
    if (!blockList) {
        return;
    }
    // Pointer to the information block
    auto* blockToFree = findBlock(ptr);
    if (!blockToFree) 
        return;
    
    blockToFree->free = true;

    // Merge adjacent free blocks
    Block_t* current = blockList;
    while (current) {
        if (current->free && current->next && current->next->free) {
            current->size += current->next->size + sizeof(Block_t);
            current->next = current->next->next;
            if (!current->next)
                lastBlock = current;
        } else {
            current = current->next;
        }
    }
    // Reduce program break if possible
    if (blockList && blockList->free) {
        bool last = !blockList->next;
        if (sbrk(-(blockList->size + sizeof(Block_t))) == (void*)-1) {
            cerr << "<sbrk/brk error>: unable to reduce program break" << endl;
            exit(1);
        }
        if (last) {
            blockList = nullptr;
            lastBlock = nullptr;
        } else {
            removeBlock(&blockList, blockList);
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
    size = ALIGN_TO_MULT_OF_4(size);
    if (!ptr) {
        if (size == 0) return nullptr;
        return customMalloc(size);
    }

    // Size is 0: free
    if (size == 0) {
        customFree(ptr);
        return ptr;     // Weird but that's what they asked
    }

    auto *block = findBlock(ptr);

    // If the requested memory is at the same size of the old memory: do nothing
    if (block->size == size) {
        return ptr;
    }

    // Temporary memory to hold ptr content
    void *temp = customMalloc(size);
    Block_t *new_block = findBlock(temp);
    size_t min_size = min(block->size, size);
    memcpy(new_block->memory, ptr, min_size);

    customFree(ptr);    // Freeing ptr so customMalloc will see it as empty memory
    /*void *new_ptr = customMalloc(size);
    if (!new_ptr) {
        return nullptr;
    }
    memcpy(new_ptr, temp, size);
    customFree(temp);*/
    return temp;
}

void heapCreate() {
    for (int i = 0 ; i < 8 ; i++) {
        Slice_t* new_slice = (Slice_t*)sbrk(sizeof(Slice_t));
        if (new_slice == (Slice_t*)-1) {
            cerr << "<sbrk/brk error>: out of memory" << endl;
            heapKill();
            exit(1);
        }
        *new_slice = Slice_t();
        new_slice->next = heap;
        heap = new_slice;
        num_slices++;
    }
}

void heapKill() {
    Slice_t* current_slice = heap;
    while (current_slice) {
        Block_t* current_block = current_slice->block_list;
        while (current_block) {
            Block_t* next_block = current_block->next;
            sbrk(-(current_block->size + sizeof(Block_t)));
            current_block = next_block;
        }
        Slice_t* next_slice = current_slice->next;
        sbrk(-sizeof(Slice_t));
        current_slice = next_slice;
        num_slices--;
    }
}