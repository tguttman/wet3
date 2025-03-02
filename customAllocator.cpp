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

BlockList_t blockList;
/*LinkedList<Slice_t> heap;
ReadersWritersLock heap_lock;
size_t num_slices = 0;
*/
void print_blockList() {
    Block_t* current = blockList.head;
    cout << "-------blockList status:--------" << endl;
    int i = 1;
    while (current) {
        cout << "Block " << i << ": ";
        current->print();
        current = current->next;
        i++;
    }
}

void splitBlock(Block_t* block, size_t size) {
    if (block->size > size + sizeof(Block_t)) {
        Block_t* remaining_block = (Block_t*)((char*)block->memory + size);
        remaining_block->size = block->size - size - sizeof(Block_t);
        remaining_block->free = true;
        remaining_block->memory = (char*)remaining_block + sizeof(Block_t);
        remaining_block->next = block->next;
        remaining_block->previous = block;
        block->size = size;
        block->next = remaining_block;
        if (remaining_block->next) {
            remaining_block->next->previous = remaining_block;
        } else {
            blockList.tail = remaining_block;
        }
    }
    block->free = false;
}

Block_t* findFreeBlock(BlockList_t& list, size_t size) {
    Block_t* current = list.head;
    while (current) {
        if (current->free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

Block_t* allocateBlock(size_t size) {
    Block_t* new_block = (Block_t*)sbrk(sizeof(Block_t) + size);
    if (new_block == (Block_t*)-1) {
        cerr << "<sbrk/brk error>: out of memory" << endl;
        return nullptr;
    }
    new_block->size = size;
    new_block->free = false;
    new_block->memory = (char*)new_block + sizeof(Block_t);
    new_block->next = nullptr;
    new_block->previous = nullptr;
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

    if (!blockList.head) { // In case it's the first allocation
        new_block = allocateBlock(size);
        if (!new_block) {
            return nullptr;
        }
        blockList.add(new_block);
    } else { // If it's not the first allocation
        new_block = findFreeBlock(blockList, size);
        if (!new_block) { // If the list is full, get more memory
            new_block = allocateBlock(size);
            if (!new_block) {
                return nullptr;
            }
            blockList.add(new_block);
        } else { // If an empty block was found in the blockList
            splitBlock(new_block, size);
        }
    }
    return new_block->memory; // Pointer to the allocated memory
}

// Returns the block that contains the memory
Block_t *findBlock(void *ptr) {
    Block_t* current = blockList.head;
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
    if (!blockList.head) {
        return;
    }
    // Pointer to the information block
    auto* blockToFree = findBlock(ptr);
    if (!blockToFree || blockToFree->free) 
        return;
    
    blockToFree->free = true;

    // Merge adjacent free blocks
    Block_t* current = blockList.head;
    while (current != blockList.tail) {
        if (current->free && current->next->free) {
            current->size += current->next->size + sizeof(Block_t);
            blockList.remove(current->next);
        } else {
            current = current->next;
        }
    }
    // Reduce program break if possible
    if (blockList.tail->free) {
        size_t size = blockList.tail->size + sizeof(Block_t);
        blockList.remove(blockList.tail);
        if (sbrk(-size) == (void*)-1) {
            cerr << "<sbrk/brk error>: unable to reduce program break" << endl;
            exit(1);
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
    return temp;
}
/*
// Thread-safe heap allocation in a circular manner
void* customMTMalloc(size_t size) {
    if(size <= 0){
        cerr << "<malloc error>: passed nonpositive size" << endl;
        return nullptr;
    }

    size = ALIGN_TO_MULT_OF_4(size);
    Block_t *new_block;
    static Slice_t* current_slice = heap.head;
    heap_lock.read_lock();
    Slice_t* slice = current_slice;
    current_slice = current_slice->next ? current_slice->next : heap.head;
    heap_lock.read_unlock();
    mutex_lock(&slice->lock);

    new_block = findFreeBlock(slice->block_list, size);
    if (new_block) {
        splitBlock(new_block, size);
        mutex_unlock(&slice->lock);
        return new_block->memory;
    }
    mutex_unlock(&slice->lock);

    heap_lock.write_lock();
    Slice_t* new_slice = (Slice_t*)sbrk(sizeof(Slice_t));
    if (new_slice == (Slice_t*)-1) {
        cerr << "<sbrk/brk error>: out of memory" << endl;
        heap_lock.write_unlock();
        return nullptr;
    }
    *new_slice = Slice_t();
    heap.add(new_slice);
    num_slices++;
    mutex_lock(&new_slice->lock);
    heap_lock.write_unlock();

    new_block = new_slice->block_list.head;
    new_block->free = false;
    mutex_unlock(&new_slice->lock);
    return new_block->memory;
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
        heap.add(new_slice);
        num_slices++;
    }
}

void heapKill() {
    Slice_t* current_slice = heap.head;
    while (current_slice) {
        Block_t* current_block = current_slice->block_list.head;
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
}*/