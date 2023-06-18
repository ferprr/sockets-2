#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSZ 1024

void usage(int argc, char **argv)
{
	printf("usage: %s <server IP> <server port>\n", argv[0]);
	printf("example: %s 127.0.0.1 51511\n", argv[0]);
	exit(EXIT_FAILURE);
}

void socket_connection(int *s, int argc, char **argv)
{
	struct sockaddr_storage storage;
	if (0 != addrparse(argv[1], argv[2], &storage))
	{
		usage(argc, argv);
	}

	*s = socket(storage.ss_family, SOCK_STREAM, 0);
	check(*s, "socket creation failed");

	struct sockaddr *addr = (struct sockaddr *)(&storage);
	check(connect(*s, addr, sizeof(storage)), "connect failed");

	char addrstr[BUFSZ];
	addrtostr(addr, addrstr, BUFSZ);

	printf("connected to %s\n", addrstr);
}

void send_msg(int s, char *buf)
{
	memset(buf, 0, BUFSZ);
	printf("mensagem> ");
	fgets(buf, BUFSZ - 1, stdin);
	size_t count = send(s, buf, strlen(buf) + 1, 0);
	if (count != strlen(buf) + 1)
	{
		logexit("send");
	}
}

void handle_server_msg(int s, char *buf)
{
	memset(buf, 0, BUFSZ);
	unsigned total = 0;
	while (1)
	{
		size_t count = recv(s, buf + total, BUFSZ - total, 0);
		if (count == 0)
		{
			// Connection terminated.
			break;
		}
		total += count;
	}
	close(s);

	printf("received %u bytes\n", total);
	puts(buf);
}

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		usage(argc, argv);
	}

	int s = -1;
	char buf[BUFSZ];

	socket_connection(&s, argc, argv);

	while (1)
	{
		handle_server_msg(s, buf);
	}

	// send_msg(s, buf);

	exit(EXIT_SUCCESS);
}