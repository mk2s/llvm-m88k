#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char **argv) {
	char *p = malloc(16);
	strcpy(p, "libc-only");
	printf("%s %d %s\n", p, argc, strlen(p) == 9 ? "ok" : "bad");
	free(p);
	return 0;
}
