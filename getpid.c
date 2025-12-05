#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
	(void) argc;
	printf("(%s) %d\n", argv[0], getpid());
	return 0;
}
