#include "helpers.h"

int main(int argc, char *argv[]) {
	DIE(argc < 4, "[client] ERROR: Usage: %s ID SERVER_IP PORT\n", argv[0]);

	// verify PORT
	short port = atoi(argv[3]);
	DIE(port <= 0, "[client] ERROR: Invalid port or error in 'atoi'. "
					"Called with port number '%s'", argv[3]);
	GOOD("[client] INFO: Working on port '%d'\n", port);

	// open listening socket
	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	//DIE(listen_fd < 0, "[client] ERROR: Could not open listening socket\n");
	GOOD("[client] INFO: Opened listening socket on '%d'\n", listen_fd);

	struct sockaddr_in listen_addr;
	memset(&listen_addr, 0, sizeof(struct sockaddr_in));

	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(port);
	listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	struct sockaddr *cast_addr = (struct sockaddr *) &listen_addr;

	int result = bind(listen_fd, cast_addr, sizeof(struct sockaddr));
	// DIE(result < 0, "[client] ERROR: Could not bind listening socket\n");
	GOOD("[client] INFO: Bound listening socket\n");

	// set as LISTENING socket
	result = listen(listen_fd, MAX_CLIENTS);
	DIE(result < 0, "[server] ERROR: Could not listen on socket '%d'\n",
					listen_fd);
	GOOD("[server] INFO: Set socket on listening mode\n");

	// open socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "[client] ERROR: Could not open socket\n");
	GOOD("[client] INFO: Successfully opened socket '%d'\n", sockfd);

	// fill in the sockaddr struct
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	result = inet_aton(argv[2], &server_addr.sin_addr);
	DIE(result == 0, "[client] ERROR: Error in function 'inet_aton', "
						"called with '%s'\n", argv[2]);
	GOOD("[client] INFO: Successfully filled in server_addr details\n");

	// connect to server
	cast_addr = (struct sockaddr *) &server_addr;

	result = connect(sockfd, cast_addr, SOCKADDR_SIZE);
	DIE(result < 0, "[client] ERROR: Could not connect on "
					"socket '%d'\n", sockfd);
	GOOD("[client] INFO: Successfully connected to socket '%d'\n", sockfd);

	// send connect message to server, containing our ID
	char buffer[BUFLEN] = {0};
	sprintf(buffer, "NAME %s", argv[1]);

	result = send(sockfd, buffer, strlen(buffer), 0);
	DIE(result < 0, "[client] ERROR: Could not send NAME message "
					"on socket '%d'\n", sockfd);
	GOOD("[client] INFO: Successfully sent 'NAME' message to server\n");

	int con_fd = -1;

	// initialise the descriptor sets
	fd_set cli_read_fds;
	fd_set cli_tmp_fds;
	FD_ZERO(&cli_read_fds);
	FD_ZERO(&cli_tmp_fds);

	// add the server socket and STDIN
	FD_SET(sockfd, &cli_read_fds);
	FD_SET(listen_fd, &cli_read_fds);
	FD_SET(0, &cli_read_fds);

	int fdmax = max(sockfd, listen_fd);

	while (TRUE) {
		// clone the set
		cli_tmp_fds = cli_read_fds;

		result = select(fdmax + 1, &cli_tmp_fds, NULL, NULL, NULL);
		DIE(result < 0, "[client] ERROR: Could not select message\n");
		GOOD("[client] INFO: Successfully selected message\n");

		memset(buffer, 0, BUFLEN);

		if (FD_ISSET(0, &cli_tmp_fds)) {
			// input from STDIN

			result = read(0, buffer, BUFLEN);
			DIE(result < 0, "[client] ERROR: Could not read from STDIN\n");
			GOOD("[client] INFO: Successfully read from STDIN\n");

			// send
			if (con_fd != -1) {
				result = send(con_fd, buffer, strlen(buffer), 0);
				DIE(result < 0, "[client] ERROR: Could not send message "
								"on socket '%d'\n", con_fd);
				GOOD("[client] INFO: Successfully sent message on "
					"socket '%d'\n", con_fd);
			} else {
				result = send(sockfd, buffer, strlen(buffer), 0);
				DIE(result < 0, "[client] ERROR: Could not send message "
								"on socket '%d'\n", sockfd);
				GOOD("[client] INFO: Successfully sent message on "
					"socket '%d'\n", sockfd);
			}

			if (strncmp(buffer, "EXIT", 4) == 0) {
				// exit command
				break;
			}

		} else if (FD_ISSET(listen_fd, &cli_tmp_fds)) {
			// accept new connection
			struct sockaddr_in con_addr;

			struct sockaddr *cast_addr = (struct sockaddr *) &con_addr;
			socklen_t length = sizeof(con_addr);

			con_fd = accept(listen_fd, cast_addr, &length);
			DIE(con_fd < 0, "[client] ERROR: Could not accept the new connection\n");
			GOOD("[client] INFO: Accepted new connection on socket '%d'\n", con_fd);

			// add to set
			FD_SET(con_fd, &cli_read_fds);
		} else {
			// message rom server
			char message[BUFLEN] = {0};
			
			result = recv(sockfd, message, BUFLEN, 0);
			DIE(result < 0, "[client] ERROR: Error while receiving message "
							"on socket '%d'\n", sockfd);
			GOOD("[client] INFO: Successfully received message on "
				"socket '%d'\n", sockfd);

			if (result == 0) {
				// server shut down
				break;
			}

			// connect to another client
			if (strncmp(message, "CONNECT", 7) == 0) {
				GOOD("[client] INFO: Connecting to another client\n");
				// parse the connect payload
				FD_CLR(sockfd, &cli_read_fds);
				close(sockfd);

				// get a new socket
				int cli_fd = socket(AF_INET, SOCK_STREAM, 0);
				DIE(cli_fd < 0, "[client] ERROR: Could not open socket\n");
				GOOD("[client] INFO: Successfully opened socket '%d'\n", cli_fd);

				// fill in the new struct
				struct sockaddr_in cli_addr;
				memset(&cli_addr, 0, sizeof(server_addr));

				int offset = strlen("CONNECT ");

				cli_addr.sin_family = AF_INET;

				memcpy(&(cli_addr.sin_addr), message + offset, sizeof(struct in_addr));
				offset += sizeof(struct in_addr);

				memcpy(&(cli_addr.sin_port), message + offset, sizeof(uint16_t));

				GOOD("[client] INFO: Successfully filled in cli_addr details\n");

				// connect to client
				struct sockaddr *cast_addr = (struct sockaddr *) &cli_addr;

				GOOD("[server] INFO: Client IP '%s' and PORT '%d'\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

				result = connect(cli_fd, cast_addr, SOCKADDR_SIZE);
				DIE(result < 0, "[client] ERROR: Could not connect on "
								"socket '%d'\n", cli_fd);
				GOOD("[client] INFO: Successfully connected to socket '%d'\n", cli_fd);

				FD_SET(cli_fd, &cli_read_fds);
				fdmax = max(fdmax, cli_fd);
			}

			// log message
			printf("%s\n", message);
		}
	}

	// signal shutdown to server
	shutdown(sockfd, SHUT_RDWR);
	close(sockfd);

	GOOD("[client] INFO: Successfully shut down the client\n");
}