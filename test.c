#include <stdio.h>
#include <stdlib.h>

/*
struct Person {
  int age;
};

typedef struct NotPerson {
  int age;
} NP;
*/

typedef struct Array{
  int arr[10];
} Array;

void edit(Array a1) {
  a1.arr[9] = -1;
}

int main (int argc, char **argv) {
  /*
  struct Person p1, p2;  
  NP np1, np2;
  */

  Array a1;

  for(int i = 0; i < 10; i++) {
    a1.arr[i] = i;
  } 


  printf("Element 9: %d\n", a1.arr[9]);
}