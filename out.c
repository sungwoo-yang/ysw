#include <stdio.h>

int main(int argc, char **argv) {
	for(int arg = 1; arg < argc; ++arg)
		printf("%s%s", arg == 1 ? "" : " ", argv[arg]);
	putchar('\n');
	return 0;
}
