//
// Created by tomer on 17/01/2025.
//

#include "customAllocator.h"

int main() {
  char *arr = (char *)customMalloc(sizeof(char));
  *arr = 'a';
  customFree(arr);
  return 0;
}