#ifndef _HELPERS_H_
#define _HELPERS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <math.h>

#define MAX_CLIENTS			SOMAXCONN // linux value = 128
#define SOCKADDR_SIZE		sizeof(struct sockaddr)

#define BUFLEN				1000
#define NAMELEN				10
#define COMMANDLEN			10
#define ARGLEN				10

#define TRUE				1
#define FALSE				0

#define LOG					TRUE

// if condition -> display message AND exit program
#define DIE(assertion, fmt, ...)						\
	do {												\
		if (assertion) {								\
			fprintf(stderr, "(%s, %d): " fmt,			\
					__FILE__, __LINE__, ## __VA_ARGS__);\
			exit(EXIT_FAILURE);							\
		}												\
	} while(0)

// if logging is enabled, log the message to stdout
#define GOOD(fmt, ...)									\
	do {												\
		if (LOG) {										\
			printf("(%s, %d): " fmt,					\
					__FILE__, __LINE__, ## __VA_ARGS__);\
		}												\
	} while(0)

int max(int a, int b);

#endif
