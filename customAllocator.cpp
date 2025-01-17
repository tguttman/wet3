//
// Created by tomer on 17/01/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <malloc.h>

using namespace std;

void* customMalloc(size_t size){
    if(size <= 0){
        cerr << "<malloc error>: passed nonpositive size" << endl;
        return nullptr;
    }
}
