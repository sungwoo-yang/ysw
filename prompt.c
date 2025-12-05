#include <stdio.h>
#include <unistd.h>

#define DELAY 5 // seconds

int main(int argc, char **argv) {
	(void) argc;

	printf("(%s) Delaying for %d seconds...\n", argv[0], DELAY);
	sleep(DELAY);

	printf("(%s) Hit enter to continue: ", argv[0]);
	getchar();

	printf("(%s) Exiting.\n", argv[0]);
	return 0;
}
