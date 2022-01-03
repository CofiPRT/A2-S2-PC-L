/*
 * Protocoale de comunicatii
 * Laborator 7 - TCP
 * Echo Server
 * server.c
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include "helpers.h"

// Primeste date de pe connfd1 si trimite mesajul receptionat pe connfd2, la serverul de echo connfd1 == connfd2
int receive_and_send(int connfd1, int connfd2)
{
    char buf[BUFLEN];
    int bytes_send;
    // int bytes_remaining;
    int bytes_received = 0;

    // send(connfd, buf, BUFLEN, 0);
    // recv(connfd, buf, BUFLEN, 0);

    // TODO 6: Receptionati un mesaj de la connfd1
    bytes_received = recv(connfd1, buf, BUFLEN, 0);

    DIE(bytes_received < 0, "[server] ERROR: Error while receiving message from client %d\n", connfd1);
    printf("[server] INFO: Successfully received message from client %d\n", connfd1);

    // bytes_remaining = bytes_received;

    // TODO 7: Timiteti mesajul la connfd2
    bytes_send = send(connfd2, buf, bytes_received, 0);

    DIE(bytes_send < 0, "[server] ERROR: Error while sending message to client %d\n", connfd2);
    printf("[server] INFO: Successfully sent message to client %d\n", connfd2);

    return bytes_received;
}

void run_echo_server(int listenfd)
{
    struct sockaddr_in client_addr;

    int bytes_received;
    int connfd = -1;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    // TODO 4: Ascultati pentru un singur client pe socketul dat
    int res = listen(listenfd, 1);

    DIE(res < 0, "[server] ERROR: Error while marking the socket as passive\n");
    printf("[server] INFO: Successfully marked the socket as passive\n");

    // TODO 5: Acceptati o conexiune
    connfd = accept(listenfd, (struct sockaddr *) &client_addr, &socket_len);

    DIE(connfd < 0, "[server] ERROR: Error while accepting an incoming connection\n");
    printf("[server] INFO: Successfully accepted an incoming connection\n");

    do {
        bytes_received = receive_and_send(connfd, connfd);
    } while (bytes_received > 0);

    // TODO 8: Inchideti conexiunea si socket-ul clientului
    shutdown(connfd, SHUT_RDWR);
    close(connfd);
}

void run_chat_server(int listenfd)
{
    struct sockaddr_in client_addr1;
    struct sockaddr_in client_addr2;

    int bytes_received;
    int connfd1 = -1;
    int connfd2 = -1;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    // TODO 4: Ascultati pentru doi clineti pe socketul dat
    int res = listen(listenfd, 2);

    DIE(res < 0, "[server] ERROR: Error while marking the socket as passive\n");
    printf("[server] INFO: Successfully marked the socket as passive\n");

    // TODO 5: Acceptati doua conexiuni
    connfd1 = accept(listenfd, (struct sockaddr *) &client_addr1, &socket_len);

    DIE(connfd1 < 0, "[server] ERROR: Error while accepting an incoming connection\n");
    printf("[server] INFO: Successfully accepted an incoming connection\n");

    connfd2 = accept(listenfd, (struct sockaddr *) &client_addr2, &socket_len);

    DIE(connfd2 < 0, "[server] ERROR: Error while accepting another incoming connection\n");
    printf("[server] INFO: Successfully accepted another incoming connection\n");

    do {
        bytes_received = receive_and_send(connfd1, connfd2);

        if (bytes_received == 0)
            break;

        bytes_received = receive_and_send(connfd2, connfd1);
    } while (bytes_received > 0);

    printf("[server] INFO: Client requested shutdown. Stopping server...\n");

    // TODO 8: Inchideti conexiunile si socketii creati
    shutdown(connfd1, SHUT_RDWR);
    close(connfd1);

    shutdown(connfd2, SHUT_RDWR);
    close(connfd2);
}

int main(int argc, char* argv[])
{
    int listenfd = -1;
    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);


    if (argc != 3) {
        printf("\n Usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    // TODO 1: Creati un socket TCP pentru receptionarea conexiunilor
    listenfd = socket(PF_INET, SOCK_STREAM, 0);

    DIE(listenfd < 0, "[server] ERROR: Could not open socket for listening\n");
    printf("[server] INFO: Successfully opened socket for listening\n");

    // TODO 2: Completati in serv_addr adresa serverului pentru bind, familia de adrese si portul rezervat pentru server
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &(serv_addr.sin_addr));

    // TODO 3: Asociati adresa serverului cu socketul creat folosind bind
    int res = bind(listenfd, (struct sockaddr *) &serv_addr, socket_len);

    DIE(res < 0, "[server] ERROR: Could not bind the socket to the port\n");
    printf("[server] INFO: Successfully bound the socket to the port\n");

    //run_echo_server(listenfd);
    run_chat_server(listenfd);


    // TODO 9: Inchideti socketul creat
    close(listenfd);

    printf("[server] INFO: Stopped\n");

    return 0;
}
