#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

#define PORT_1 8080
#define PORT_2 80

#define HOST_1 "ec2-3-8-116-10.eu-west-2.compute.amazonaws.com"
#define HOST_2 "api.openweathermap.org"

#define IP_2 "188.166.16.132"

#define CONTENT_TYPE "application/x-www-form-urlencoded"

#define PATH_1 "/api/v1/dummy"
#define PATH_2 "/api/v1/auth/login"
#define PATH_3 "/api/v1/auth/logout"
#define PATH_4 "/api/v1/weather/key"
#define PATH_5 "/data/2.5/weather"

#define MAXLEN 100

struct sockaddr *get_addr(char *name) {
    struct addrinfo hints;
    struct addrinfo *result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    if (getaddrinfo(name, NULL, &hints, &result)) {
        perror("getaddrinfo");
    }

    return result->ai_addr;
}

int main(int argc, char *argv[])
{
    int i;

    char *message;
    char *response;

    struct sockaddr_in *serv_addr = (struct sockaddr_in *) get_addr(HOST_1);

    char *ip = calloc(MAXLEN, 1);
    inet_ntop(AF_INET, &(serv_addr->sin_addr), ip, MAXLEN);

    int sockfd = open_connection(ip, PORT_1, AF_INET, SOCK_STREAM, 0);

    free(ip);

        
    // Ex 1.1: GET dummy from main server
    printf("[CLIENT] Sending GET request\n\n");
    message = compute_get_request(HOST_1, PATH_1, NULL, NULL, 0);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("[CLIENT] Received response:\n%s\n\n\n\n\n", response);
    free(message);
    free(response);


    // Ex 1.2: POST dummy and print response from main server
    char **body_data = malloc(MAXLEN * sizeof(char *));
    for (i = 0; i < MAXLEN; i++) {
        body_data[i] = calloc(LINELEN, 1);
    }

    int body_data_lines = 1;
    sprintf(body_data[0], "stircu=amuzant");


    // POST
    printf("[CLIENT] Sending POST request\n\n");
    message = compute_post_request(HOST_1, PATH_1, CONTENT_TYPE,
                body_data, body_data_lines, NULL, 0);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("[CLIENT] Received response:\n%s\n\n\n\n\n", response);
    free(message);
    free(response);


    // Ex 2: Login into main server
    body_data_lines = 2;
    sprintf(body_data[0], "username=student");
    sprintf(body_data[1], "password=student");

    printf("[CLIENT] Attempting login\n\n");
    message = compute_post_request(HOST_1, PATH_2, CONTENT_TYPE,
                body_data, body_data_lines, NULL, 0);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("[CLIENT] Received response:\n%s\n\n\n\n\n", response);



    // COOKIE
    char *cookie = calloc(LINELEN, 1);

    char *start = strstr(response, "Set-Cookie: ");

    start += strlen("Set-Cookie: ");

    char *end = strstr(start, "\r\n");

    int diff = end - start;
    strncpy(cookie, start, diff);

    printf("[CLIENT] Response cookie:\n%s\n\n", cookie);
    free(message);
    free(response);



    char **cookies = malloc(MAXLEN * sizeof(char *));
    for (i = 0; i < MAXLEN; i++) {
        cookies[i] = calloc(LINELEN, 1);
    }


    // Ex 3: GET weather key from main server
    int cookie_count = 1;
    sprintf(cookies[0], "%s", cookie);

    printf("[CLIENT] Sending GET request with cookie\n\n");
    message = compute_get_request(HOST_1, PATH_4, 0,
        cookies, cookie_count);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("[CLIENT] Received response:\n%s\n\n\n\n\n", response);


    // KEY
    char *key = calloc(LINELEN, 1);

    start = strstr(response, "\"key\":\"");

    if (start == NULL) {
        perror("[CLIENT] ERROR: No key received. Is the server working alright?\n\n\n");
        exit(-1);
    }

    start += strlen("\"key\":\"");

    end = strstr(start, "\"");

    diff = end - start;
    strncpy(key, start, end - start);

    printf("[CLIENT] Response key:\n%s\n\n", key);
    free(message);
    free(response);



    printf("[CLIENT] ///// BONUS /////\n\n");


    // Ex 4: GET weather data from OpenWeather API
    int sockfd_2 = open_connection(IP_2, PORT_2, AF_INET, SOCK_STREAM, 0);


    char *query = calloc(LINELEN, 1);

    char *lat = calloc(MAXLEN, 1);
    char *lon = calloc(MAXLEN, 1);

    sprintf(lat, "43.8152");
    sprintf(lon, "28.5741");

    printf("[CLIENT] Sending GET request with key\n\n");
    sprintf(query, "lat=%s&lon=%s&appid=%s", lat, lon, key);
    message = compute_get_request(HOST_2, PATH_5, query, NULL, 0);

    send_to_server(sockfd_2, message);
    response = receive_from_server(sockfd_2);
    printf("[CLIENT] Received response:\n%s\n\n\n\n\n", response);



    // Ex 5: POST weather data for verification to main server
    char *weather_path = calloc(MAXLEN, 1);

    printf("[CLIENT] Sending POST request for validation\n\n");
    sprintf(weather_path, "/api/v1/weather/%s/%s", lat, lon);

    body_data_lines = 1;
    sprintf(body_data[0], "%s", basic_extract_json_response(response));
    free(message);
    free(response);
    free(query);

    cookie_count = 1;
    sprintf(cookies[0], "%s", cookie);
    
    message = compute_post_request(HOST_1, weather_path, "application/json",
        body_data, body_data_lines, cookies, cookie_count);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("[CLIENT] Received response:\n%s\n\n\n\n\n", response);
    free(message);
    free(response);
    free(weather_path);


    // BONUS: login again
    printf("[CLIENT] BONUS: Attempting login AGAIN just to annoy the server\n\n");

    body_data_lines = 2;
    sprintf(body_data[0], "username=student");
    sprintf(body_data[1], "password=student");

    cookie_count = 1;
    sprintf(cookies[0], "%s", cookie);

    message = compute_post_request(HOST_1, PATH_2, CONTENT_TYPE,
                body_data, body_data_lines, cookies, cookie_count);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("[CLIENT] Received response:\n%s\n\n\n\n\n", response);
    free(message);
    free(response);


    // Ex 6: Logout from main server
    printf("[CLIENT] Sending GET request to logout\n\n");

    cookie_count = 1;
    sprintf(cookies[0], "%s", cookie);
    message = compute_get_request(HOST_1, PATH_3, NULL,
        cookies, cookie_count);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("[CLIENT] Received response:\n%s\n\n\n\n\n", response);
    free(message);
    free(response);
    free(cookie);

    for (i = 0; i < MAXLEN; i++) {
        free(body_data[i]);
        free(cookies[i]);
    }

    // BONUS: make the main server return "Already logged in!"

    // free the allocated data at the end!
    close_connection(sockfd);
    close_connection(sockfd_2);

    return 0;
}
