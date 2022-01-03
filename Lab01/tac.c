#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {


	if (argc == 1) {
		// nothing to print
		exit(-1);
	}

	char buf[1024];
	char ch;

	int i;
	for (i = 1; i < argc; i++) {
		int source_file = open(argv[i], O_RDONLY);

		if (source_file < 0) {
			perror("Error opening file\n");
			exit(-2);
		}

		int line_end = lseek(source_file, -1, SEEK_END);
		int line_start = line_end;

		while (line_start > 0) {
			// get curr char
			read(source_file, &ch, 1);
			// step back
			lseek(source_file, -1, SEEK_CUR);

			if (ch == '\n') {
				if (line_start == line_end) {
					// step back (before '\n')
					line_start--;
					lseek(source_file, -1, SEEK_CUR);
					continue;
				}

				// after '\n'
				lseek(source_file, 1, SEEK_CUR);
				read(source_file, buf, line_end - line_start);

				// write to stdout
				write(1, buf, line_end - line_start);

				// step back to found '\n'
				lseek(source_file, line_start - line_end - 1, SEEK_CUR);

				// at found '\n'
				line_end = line_start;
			}

			// either way, step back
			line_start--;
			lseek(source_file, -1, SEEK_CUR);
		}

		// first line of the file

		read(source_file, buf, line_end - line_start + 1);

		// write to stdout
		write(1, buf, line_end - line_start + 1);
	}
}