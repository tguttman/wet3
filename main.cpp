//
// Created by tomer on 17/01/2025.
//

#include "customAllocator.h"

int main() {
  /*char *arr = (char *)customMalloc(15);
  *arr = 'a';
  customFree(arr);*/
  int *arr2 = (int *)customCalloc(3 , 4);
  arr2[0] = 0;
  arr2[1] = 1;
  arr2[2] = 2;
  arr2 = (int *)customRealloc(arr2, 8);
  customFree(arr2);
  return 0;
}