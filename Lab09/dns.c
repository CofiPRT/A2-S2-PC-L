// Protocoale de comunicatii
// Laborator 9 - DNS
// dns.c

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

int usage(char* name)
{
	printf("Usage:\n\t%s -n <NAME>\n\t%s -a <IP>\n", name, name);
	return 1;
}

// Receives a name and prints IP addresses
void get_ip(char* name)
{
	int ret;
	struct addrinfo hints, *result, *p;

	// TODO: set hints
	memset(&hints, 0, sizeof(struct addrinfo));

	hints.ai_flags = AI_CANONNAME;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	// TODO: get addresses
	ret = getaddrinfo(name, NULL, &hints, &result);

	if (ret < 0) {
		fprintf(stderr,
			"[DNS] ERROR: Error while getting address info for '%s'\n", name);
		exit(-1);
	}

	printf("[DNS] INFO: Successfully got address info for '%s'\n", name);

	// TODO: iterate through addresses and print them
	printf("[DNS] INFO: Addresses:\n");

	p = result;
	while (p) {
		if (p->ai_family == AF_INET) {
			char ip[255];
			inet_ntop(AF_INET,
				&((struct sockaddr_in *) p->ai_addr)->sin_addr.s_addr,
				ip,
				p->ai_addrlen);

			printf("\t%s\n", ip);
		}

		p = p->ai_next;
	}

	// TODO: free allocated data
	freeaddrinfo(result);
}

// Receives an address and prints the associated name and service
void get_name(char* ip)
{
	int ret;
	struct sockaddr_in addr;
	char host[1024];
	char service[20];

	// TODO: fill in address data
	memset(&addr, 0, sizeof(struct sockaddr_in));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	inet_aton(ip, &addr.sin_addr);

	// TODO: get name and service
	ret = getnameinfo((struct sockaddr *) &addr,
					sizeof(addr),
					host,
					1024,
					service,
					20,
					0);

	if (ret < 0) {
		fprintf(stderr,
			"[DNS] ERROR: Error while getting name info and service\n");
	}

	// TODO: print name and service
	printf("[DNS] INFO: Successfully got name info and service:\n"
			"\t%s %s\n", host, service);
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		return usage(argv[0]);
	}

	if (strncmp(argv[1], "-n", 2) == 0) {
		get_ip(argv[2]);
	} else if (strncmp(argv[1], "-a", 2) == 0) {
		get_name(argv[2]);
	} else {
		return usage(argv[0]);
	}

	return 0;
}
