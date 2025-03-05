#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>

int main(int argc, char **argv) {
	if (argc < 3) {
		fprintf(stderr, "Usage: oct2bin <infile> <outfile>\n");
		return -1;
	}

	if (access(argv[1], R_OK) != 0) {
		fprintf(stderr, "Error: cannot access %s\n", argv[1]);
		return -1;
	}
	FILE *in = fopen(argv[1], "r");
	int out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC);
	char buf[1024];
	while (fgets(buf, 1024, in)) {
		uint16_t val = strtoul(buf, NULL, 8);
		write(out, &val, 2);
	}
	fclose(in);
	close(out);
}
