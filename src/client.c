// src/client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT 8080
#define BUF_SIZE 1024
#define TIMEOUT_SEC 3

int main(int argc, char *argv[]) {
    const char *server_ip = (argc > 1) ? argv[1] : DEFAULT_IP;
    int port = (argc > 2) ? atoi(argv[2]) : DEFAULT_PORT;
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port: %s\n", argv[2]);
        return 1;
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    // UDP has no delivery guarantee, so don't wait forever for a reply
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt");
        close(sockfd);
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid IP address: %s\n", server_ip);
        close(sockfd);
        return 1;
    }

    printf("UDP client -> %s:%d (type 'quit' to exit)\n", server_ip, port);

    char message[BUF_SIZE];
    char buffer[BUF_SIZE];
    while (1) {
        printf("> ");
        fflush(stdout);
        if (fgets(message, sizeof(message), stdin) == NULL) {
            break;
        }
        message[strcspn(message, "\n")] = '\0';

        if (strcmp(message, "quit") == 0) {
            break;
        }
        if (message[0] == '\0') {
            continue;
        }

        if (sendto(sockfd, message, strlen(message), 0,
                   (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("sendto");
            continue;
        }

        ssize_t n = recvfrom(sockfd, buffer, BUF_SIZE - 1, 0, NULL, NULL);
        if (n < 0) {
            fprintf(stderr, "No reply from server (timeout after %d s)\n", TIMEOUT_SEC);
            continue;
        }
        buffer[n] = '\0';
        printf("Server replied: %s\n", buffer);
    }

    close(sockfd);
    return 0;
}
