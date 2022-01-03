#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10001


int main(int argc, char** argv) {
	msg r;
	init(HOST,PORT);

	char file_name[MAX_LEN + 5];
	int source_file = -2;

	while (1) {
		if (recv_message(&r) < 0) {
			fprintf(stderr, "[recv] ERROR: Error while waiting for a message\n");
			exit(-1);
		}

		printf("[recv] INFO: Payload received\n");

		if (r.len == -2) {
			// No more files
			close(source_file);
			break;
		}

		if (r.len == -1) {
			if (source_file != -2) {
				close(source_file);
			}

			// Begin writing to a new file
			printf("[recv] INFO: Opening new file '%s'\n", file_name);

			sprintf(file_name, "%s_recv", r.payload);
			source_file = open(file_name, O_TRUNC | O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

			if (source_file < 0) {
				fprintf(stderr, "[recv] ERROR: Could not open file '%s'\n", file_name);
				exit(-2);
			}

			// Notify sender
			sprintf(r.payload, "[ACK] Successfully opened a new file");
			r.len = strlen(r.payload) + 1;
			send_message(&r);

			continue;
		}

		if (write(source_file, r.payload, r.len) < 0) {
			fprintf(stderr, "[recv] ERROR: Error while writing to file '%s'\n", file_name);
			exit(-3);
		}

		printf("[recv] INFO: Successfully written to file, notifying sender...\n");

		sprintf(r.payload, "[ACK] Successfully written to file");
		r.len = strlen(r.payload) + 1;
		send_message(&r);
	}

	return 0;
}
