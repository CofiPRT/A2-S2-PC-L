#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];

	if (argc < 3) {
		usage(argv[0]);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "[client] ERROR: Could not open socket\n");

	printf("[client] INFO: Successfully opened socket '%d'\n", sockfd);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));
	ret = inet_aton(argv[1], &serv_addr.sin_addr);
	DIE(ret == 0, "[client] ERROR: Error in function 'inet_aton', "
					"called with '%s'\n", argv[1]);

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "[client] ERROR: Could not connect to socket '%d'\n", sockfd);

	printf("[client] INFO: Successfully connected to socket '%d'\n", sockfd);

	fd_set read_fds;
	fd_set tmp_fds;
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	FD_SET(sockfd, &read_fds);
	FD_SET(0, &read_fds);

	int fdmax = sockfd;

	while (1) {
  		// se citeste de la tastatura
		tmp_fds = read_fds;

		int select_ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(select_ret < 0, "[client] ERROR: Could not select message\n");

		printf("[client] INFO: Successfully selected message\n");

		memset(buffer, 0, BUFLEN);
		
		if (FD_ISSET(0, &tmp_fds)) {
			// de la stdin
			int recv_read = read(0, buffer, BUFLEN);
			DIE(recv_read < 0, "[client] ERROR: Could not read from stdin\n");

			printf("[client] INFO: Message from stdin\n");

			if (strncmp(buffer, "exit", 4) == 0) {
				printf("[client] INFO: Exiting...\n");
				break;
			}

			// se trimite mesaj la server
			int send_ret = send(sockfd, buffer, strlen(buffer), 0);
			DIE(send_ret < 0, "[client] ERROR: Could not send message on "
								"socket '%d'\n", sockfd);

			printf("[client] INFO: Successfully sent message to server\n");
		} else {
			// de la server
			int recv_ret = recv(sockfd, buffer, sizeof(buffer), 0);
			DIE(recv_ret < 0, "[client] ERROR: Error while receiving message "
				"from the server\n");

			if (recv_ret == 0) {
				// serverul s-a inchis
				printf("[client] INFO: Server shut down. Exiting...\n");
				break;
			}

			printf("[client] INFO: Successfully received message from server:\n"
				"%s\n", buffer);
		}
	}

	shutdown(sockfd, SHUT_RDWR);
	close(sockfd);

	printf("[client] INFO: Successfully shut down client\n");

	return 0;
}
