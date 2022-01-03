#include "helpers.h"

// stuff we work with

typedef struct client {
	char name[NAMELEN + 1];
	struct in_addr ip;
	uint16_t port;
	int socket;

	struct client *next;
} client;

client *clients = NULL;

struct sockaddr_in client_addr;

client *new_client(char *name, struct in_addr addr, uint16_t port, int socket) {
	client *cli = malloc(sizeof(client));

	DIE(!cli, "[server] ERROR: Could not alloc new client\n");

	memset(cli->name, 0, NAMELEN + 1);
	memcpy(cli->name, name, NAMELEN);

	memcpy(&(cli->ip), &addr, sizeof(struct in_addr));
	memcpy(&(cli->port), &port, sizeof(port));
	memcpy(&(cli->socket), &socket, sizeof(socket));

	cli->next = NULL;

	return cli;
}

client *add_client(client *cli) {
	if (clients == NULL) {
		// beginning
		clients = cli;
	} else {
		cli->next = clients;
		clients = cli;
	}

	return cli;
}

// main sockets
int sockfd_tcp = -1;
int fd_max = -1;

// clients
fd_set read_fds; // descriptor list

void init_sockets(char *port_char) {
	// open TCP socket
	sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd_tcp < 0, "[server] ERROR: Could not open TCP socket\n");
	GOOD("[server] INFO: Opened TCP socket on '%d'\n", sockfd_tcp);

	// verify PORT
	short port = atoi(port_char);
	DIE(port <= 0, "[server] ERROR: Invalid port or error in 'atoi'. "
					"Called with port number '%s'", port_char);
	GOOD("[server] INFO: Working on port '%d'\n", port);

	// fill in the sockaddr struct
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	struct sockaddr *cast_addr = (struct sockaddr *) &server_addr;

	// bind TCP socket
	int result = bind(sockfd_tcp, cast_addr, SOCKADDR_SIZE);
	DIE(result < 0, "[server] ERROR: Could not bind TCP socket\n");
	GOOD("[server] INFO: Bound TCP socket\n");

	// set the TCP socket as LISTENING socket
	result = listen(sockfd_tcp, MAX_CLIENTS);
	DIE(result < 0, "[server] ERROR: Could not listen on socket '%d'\n",
					sockfd_tcp);
	GOOD("[server] INFO: Set TCP on listening mode\n");
}

void accept_client() {
	// create a new socket for this client
	struct sockaddr *cast_addr = (struct sockaddr *) &client_addr;
	socklen_t length = sizeof(client_addr);

	int client_fd = accept(sockfd_tcp, cast_addr, &length);
	DIE(client_fd < 0, "[server] ERROR: Could not accept the new connection\n");
	GOOD("[server] INFO: Accepted new connection on socket '%d'\n", client_fd);

	// add to set
	FD_SET(client_fd, &read_fds);
	fd_max = max(fd_max, client_fd);
}

void manage_client(int client_fd) {
	char buffer[BUFLEN] = {0};

	// store the message received on this socket
	int result = recv(client_fd, buffer, sizeof(buffer), 0);
	DIE(result < 0, "[server] ERROR: Error while receiving message from "
					"socket '%d'\n", client_fd);
	GOOD("[server] INFO: Received message on socket '%d':\n%s\n", client_fd,
																	buffer);
	GOOD("[server] INFO: Client IP '%s' and PORT '%d'\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

	char command[COMMANDLEN + 1] = {0};
	char arg[ARGLEN + 1] = {0};

	int args = sscanf(buffer, "%s %s ", command, arg);
	DIE(args == 0, "[server] ERROR: Invalid message from "
					"socket '%d'\n", client_fd);
	GOOD("[server] INFO: Parsed message from socket '%d'\n", client_fd);

	if (!strcmp(command, "NAME")) {
		DIE(args != 2, "[server] ERROR: Invalid number of arguments for "
						"command 'NAME' "
						"Expected:\n\t\tNAME ID, "
						"Received:\n\t\t'%s'\n", buffer);

		client *new_cli = new_client(arg, client_addr.sin_addr, client_addr.sin_port, client_fd);
		add_client(new_cli);
		
		GOOD("[server] INFO: Successfully added new client with name '%s', "
			"on port '%d' and ip '%s'\n", new_cli->name,
											new_cli->port,
											inet_ntoa(new_cli->ip));
	} else if (!strcmp(command, "LIST")) {
		DIE(args != 1, "[server] ERROR: Invalid number of arguments for "
						"command 'LIST'\n"
						"\tExpected:\n\t\tLIST\n"
						"\tReceived:\n\t\t%s\n", buffer);
		GOOD("[server] INFO: Recognized command 'LIST'\n");

		memset(buffer, 0, sizeof(buffer));

		sprintf(buffer, "Available clients:\n");
		int clients_no = 0;

		client *curr_client = clients;

		while (curr_client) {
			if (curr_client->socket != client_fd) {
				strcat(buffer, "\t");
				strcat(buffer, curr_client->name);
				strcat(buffer, "\n");

				clients_no++;
			}

			curr_client = curr_client->next;
		}

		if (clients_no == 0) {
			strcat(buffer, "\tnone\n");
		}

		send(client_fd, buffer, sizeof(buffer), 0);

		GOOD("[server] INFO: Successfully sent LIST to client\n");
	} else if (!strcmp(command, "CONNECT")) {
		DIE(args != 2, "[server] ERROR: Invalid number of arguments for "
						"command 'CONNECT'\n"
						"\tExpected:\n\t\tCONNECT NAME\n"
						"\tReceived:\n\t\t%s\n", buffer);
		GOOD("[server] INFO: Recognized command 'CONNECT'\n");

		memset(buffer, 0, sizeof(buffer));

		sprintf(buffer, "CONNECT ");
		int offset = strlen(buffer);
		int client_found = FALSE;

		client *curr_client = clients;
		client *prev_client = NULL;

		while (curr_client) {
			if (!strcmp(arg, curr_client->name)) {
				memcpy(buffer + offset, &(curr_client->ip), sizeof(struct in_addr));
				offset += sizeof(struct in_addr);

				memcpy(buffer + offset, &(curr_client->port), sizeof(uint16_t));

				client_found = TRUE;

				// also remove them
				if (prev_client == NULL) {
					// first client
					clients = curr_client->next;
				} else {
					prev_client->next = curr_client->next;
				}

				free(curr_client);

				break;
			}

			prev_client = curr_client;
			curr_client = curr_client->next;
		}

		if (!client_found) {
			sprintf(buffer, "No client with name '%s' exists\n", arg);
		}

		send(client_fd, buffer, sizeof(buffer), 0);

		if (client_found) {
			// also remove the issuing client

			curr_client = clients;
			prev_client = NULL;

			while (curr_client) {
				if (curr_client->socket == client_fd) {
					if (prev_client == NULL) {
						clients = curr_client->next;
					} else {
						prev_client->next = curr_client->next;
					}

					free(curr_client);

					break;
				}

				prev_client = curr_client;
				curr_client = curr_client->next;
			}
		}

		FD_CLR(client_fd, &read_fds);

		GOOD("[server] INFO: Successfully connected client\n");
	} else if (!strcmp(command, "EXIT")) {
		DIE(args != 1, "[server] ERROR: Invalid number of arguments for "
						"command 'EXIT'\n"
						"\tExpected:\n\t\tEXIT\n"
						"\tReceived:\n\t\t%s\n", buffer);
		GOOD("[server] INFO: Recognized command 'EXIT'\n");



		// log
		printf("Client disconnected.\n");
	} else {
		DIE(TRUE, "[server] ERROR: Invalid command\n"
					"\tExpected one of the following:\n"
					"\t\tsubscribe TOPIC SF\n"
					"\t\tunsubscribe TOPIC\n"
					"\t\texit\n"
					"\tReceived:\n\t\t%s\n", buffer);
	}
}




void manage_server() {
	fd_set tmp_fds; // auxiliary descriptor list

	// initialise descriptor sets
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// add the TCP and UDP sockets (and STDIN)
	FD_SET(sockfd_tcp, &read_fds);

	// highest numbered file descriptor (used as argument in 'select')
	fd_max = sockfd_tcp;

	// run indefinitely
	while (TRUE) {
		// clone the set
		tmp_fds = read_fds;

		GOOD("[server] INFO: Waiting to select message\n");

		// select file descriptor ready for reading
		int result = select(fd_max + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(result < 0, "[server] ERROR: Could not select message\n");
		GOOD("[server] INFO: Successfully selected message\n");

		// find selected socket
		for (int i = 1; i <= fd_max; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd_tcp) {
					// new incoming TCP client, accept
					GOOD("[server] INFO: Accepting client\n");
					accept_client();
				} else {
					// client message received
					GOOD("[server] INFO: Managing client message\n");
					manage_client(i);
				}
			}
		}
	}
}

void stop_server() {
	// close connections
	for (int i = 1; i < fd_max; i++) {
		if (FD_ISSET(i, &read_fds) && i != sockfd_tcp) {
			// shutdown
			shutdown(i, SHUT_RDWR);
			close(i);

			FD_CLR(i, &read_fds);
		}
	}

	// close sockets
	close(sockfd_tcp);
}

int main(int argc, char *argv[]) {
	DIE(argc < 2, "[server] ERROR: Usage: %s PORT\n", argv[0]);

	// initialise sockets
	init_sockets(argv[1]);

	// do stuff
	manage_server();

	// stop it
	stop_server();

	GOOD("[server] INFO: Successfully shut down server\n");
}