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

// // Structure to hold client information
// struct client_data
// {
//     int uid;
//     int csock;
//     struct sockaddr_storage storage;
// };

// especificação das mensagens
struct control_command
{
    int idmsg;
    int idsender;
    struct client_data *users_list[15];
};

struct communication_command
{
    int idmsg;
    int idsender;
    int idreceiver;
    char message[BUFSZ];
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

void server_config(int *s, int argc, char **argv)
{

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage))
    {
        usage(argc, argv);
    }

    *s = socket(storage.ss_family, SOCK_STREAM, 0);
    check(*s, "socket creation failed");

    int enable = 1;
    check(setsockopt(*s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)), "setsockopt failed");

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    check(bind(*s, addr, sizeof(storage)), "bind failed");

    check(listen(*s, 10), "listen failed");

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connections\n", addrstr);
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

int check_user_limit(char *buf, int sock)
{

    if (users_count > MAX_CLIENTS)
    {
        // printf("User limit exceeded.\n");
        strcat(buf, "User limit exceeded\n");
        int count = send(sock, buf, strlen(buf) + 1, 0);
        if (count != strlen(buf) + 1)
        {
            logexit("send");
        }
        close(sock);
        pthread_mutex_unlock(&clients_mutex);

        return -1;
    }

    return 0;
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

    // Check user limit
    pthread_mutex_lock(&clients_mutex);

    int limit_exceeded = check_user_limit(buf, cdata->csock);
    if (limit_exceeded == -1)
    {
        printf("User limit exceeded\n");
        return NULL;
    }

    clients[users_count] = cdata->uid;
    clients[users_count] = cdata->csock;
    users_count++;
    pthread_mutex_unlock(&clients_mutex);

    printf("User 0%d added\n", users_count);
    char user_id[2];
    sprintf(user_id, "%d", users_count);
    strcat(buf, "User 0");
    strcat(buf, user_id);
    strcat(buf, " joined the group");

    // Broadcast the received message to all connected clients
    broadcast_msg(buf);

    // close(cdata->csock);

    pthread_exit(EXIT_SUCCESS);
}

void accept_connection(int *s)
{

    while (1)
    {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(*s, caddr, &caddrlen);
        check(csock, "client socket creation failed");

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
