#include <loki/lokilib.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	uint result;
	GET_CONTROL_REGISTER(result, 1);
	if (result != 0) {
		fprintf(stderr, "%s: Incorrect value for creg 1.\n", argv[0]);
		return EXIT_FAILURE;
	}
	result = 0xdeadface;
	SET_CONTROL_REGISTER(result, 13);
	if (result != 0xdeadface) {
		fprintf(stderr, "%s: Incorrect value for set on creg 13.\n", argv[0]);
		return EXIT_FAILURE;
	}
	GET_CONTROL_REGISTER(result, 13);
	if (result != 0xdeadface) {
		fprintf(stderr, "%s: Incorrect value for creg 13.\n", argv[0]);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
