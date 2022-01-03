/*
 * Protocoale de comunicatii
 * Laborator 11 - E-mail
 * send_mail.c
 */

#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>  
#include <unistd.h>     
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define SMTP_PORT   25
#define MAXLEN      1000

#define TRUE        1
#define FALSE       0

#define LOG         TRUE

// if condition -> display message AND exit program
#define DIE(assertion, fmt, ...)                        \
    do {                                                \
        if (assertion) {                                \
            fprintf(stderr, "(%s, %d): " fmt,           \
                    __FILE__, __LINE__, ## __VA_ARGS__);\
            exit(EXIT_FAILURE);                         \
        }                                               \
    } while(0)

// if logging is enabled, log the message to stdout
#define GOOD(fmt, ...)                                  \
    do {                                                \
        if (LOG) {                                      \
            printf("(%s, %d): " fmt,                    \
                    __FILE__, __LINE__, ## __VA_ARGS__);\
        }                                               \
    } while(0)

/**
 * Citeste maxim maxlen octeti de pe socket-ul sockfd in
 * buffer-ul vptr. Intoarce numarul de octeti cititi.
 */
ssize_t read_line(int sockd, void *vptr, size_t maxlen)
{
    ssize_t n, rc;
    char c, *buffer;

    buffer = vptr;

    for (n = 1; n < maxlen; n++) {
        if ((rc = read(sockd, &c, 1)) == 1) {
            *buffer++ = c;

            if (c == '\n') {
                break;
            }
        } else if (rc == 0) {
            if (n == 1) {
                return 0;
            } else {
                break;
            }
        } else {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }
    }

    *buffer = 0;
    return n;
}

/**
 * Trimite o comanda SMTP si asteapta raspuns de la server. Comanda
 * trebuie sa fie in buffer-ul sendbuf. Sirul expected contine
 * inceputul raspunsului pe care trebuie sa-l trimita serverul
 * in caz de succes (de exemplu, codul 250). Daca raspunsul
 * semnaleaza o eroare, se iese din program.
 */
void send_command(int sockfd, char sendbuf[], char *expected)
{
    char recvbuf[MAXLEN];
    int nbytes;
    char CRLF[3];
    
    CRLF[0] = 13;
    CRLF[1] = 10;
    CRLF[2] = 0;

    strcat(sendbuf, CRLF);
    GOOD("[client] INFO: Sending to server:\n%s\n", sendbuf);

    write(sockfd, sendbuf, strlen(sendbuf));

    nbytes = read_line(sockfd, recvbuf, MAXLEN - 1);
    recvbuf[nbytes] = 0;
    GOOD("[client] INFO: Server responded with:\n%s\n", recvbuf);

    DIE(strstr(recvbuf, expected) != recvbuf, "[client] ERROR: Received "
        "error from server\n");
}

int main(int argc, char **argv) {
    DIE(argc != 3, "Usage:\n\t./send_msg SERVER_IP FILENAME\n");

    int sockfd;
    int port = SMTP_PORT;
    struct sockaddr_in servaddr;
    char server_ip[INET_ADDRSTRLEN];
    char sendbuf[MAXLEN]; 
    char recvbuf[MAXLEN];

    strncpy(server_ip, argv[1], INET_ADDRSTRLEN);

    // TODO 1: creati socket-ul TCP client
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "[client] ERROR: Could not open socket\n");
    GOOD("[client] INFO: Successfully opened socket '%d'\n", sockfd);

    // TODO 2: completati structura sockaddr_in cu
    // portul si adresa IP ale serverului SMTP
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    int result = inet_aton(server_ip, &servaddr.sin_addr);
    DIE(result == 0, "[client] ERROR: Error in function 'inet_aton', "
                    "called with '%s'\n", server_ip);
    GOOD("[client] INFO: Successfully filled in serv");
        
    // TODO 3: conectati-va la server
    struct sockaddr *cast_addr = (struct sockaddr *) &servaddr;
    socklen_t length = sizeof(servaddr);

    result = connect(sockfd, cast_addr, length);
    DIE(result < 0, "[client] ERROR: Could not connect on "
                    "socket '%d\n", sockfd);
    GOOD("[client] INFO: Successfully connected to socket '%d'\n", sockfd);


    // se primeste mesajul de conectare de la server
    read_line(sockfd, recvbuf, MAXLEN - 1);
    printf("[client] INFO: Received from server:\n%s\n", recvbuf);

    // se trimite comanda de HELO
    sprintf(sendbuf, "HELO localhost");
    send_command(sockfd, sendbuf, "250");

    // TODO 4: trimiteti comanda de MAIL FROM
    sprintf(sendbuf, "MAIL FROM: raressmart@yahoo.com");
    send_command(sockfd, sendbuf, "250");

    // TODO 5: trimiteti comanda de RCPT TO
    sprintf(sendbuf, "RCPT TO: smtpd");
    send_command(sockfd, sendbuf, "250");

    // TODO 6: trimiteti comanda de DATA

    // TODO 7: trimiteti e-mail-ul (antete + corp + atasament)
    sprintf(sendbuf,
        "DATA\r\n"
        "MIME-Version: 1.0\r\n"
        "From: raressmart@yahoo.com\r\n"
        "To: smtdpd\r\n"
        "Subject: Doar o mica treaba in plus\r\n"
        "Content-Type: multipart/mixed; boundary=|||\r\n"
        "|||\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Buna ziua,\r\n"
        "Te rog sa le transmiti si colegilor de grupa ca s-au publicat cele\r\n"
        "4 teme, destul de simple. Nu trebuie decat sa faceti un AI ce\r\n"
        "rezolva problema turing, si alte mici detalii.\r\n"
        "\r\n"
        "Totodata, am publicat cele 10 teste de curs cu deadline maine, pe\r\n"
        "care va rog sa le faceti in proportie de 9/10 pentru a trece\r\n"
        "materia. Va transmit, totodata, ca vom face de acum doar 4 cursuri\r\n"
        "pe saptamana a cate 3 ore fiecare.\r\n"
        "\r\n"
        "Stiu ca aceasta perioada este una grea pentru voi asa ca am\r\n"
        "incercat sa nu va stresez prea mult.\r\n"
        "\r\n"
        "Atasat aveti modalitatea de notare pentru aceasta materie\r\n"
        "\r\n"
        "Cu bine,\r\n"
        "Profesorul\r\n"
        "|||\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Disposition: attachment; filename=\"%s\"\r\n", argv[2]);

    char filebuf[MAXLEN];
    int fd = open(argv[2], O_RDONLY);
    DIE(fd < 0, "[client] ERROR: Could not open file for reading\n");

    result = read(fd, filebuf, MAXLEN);
    DIE(result < 0, "[client] ERROR: Could not read from file\n");

    strncat(sendbuf, filebuf, result);
    send_command(sockfd, sendbuf, "354");

    sprintf(sendbuf, "\r\n.");
    send_command(sockfd, sendbuf, "250");


    // TODO 8: inchideti socket-ul TCP client
    sprintf(sendbuf, "QUIT");
    send_command(sockfd, sendbuf, "221");
    close(sockfd);

    GOOD("[client] INFO: Successfully shut down client\n");

    return 0;
}
