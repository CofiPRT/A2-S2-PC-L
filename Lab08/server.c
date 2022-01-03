#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int i, ret;
	socklen_t clilen;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "[server] ERROR: Cold not open socket\n");

	printf("[server] INFO: Successfully opened socket '%d'\n", sockfd);

	portno = atoi(argv[1]);
	DIE(portno == 0, "[server] ERROR: Error in function 'atoi', "
						"called with '%s'\n", argv[1]);

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "[server] ERROR: Could not bind socket '%d'\n", sockfd);

	printf("[server] INFO: Successfully bound socket '%d'\n", sockfd);

	ret = listen(sockfd, MAX_CLIENTS);
	DIE(ret < 0, "[server] INFO: Error while passivating socket '%d'\n", sockfd);

	printf("[server] INFO: Successfully passivated socket '%d'\n", sockfd);

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "[server] ERROR: Could not select message\n");

		printf("[server] INFO: Successfully selected message\n");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					printf("[server] INFO: New incoming connection\n");

					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "[server] ERROR: Could not "
						"accept the new connection on socket '%d'\n",
						sockfd);

					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}

					printf("[server] INFO: Successfully accepted the new "
						" connection from '%s', on port '%d', "
						"and socket '%d'\n",
							inet_ntoa(cli_addr.sin_addr),
							ntohs(cli_addr.sin_port), newsockfd);

					// will write available clients
					char list[BUFLEN];
					char tmp_list[BUFLEN];
					memset(list, 0, BUFLEN);
					int available_clients = 0;

					sprintf(list,
						"Welcome! Here's a list of available client IDs:");

					// notifying other clients
					int j;
					for (j = 0; j <= fdmax; j++) {
						if (!FD_ISSET(j, &read_fds)
								|| j == newsockfd
								|| j == sockfd)
							continue;

						// add to the list
						sprintf(tmp_list, "%s %d", list, j);
						memcpy(list, tmp_list, strlen(tmp_list));
						available_clients++;

						sprintf(buffer,
							"A new client with ID '%d' connected!\n",
								newsockfd);

						int send_ret = send(j, buffer, strlen(buffer), 0);
						DIE(send_ret < 0, "[server] ERROR: Error while "
							"sending message to client '%d'\n", j);

						printf("[server] INFO: Notified client '%d' "
							"regarding the new connection\n", j);
					}

					// offer the list of available clients
					if (available_clients == 0) {
						sprintf(list, "Welcome! Unfortunately, there "
							"aren't any other connected clients at the "
							"moment. But there will be, for sure!");
					}
					sprintf(tmp_list, "%s\n", list);
					memcpy(list, tmp_list, strlen(tmp_list));

					int send_ret = send(newsockfd, list, strlen(list), 0);
					DIE(send_ret < 0, "[server] ERROR: Error while "
						"sending message to client '%d'\n", j);

					printf("[server] INFO: Notified client '%d' "
						"regarding the available connections\n", newsockfd);


				} else {
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					printf("[server] INFO: New message from "
							"client '%d'\n", i);

					memset(buffer, 0, BUFLEN);
					int recv_ret = recv(i, buffer, sizeof(buffer), 0);
					DIE(recv_ret < 0, "[server] ERROR: Error while "
						"receiving message from client '%d'\n", i);

					printf("[server] INFO: Successfully received "
						"message from client '%d'\n", i);

					if (recv_ret == 0) {
						// conexiunea s-a inchis
						printf("[server] INFO: Client '%d' closed the "
								"socket\n", i);
						close(i);
						
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);

						// notifying other clients
						int j;
						for (j = 0; j <= fdmax; j++) {
							if (!FD_ISSET(j, &read_fds)
									|| j == i
									|| j == sockfd)
								continue;

							memset(buffer, 0, BUFLEN);
							sprintf(buffer,
								"Client with ID '%d' disconnected!\n", i);

							int send_ret = send(j, buffer, strlen(buffer), 0);
							DIE(send_ret < 0, "[server] ERROR: Error while "
								"sending message to client '%d'\n", j);

							printf("[server] INFO: Notified client '%d' "
								"regarding the disconnected client\n", j);
						}
					} else {
						printf ("[server] INFO: The message is:\n"
							"%s\n", buffer);

						char *message;
						int dest_client = (int)strtol(buffer, &message, 10);

						int send_ret = send(dest_client, message + 1, recv_ret, 0);
						DIE(send_ret < 0, "[server] ERROR: Error while "
							"sending message to client '%d'\n", dest_client);

						printf("[server] INFO: Successfully sent message "
							"to client '%d'\n", dest_client);
					}
				}
			}
		}
	}

	close(sockfd);

	return 0;
}
