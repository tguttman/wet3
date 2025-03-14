#ifndef CUSTOM_ALLOCATOR_
#define CUSTOM_ALLOCATOR_

/*=============================================================================
* do no edit lines below!
=============================================================================*/
#include <stddef.h> //for size_t
#include <list>
#include <pthread.h>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <unistd.h>
#include <errno.h>

using namespace std;

void* customMalloc(size_t size);
void  customFree(void* ptr);
void* customCalloc(size_t nmemb, size_t size);
void* customRealloc(void* ptr, size_t size);
/*=============================================================================
* do no edit lines above!
=============================================================================*/

/*=============================================================================
* if writing bonus - uncomment lines below
=============================================================================*/
/*#ifndef __BONUS__
#define __BONUS__
#endif
void* customMTMalloc(size_t size);
void* customMTFree(void* ptr);
void* customMTCalloc(size_t nmemb, size_t size);
void* customMTRealloc(void* ptr, size_t size);

void heapCreate();
void heapKill();
*/
/*=============================================================================
* defines
=============================================================================*/
#define SBRK_FAIL (void*)(-1)
#define ALIGN_TO_MULT_OF_4(x) (((((x) - 1) >> 2) << 2) + 4)
#define KB (1 << 10)
#define SLICE_SIZE (4 * KB)

using namespace std;
void print_blockList();
/*inline void mutex_init(pthread_mutex_t* lock) {
    if (pthread_mutex_init(lock, NULL) != 0) {
        cerr << "<mutex error>: mutex init failed" << endl;
        heapKill();
        exit(1);
    }
}

inline void mutex_lock(pthread_mutex_t* lock) {
    if (pthread_mutex_lock(lock) != 0) {
        cerr << "<mutex error>: mutex lock failed" << endl;
        heapKill();
        exit(1);
    }
}

inline int mutex_trylock(pthread_mutex_t* lock) {
    int res = pthread_mutex_trylock(lock);
    if (res != 0 && errno != EBUSY) {
        cerr << "<mutex error>: mutex trylock failed" << endl;
        heapKill();
        exit(1);
    }
    return res;
}

inline void mutex_unlock(pthread_mutex_t* lock) {
    if (pthread_mutex_unlock(lock) != 0) {
        cerr << "<mutex error>: mutex unlock failed" << endl;
        heapKill();
        exit(1);
    }
}

inline void mutex_destroy(pthread_mutex_t* lock) {
    if (pthread_mutex_destroy(lock) != 0) {
        cerr << "<mutex error>: mutex destroy failed" << endl;
        heapKill();
        exit(1);
    }
}

class ReadersWritersLock {
private:
    uint32_t num_readers = 0;
    pthread_mutex_t r_lock;
    pthread_mutex_t w_lock;

public:
    ReadersWritersLock() {
        mutex_init(&r_lock);
        mutex_init(&w_lock);
    }

    ~ReadersWritersLock() {
        mutex_destroy(&r_lock);
        mutex_destroy(&w_lock);
    }

    void read_lock() {
        mutex_lock(&r_lock);
        this->num_readers++;
        if (this->num_readers == 1)
            mutex_lock(&w_lock);
        mutex_unlock(&r_lock);
    }

    void read_unlock() {
        mutex_lock(&r_lock);
        this->num_readers--;
        if (this->num_readers == 0)
            mutex_unlock(&w_lock);
        mutex_unlock(&r_lock);
    }

    void write_lock() {
        mutex_lock(&w_lock);
    }

    void write_unlock() {
        mutex_unlock(&w_lock);
    }
};
*/
/*=============================================================================
* Block
=============================================================================*/
//suggestion for block usage - feel free to change this
typedef struct Block
{
    size_t size;
    bool free;
    void* memory; // Pointer to the allocated memory
    struct Block* next;
    struct Block* previous;

    void print() {
        cout << "size=" << size << ", free=" << free << ", memory=" << memory << ", next=" << next << ", previous=" << previous << endl;
    }
} Block_t;

template <class T>
class LinkedList {
public:
    T* head;
    T* tail;


    LinkedList() : head(nullptr), tail(nullptr) {}

    void add(T* new_block) {
        if (!head) {
            head = new_block;
            tail = new_block;
        } else {
            tail->next = new_block;
            new_block->previous = tail;
            tail = new_block;
        }
    }

    void remove(T* block) {
        if (head == block) {
            head = block->next;
            if (head) {
                head->previous = nullptr;
            } else {
                tail = nullptr;
            }
            return;
        }

        block->previous->next = block->next;

        if (block->next) {
            block->next->previous = block->previous;
        } else {
            tail = block->previous;
        }
    }

    T* find(size_t size) {
        T* current = head;
        while (current) {
            if (current->free && current->size >= size) {
                return current;
            }
            current = current->next;
        }
        return nullptr;
    }
};

typedef LinkedList<Block_t> BlockList_t;

/*typedef struct Slice {
    size_t free_space;
    pthread_mutex_t lock;
    BlockList_t block_list;
    struct Slice* next;
    struct Slice* previous;

    Slice() : free_space(SLICE_SIZE), next(nullptr) {
        mutex_init(&lock);
        Block_t* new_block = (Block_t*)sbrk(sizeof(Block_t));
        new_block->size = SLICE_SIZE;
        new_block->free = true;
        new_block->memory = sbrk(SLICE_SIZE);
        if (new_block->memory == SBRK_FAIL) {
            cerr << "<sbrk/brk error>: out of memory" << endl;
            heapKill();
            exit(1);
        }
        new_block->next = nullptr;
        new_block->previous = nullptr;
        block_list.add(new_block);
    }

    ~Slice() {
        mutex_destroy(&lock);
        Block_t* current = block_list.head;
        while (current) {
            Block_t* next = current->next;
            sbrk(-(current->size + sizeof(Block_t)));
            current = next;
        }
    }
} Slice_t;
*/
extern BlockList_t blockList;
/*extern LinkedList<Slice_t> heap;
extern size_t num_slices;
*/
Block_t* findFreeBlock(BlockList_t& list, size_t size);
Block_t* allocateBlock(size_t size);

#endif // CUSTOM_ALLOCATOR_
