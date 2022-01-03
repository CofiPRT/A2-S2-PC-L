#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10000


int main(int argc, char** argv) {
	init(HOST,PORT);
	msg t;

	if (argc == 1) {
		// nothing to print
		return 0;
	}

	int i;
	for (i = 1; i < argc; i++) {
		int source_file = open(argv[i], O_RDONLY);

		if (source_file < 0) {
			fprintf(stderr, "[send] ERROR: Could not open file '%s'\n", argv[i]);
			exit(-1);
		}

		strcpy(t.payload, argv[i]);
		t.len = -1;

		// Send filename
		printf("[send] INFO: Sending filename '%s'...\n", argv[i]);
		send_message(&t);

		// Check response
		if (recv_message(&t) < 0) {
			fprintf(stderr, "[send] ERROR: Filename confirmation error\n");
			exit(-2);
		}

		printf("[send] INFO: Filename confirmation received\n");

		int file_size = lseek(source_file, 0, SEEK_END);
		lseek(source_file, 0, SEEK_SET);

		int message_index = 1;

		while (lseek(source_file, 0, SEEK_CUR) < file_size) {
			int bytes_read = read(source_file, t.payload, sizeof(t.payload));

			if (bytes_read < 0) {
				fprintf(stderr, "[send] ERROR: Could not read from file '%s'\n", argv[i]);
				exit(-2);
			}

			t.len = bytes_read;

			// Start sending
			printf("[send] INFO: Sending message '%d' from file '%s'...\n", message_index++, argv[i]);
			send_message(&t);

			// Check response
			if (recv_message(&t) < 0) {
				fprintf(stderr, "[send] ERROR: Error while waiting for an answer\n");
				exit(-3);
			}

			printf("[send] INFO: Received response: '%s'\n", t.payload);
		}
	}

	// No more files, notify receiver
	t.len = -2;
	send_message(&t);

	return 0;
}
