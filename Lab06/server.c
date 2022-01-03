/*
*  	Protocoale de comunicatii: 
*  	Laborator 6: UDP
*	mini-server de backup fisiere
*/

#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>

#include "helpers.h"


void usage(char*file)
{
	fprintf(stderr,"Usage: %s server_port file\n",file);
	exit(0);
}

/*
*	Utilizare: ./server server_port nume_fisier
*/
int main(int argc,char**argv)
{
	if (argc!=3)
		usage(argv[0]);
	
	struct sockaddr_in my_sockaddr, from_station ;
	char buf[BUFLEN];


	/*Deschidere socket*/
	int sockid = socket(PF_INET, SOCK_DGRAM, 0);

	if (sockid < 0) {
		fprintf(stderr, "[server] ERROR: Could not open socket\n");
		exit(-1);
	}

	printf("[server] INFO: Successfully opened socket\n");
	
	/*Setare struct sockaddr_in pentru a asculta pe portul respectiv */
	my_sockaddr.sin_family = AF_INET;
	my_sockaddr.sin_port = htons(atoi(argv[1]));
	my_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	/* Legare proprietati de socket */
	bind(sockid, (struct sockaddr *) &my_sockaddr, sizeof(struct sockaddr_in));
	
	
	/* Deschidere fisier pentru scriere */
	int source_file = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);

	if (source_file < 0) {
		fprintf(stderr, "[server] ERROR: Could not open file '%s'\n", argv[2]);
		exit(-2);
	}

	printf("[server] INFO: Successfully opened file '%s'\n", argv[2]);
	
	/*
	*  cat_timp  mai_pot_citi
	*		citeste din socket
	*		pune in fisier
	*/

	int write_index = 1;

	while (1) {
		socklen_t addrlen;

		int bytes_received = recvfrom(sockid, buf, BUFLEN, 0, (struct sockaddr *)&from_station, &addrlen);

		if (bytes_received < 0) {
			fprintf(stderr, "[server] ERROR: Error while receiving message\n");
			exit(-3);
		}

		if (bytes_received == 0) {
			/* UDP: datagrams of length 0 are acceptable */
			// inchide serverul
			printf("[server] INFO: Datagram of length 0 received from port '%d'. Stopping server...\n", ntohs(from_station.sin_port));
			break;
		}

		printf("[server] INFO: Successfully received message from port '%d'\n", ntohs(from_station.sin_port));

		int bytes_written = write(source_file, buf, bytes_received);

		if (bytes_written < 0) {
			fprintf(stderr, "[server] ERROR: Could not write to file '%s'\n", argv[2]);
			exit(-4);
		}

		printf("[server] INFO: Successfully written message no. '%d' to file '%s'\n", write_index, argv[2]);

		write_index++;
	}

	


	/*Inchidere socket*/	
	close(sockid);

	
	/*Inchidere fisier*/
	close(source_file);

	return 0;
}
