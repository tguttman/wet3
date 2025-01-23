//
// Created by tomer on 17/01/2025.
//

#include "customAllocator.h"

int main() {
  char *arr = (char *)customMalloc(sizeof(char));
  *arr = 'a';
  char *arr2 = (char *)customCalloc(3 , sizeof(char));
  arr2[0] = 'b';
  arr2[1] = 'c';
  arr2[2] = 'd';
  customFree(arr);
  customFree(arr2);
  return 0;
}