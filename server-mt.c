#include "common.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#define MAX_CLIENTS 15
#define BUFSZ 1024

// especificação das mensagens
struct control_command
{
    int idmsg;
    int idsender;
    int users_list[15];
};

struct communication_command
{
    int idmsg;
    int idsender;
    int idreceiver;
    char message[BUFSZ];
};

// Structure to hold client information
struct client_data
{
    int uid;
    int csock;
    struct sockaddr_storage storage;
};

int clients[MAX_CLIENTS];                                  // Array to hold client connections
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for thread synchronization
int users_count = 0;

void usage(int argc, char **argv)
{
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

void broadcast_msg(char *buf)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < users_count; i++)
    {
        send(clients[i], buf, strlen(buf), 0);
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *client_thread(void *data)
{
    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);

    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    // printf("[log] connection from %s\n", caddrstr);

    // Add the client socket to the array
    pthread_mutex_lock(&clients_mutex);
    if (users_count > MAX_CLIENTS)
    {
        // printf("User limit exceeded.\n");
        strcat(buf, "User limit exceeded\n");
        int count = send(cdata->csock, buf, strlen(buf) + 1, 0);
        if (count != strlen(buf) + 1)
        {
            logexit("send");
        }
        close(cdata->csock);
        pthread_mutex_unlock(&clients_mutex);
        return NULL;
    }

    clients[users_count++] = cdata->csock;
    pthread_mutex_unlock(&clients_mutex);

    printf("User %d added\n", users_count);
    char user_id[2];
    sprintf(user_id, "%d", users_count);
    strcat(buf, "User ");
    strcat(buf, user_id);
    strcat(buf, " joined the group");

    // for (int i = 0; i <= users_count; i++)
    // {
    //     printf("%d: %d", i, clients[i]->csock);
    // }

    // Broadcast the received message to all connected clients
    broadcast_msg(buf);

    // int count = send(cdata->csock, buf, strlen(buf) + 1, 0);
    // if (count != strlen(buf) + 1)
    // {
    //     logexit("send");
    // }

    // char buf[BUFSZ];
    // memset(buf, 0, BUFSZ);
    // size_t count = recv(cdata->csock, buf, BUFSZ - 1, 0);
    // printf("[msg] %s, %d bytes: %s\n", caddrstr, (int)count, buf);

    // sprintf(buf, "remote endpoint: %.1000s\n", caddrstr);
    // count = send(cdata->csock, buf, strlen(buf) + 1, 0);
    // if (count != strlen(buf) + 1)
    // {
    //     logexit("send");
    // }
    close(cdata->csock);

    pthread_exit(EXIT_SUCCESS);
}

void server_config(int *s, int argc, char **argv)
{

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage))
    {
        usage(argc, argv);
    }

    *s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (*s == -1)
    {
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(*s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)))
    {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(*s, addr, sizeof(storage)))
    {
        logexit("bind");
    }

    if (0 != listen(*s, 10))
    {
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connections\n", addrstr);
}

void accept_connection(int *s)
{

    while (1)
    {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(*s, caddr, &caddrlen);
        if (csock == -1)
        {
            logexit("accept");
        }

        struct client_data *cdata = malloc(sizeof(*cdata));
        if (!cdata)
        {
            logexit("malloc");
        }
        cdata->csock = csock;
        memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, cdata);
    }
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        usage(argc, argv);
    }

    int s = -1;
    server_config(&s, argc, argv);

    accept_connection(&s);

    exit(EXIT_SUCCESS);
}
