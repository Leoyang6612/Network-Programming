#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h> /* INADDR_ANY */
#include <netdb.h>
#include <ctype.h> /* toupper() */
#include <sys/wait.h>
#include <fcntl.h>
#include <openssl/aes.h>

// self-define headers
#include "map_func.h"
#include "general.h"

void chat(int __connSock)
{

	int n, maxfd = -1;
	fd_set rfds;
	char message[BUFFER_MAX];

	if (__connSock > maxfd)
	{
		maxfd = __connSock;
	}

	while (1)
	{
		FD_ZERO(&rfds);
		FD_SET(STDIN_FILENO, &rfds);
		FD_SET(__connSock, &rfds);
		// printf("Waiting...\n");
		n = select(maxfd + 1, &rfds, NULL, NULL, NULL);
		if (n == -1)
		{
			printf("ERROR: client select failed\n");
			break;
		}
		else if (n == 0)
		{
			continue;
		}
		else
		{
			// check inputs of client
			if (FD_ISSET(STDIN_FILENO, &rfds))
			{
				bzero(message, BUFFER_MAX);
				fgets(message, BUFFER_MAX, stdin);
				// we don't have to send '\n'(new line character)!
				message[strlen(message) - 1] = '\0';
				if (strcasecmp(message, "]exit") == 0)
					break;
				encryptSend(__connSock, message);
				if (n < 0)
				{
					printf("ERROR: message was not sent to server\n");
				}
				else
				{
					// printf("Send %d bytes to server: %s\n", n, message);
				}
			}
			// check message received from server
			if (FD_ISSET(__connSock, &rfds))
			{
				bzero(message, BUFFER_MAX);
				n = recv(__connSock, message, BUFFER_MAX, 0);
				if (n > 0)
				{
					// printf("Receive %d bytes from server: \n%s\n", n, message);
					char *dec_out = NULL;
					decrypt(message, &dec_out, BUFFER_MAX);
					printf("%s\n", dec_out);
					free(dec_out);
				}
				else
				{
					if (n < 0)
						printf("ERROR: no message received from server\n");
				}
			}
		}
	}
}

int main(int __argc, char **__argv)
{
	if (__argc != 3)
	{
		printf("FORMAT: ./chat_client [ip-address] [dst-port-number]\n");
		exit(1);
	}
	int connSock, rc;
	char server[100] = "";
	struct in6_addr serverAddr;
	struct addrinfo hints, *res = NULL;

	strcpy(server, __argv[1]);

	memset(&hints, 0x00, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	rc = inet_pton(AF_INET, server, &serverAddr);
	if (rc == 1) /* valid IPv4 text address? */
	{
		hints.ai_family = AF_INET;
	}
	else
	{
		rc = inet_pton(AF_INET6, server, &serverAddr);
		if (rc == 1) /* valid IPv6 text address? */
		{
			hints.ai_family = AF_INET6;
		}
	}

	rc = getaddrinfo(server, __argv[2], &hints, &res);
	if (rc != 0)
	{
		printf("Host not found --> %s\n", gai_strerror(rc));
		if (rc == EAI_SYSTEM)
			perror("getaddrinfo() failed");
	}

	connSock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if (connect(connSock, res->ai_addr, res->ai_addrlen) < 0)
	{
		perror("connect");
		exit(2);
	}
	else
	{
		printf("Connection established. Input \"exit/EXIT\" to leave!!\n");
	}

	chat(connSock);
	close(connSock);
	if (res != NULL)
		freeaddrinfo(res);
}