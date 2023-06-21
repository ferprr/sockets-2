#pragma once

#include <stdlib.h>

#include <arpa/inet.h>

// Structure to hold client information
struct client_data
{
    int uid;
    int csock;
    struct sockaddr_storage storage;
};

int check(int ret, const char *msg);

void logexit(const char *msg);

int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage);

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage);
