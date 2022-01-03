#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/*
 * Macro de verificare a erorilor
 * Exemplu:
 * 		int fd = open (file_name , O_RDONLY);
 * 		DIE( fd == -1, "open failed");
 */

#define DIE(assertion, fmt, ...)						\
	do {												\
		if (assertion) {								\
			fprintf(stderr, "(%s, %d): " fmt,			\
					__FILE__, __LINE__, ## __VA_ARGS__);\
			perror("");									\
			exit(EXIT_FAILURE);							\
		}												\
	} while(0)

/* Dimensiunea maxima a calupului de date */
#define BUFLEN 1500

#endif
