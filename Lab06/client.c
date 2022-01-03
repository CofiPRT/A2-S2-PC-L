/*
*  	Protocoale de comunicatii: 
*  	Laborator 6: UDP
*	client mini-server de backup fisiere
*/

#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "helpers.h"

void usage(char*file)
{
	fprintf(stderr,"Usage: %s ip_server port_server file\n",file);
	exit(0);
}

/*
*	Utilizare: ./client ip_server port_server nume_fisier_trimis
*/
int main(int argc,char**argv)
{
	if (argc < 4)
		usage(argv[0]);
	
	struct sockaddr_in to_station;
	char buf[BUFLEN];


	/*Deschidere socket*/
	int sockid = socket(PF_INET, SOCK_DGRAM, 0);

	if (sockid < 0) {
		fprintf(stderr, "[client] ERROR: Could not open socket\n");
		exit(-1);
	}

	printf("[client] INFO: Successfully opened socket\n");
	
	/*Setare struct sockaddr_in pentru a specifica unde trimit datele*/
	to_station.sin_family = AF_INET;
	to_station.sin_port = htons(atoi(argv[2]));
	inet_aton(argv[1], &(to_station.sin_addr));

	int total_chunks = 0;

	int i;
	for (i = 3; i < argc; i++) {
		/* Deschidere fisier pentru citire */
		int source_file = open(argv[i], O_RDONLY);

		if (source_file < 0) {
			fprintf(stderr, "[client] ERROR: Could not open file '%s'\n", argv[i]);
			exit(-2);
		}

		printf("[client] INFO: Successfully opened file '%s'\n", argv[i]);

		int file_size = lseek(source_file, 0, SEEK_END);
		lseek(source_file, 0, SEEK_SET);

		int message_index = 1;

		/*
		 *  cat_timp  mai_pot_citi
		 *		citeste din fisier
		 *		trimite pe socket
		 */

		while (lseek(source_file, 0, SEEK_CUR) < file_size) {
			int bytes_read = read(source_file, buf, BUFLEN);

			if (bytes_read < 0) {
				fprintf(stderr, "[client] ERROR: Could not read from file '%s', message %d\n", argv[i], message_index);
				exit(-3);
			}

			printf("[client] INFO: Successfully read chunk '%d' from file '%s'\n", message_index, argv[i]);

			int bytes_sent = sendto(sockid, buf, bytes_read, 0, (struct sockaddr *) &to_station, sizeof(struct sockaddr_in));

			if (bytes_sent < 0) {
				fprintf(stderr, "[client] ERROR: Error while sending message '%d'\n", message_index);
				exit(-4);
			}

			printf("[client] INFO: Successfully sent message '%d'\n", message_index);

			message_index++;
			total_chunks++;
		}

		close(source_file);
	}
	
	printf("[client] INFO: Total chunks sent: %d\n", total_chunks);
	// anunta ca nu se mai trimit fisiere
	sendto(sockid, "", 0, 0, (struct sockaddr *) &to_station, sizeof(struct sockaddr_in));


	/*Inchidere socket*/
	close(sockid);

	return 0;
}
