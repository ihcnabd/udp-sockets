// src/server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define DEFAULT_PORT 8080
#define BUF_SIZE 1024
#define MAX_CLIENTS 64
#define CLIENT_TIMEOUT 60 /* seconds of silence before a client is forgotten */

/* udp has no connections, so a "client" is just an address we heard from recently */
struct client {
    int in_use;
    struct in_addr addr;
    unsigned short port; /* network byte order, as in sockaddr_in */
    time_t last_seen;
    unsigned long messages;
};

static struct client clients[MAX_CLIENTS];

static int count_clients(void) {
    int count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].in_use) {
            count++;
        }
    }
    return count;
}

/* forget clients that have been silent for too long */
static void expire_clients(time_t now) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        struct client *c = &clients[i];
        if (c->in_use && now - c->last_seen > CLIENT_TIMEOUT) {
            c->in_use = 0;

            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &c->addr, ip, sizeof(ip));
            printf("Client timed out: %s:%d (%d active)\n", ip, ntohs(c->port), count_clients());
        }
    }
}

/* find the entry of this sender, or create one.
   returns NULL if the table is full; *is_new tells if the entry was just created. */
static struct client *track_client(const struct sockaddr_in *from, time_t now, int *is_new) {
    struct client *free_slot = NULL;
    *is_new = 0;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        struct client *c = &clients[i];
        if (!c->in_use) {
            if (free_slot == NULL) {
                free_slot = c;
            }
            continue;
        }
        if (c->addr.s_addr == from->sin_addr.s_addr && c->port == from->sin_port) {
            c->last_seen = now;
            c->messages++;
            return c;
        }
    }

    if (free_slot == NULL) {
        return NULL;
    }
    free_slot->in_use = 1;
    free_slot->addr = from->sin_addr;
    free_slot->port = from->sin_port;
    free_slot->last_seen = now;
    free_slot->messages = 1;
    *is_new = 1;
    return free_slot;
}

int main(int argc, char *argv[]) {
    int port = (argc > 1) ? atoi(argv[1]) : DEFAULT_PORT;
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port: %s\n", argv[1]);
        return 1;
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    /* wake up every second, even when nobody sends, so silent clients expire on time */
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt");
        close(sockfd);
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(sockfd);
        return 1;
    }

    printf("UDP server listening on port %d (tracks up to %d clients)\n", port, MAX_CLIENTS);

    /* one loop serves every client: each datagram carries its sender's address */
    char buffer[BUF_SIZE];
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        ssize_t n = recvfrom(sockfd, buffer, BUF_SIZE - 1, 0,
                             (struct sockaddr *)&client_addr, &client_len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                expire_clients(time(NULL)); /* nothing arrived this second */
            } else {
                perror("recvfrom");
            }
            continue;
        }
        buffer[n] = '\0';

        time_t now = time(NULL);
        expire_clients(now);

        int is_new;
        struct client *c = track_client(&client_addr, now, &is_new);

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        int client_port = ntohs(client_addr.sin_port);

        if (is_new) {
            printf("New client: %s:%d (%d active)\n", ip, client_port, count_clients());
        }
        if (c != NULL) {
            printf("Received from %s:%d (message %lu) -> %s\n", ip, client_port, c->messages, buffer);
        } else {
            printf("Received from %s:%d (not tracked, table full) -> %s\n", ip, client_port, buffer);
        }

        char reply[BUF_SIZE + 16];
        snprintf(reply, sizeof(reply), "ACK: %s", buffer);

        if (sendto(sockfd, reply, strlen(reply), 0,
                   (struct sockaddr *)&client_addr, client_len) < 0) {
            perror("sendto");
        }
    }

    close(sockfd);
    return 0;
}