#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    if (argc == 1) {
        // nothing to print
        exit(-1);
    }

    char buf[1024];

    int i;
    for (i = 1; i < argc; i++) {
        int source_file = open(argv[i], O_RDONLY);

        if (source_file < 0) {
            perror("Error opening file\n");
            exit(-2);
        }

        int bytes;
        if ((bytes = read(source_file, buf, sizeof(buf))) < 0) {
            perror("Error reading file\n");
            exit(-3);
        }

        if (write(1, buf, bytes)) {
            perror("Error writing to stdout");
            exit(-4);
        }
    }

    return 0;
}