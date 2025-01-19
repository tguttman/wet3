//
// Created by tomer on 17/01/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "customAllocator.h"

using namespace std;

void* customMalloc(size_t size) {
    if(size <= 0){
        cerr << "<malloc error>: passed nonpositive size" << endl;
        return nullptr;
    }

    size = ALIGN_TO_MULT_OF_4(size);
    block *new_block;

    if(!blockList){
        new_block = moreMemmory(size);
    }
}
