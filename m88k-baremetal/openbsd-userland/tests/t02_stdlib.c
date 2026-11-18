#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int cmpi(const void *a, const void *b) { int x = *(const int *)a, y = *(const int *)b; return (x > y) - (x < y); }
static int cmps(const void *a, const void *b) { return strcmp(*(const char *const *)a, *(const char *const *)b); }
int main(void) {
  int a[] = {5, 3, 9, -1, 0, 7, 3, 12, -8, 100};
  qsort(a, 10, sizeof a[0], cmpi);
  for (int i = 0; i < 10; i++) printf("%d ", a[i]);
  printf("\n");
  int key = 7, *p = bsearch(&key, a, 10, sizeof a[0], cmpi);
  printf("bsearch %d\n", p ? (int)(p - a) : -1);
  const char *s[] = {"pear", "apple", "fig", "banana", "cherry"};
  qsort(s, 5, sizeof s[0], cmps);
  for (int i = 0; i < 5; i++) printf("%s ", s[i]);
  printf("\n");
  char *m = malloc(10); strcpy(m, "hello");
  for (int n = 20; n < 2000; n *= 2) { m = realloc(m, n); strcat(m, "!"); }
  printf("%s %zu\n", m, strlen(m)); free(m);
  printf("%ld %ld %lu %d\n", strtol("-1234", 0, 10), strtol("ff", 0, 16), strtoul("4294967295", 0, 10), atoi("77"));
  printf("%s %s %d %d\n", strchr("abcdef", 'd'), strstr("hello world", "o w"), strncmp("abc", "abd", 2), memcmp("a", "b", 1) < 0);
  char b[32]; memset(b, 'x', 31); b[31] = 0; memcpy(b + 3, "ABC", 3); puts(b);
  return 0;
}
