#include <stdio.h>
#include <cstring>

int main() {
  int **a;
  int *b[5];
  memset(b, 0, sizeof(int*) * 5);
  printf("%x", a);
  printf("%x", b);

  for (int i = 0; i < 5; i++) {
    a[i] = &i;
    printf("%d\n", *a[i]);
    printf("%x\n", a[i]);
    b[i] = &i;
  }

  for (int i = 0; i < 5; i++) {
    printf("i=%d\n", i);
    printf("%d\n", *a[i]);
    printf("%d\n", *(b[i]));
  }
}