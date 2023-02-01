#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h> /* INADDR_ANY */
#include <ctype.h>		/* toupper() */
#include <sys/wait.h>
#include <time.h>
#include <assert.h>

// self-define headers
#include "user_mngmt.h"
#include "map_func.h"
#include "general.h"

// chat with client
void userServe(int __acceptSock)
{
	int i, j;
	int connSock;
	struct sockaddr_in6 clientAddr;
	socklen_t clientAddrLen;
	fd_set rfds, allfds; // allfds includes acceptSock and other connSock!

	FD_ZERO(&rfds);
	FD_ZERO(&allfds);
	FD_SET(__acceptSock, &allfds);

	char rcvRawBuff[BUFFER_MAX];
	char rcvBuff[BUFFER_MAX];
	char sendBuff[BUFFER_MAX];

	int nReady = 0, nBytes = 0;
	int maxfd = __acceptSock;
	for (;;)
	{
		// printf("Waiting...\n");
		rfds = allfds; // copy it
					   // listen to fd attempting to read from acceptSock!!
		if ((nReady = select(maxfd + 1, &rfds, NULL, NULL, NULL)) == -1)
		{
			perror("select");
			exit(1);
		}
		// printf("nReady is %d\n", nReady);
		for (i = 0; i <= maxfd; i++)
		{
			// we got one!!
			if (FD_ISSET(i, &rfds))
			{
				bzero(sendBuff, BUFFER_MAX);
				bzero(rcvBuff, BUFFER_MAX);
				bzero(rcvRawBuff, BUFFER_MAX);

				// handle new connections
				if (i == __acceptSock)
				{
					clientAddrLen = sizeof(clientAddr);
					connSock = accept(__acceptSock, (struct sockaddr *)&clientAddr, &clientAddrLen);
					if (connSock == -1)
					{
						perror("accept");
					}
					else
					{
						FD_SET(connSock, &allfds); // add to allfds set
						if (connSock > maxfd)
						{ // keep track of the max
							maxfd = connSock;
						}
						char addr[INET6_ADDRSTRLEN];
						inet_ntop(AF_INET6, &clientAddr.sin6_addr, addr, sizeof(addr));
						printf("Server: got connection from %s:%d\n", addr, ntohs(clientAddr.sin6_port));
						// printf("connsock: %d\n", connSock);
						addToList(connSock);

						clientInfo *node = findNodeByFd(connSock);
						assert(node != NULL);

						sprintf(sendBuff, "Register(R/r)? / Login(L/l)?:");
						if (encryptSend(connSock, sendBuff) == -1)
						{
							perror("new connection");
						}
					}
				}
				else
				{
					// handle data from a client
					if ((nBytes = recv(i, rcvRawBuff, BUFFER_MAX, 0)) <= 0)
					{
						// got error or connection closed by client
						if (nBytes == 0)
						{
							// connection closed
							printf("Socket %d hung up...Bye :P\n", i);
						}
						else
						{
							perror("recv");
						}
						close(i);			// bye!
						FD_CLR(i, &allfds); // remove from allfds set
						char msg[100];
						clientInfo *node = findNodeByFd(i);
						assert(node != NULL);

						sprintf(msg, "%s is leaving now!", node->uname);
						broadcastExcept(msg, i);
						removeFromList(i); // remove from user list
						maxfd = updateMaxFd();
					}
					else // deal with different commands!
					{
						char *out = NULL;
						decrypt(rcvRawBuff, &out, BUFFER_MAX);
						strcpy(rcvBuff, out);
						free(out);

						// printf("recv: %s\n", rcvBuff);

						clientInfo *node = findNodeByFd(i);
						assert(node != NULL);

						char uname[10] = "";
						char passwd[20] = "";
						switch (node->stage)
						{
						case SUSPEND:
							if (strcasecmp(rcvBuff, "l") == 0)
							{
								sprintf(sendBuff, "Format: uname passwd");
								node->stage = WAITING_TO_LOGIN;
							}
							else if (strcasecmp(rcvBuff, "r") == 0)
							{
								sprintf(sendBuff, "Format: uname passwd");
								node->stage = WAITING_TO_REGISTER;
							}
							else
							{
								sprintf(sendBuff, "Register(R/r)? / Login(L/l)?:");
							}

							if (encryptSend(i, sendBuff) == -1)
							{
								perror("suspend");
							}
							continue;
						case WAITING_TO_LOGIN:
							sscanf(rcvBuff, "%s %s", uname, passwd);
							int ret = userLogin(node, uname, passwd, sendBuff);
							if (encryptSend(i, sendBuff) == -1)
							{
								perror("login");
							}
							if (ret == 0)
							{
								sprintf(sendBuff, "%s has recoverd!", node->uname);
								broadcastExcept(sendBuff, i);
							}
							continue;
						case WAITING_TO_REGISTER:
							sscanf(rcvBuff, "%s %s", uname, passwd);

							userRegister(node, uname, passwd);
							sprintf(sendBuff, "Welcome!~ :>");
							if (encryptSend(i, sendBuff) == -1)
							{
								perror("register");
							}
							continue;
						default:
							break;
						}

						if (strncmp(rcvBuff, "]setName", sizeof("]setName") - 1) == 0)
						{
							char name[10];
							sscanf(rcvBuff, "]setName %s", name);

							bzero(node->uname, sizeof(node->uname));
							strcpy(node->uname, name);
							sprintf(sendBuff, "(server->) OK, your name is '%s' now!", node->uname);
							if (encryptSend(i, sendBuff) == -1)
							{
								perror("setName");
							}
						}
						else if (strncmp(rcvBuff, "]look", sizeof("]look") - 1) == 0)
						{
							int x, y;
							sscanf(rcvBuff, "]look %d %d", &x, &y);

							char playerString[100] = "";
							getPlayersAt(x, y, playerString, i);
							char itemString[100] = "";
							getItemsAt(x, y, itemString);
							sprintf(sendBuff, "Player(s): %s\nItem(s): %s", playerString, itemString);
							if (encryptSend(i, sendBuff) == -1)
							{
								perror("look");
							}
						}
						else if (strncmp(rcvBuff, "]inventory", sizeof("]inventory") - 1) == 0)
						{
							getInventoryByFd(i, sendBuff);
							if (encryptSend(i, sendBuff) == -1)
							{
								perror("inventory");
							}
						}
						else if (strncmp(rcvBuff, "]take", sizeof("]take") - 1) == 0)
						{
							char itemString[10] = "";
							sscanf(rcvBuff, "]take %s", itemString);

							int itemID = convertToInt(itemString);
							int x = node->cord.x;
							int y = node->cord.y;
							int ret = takeItemAt(x, y, itemID);
							switch (ret)
							{
							case 0:
								node->inventory[itemID].n += 1;
								sprintf(sendBuff, "%s took %s!", node->uname, itemString);
								broadcast(sendBuff);
								continue;
								break;
							case -1:
								sprintf(sendBuff, "Find no item named: %s ><", itemString);
								break;
							case 1:
								sprintf(sendBuff, "There's no %s at (%d, %d)", itemString, x, y);
								break;
							default:
								break;
							}

							if (encryptSend(i, sendBuff) == -1)
							{
								perror("take");
							}
						}
						else if (strncmp(rcvBuff, "]deposit", sizeof("]deposit") - 1) == 0)
						{
							char itemString[10] = "";
							sscanf(rcvBuff, "]deposit %s", itemString);

							int itemID = convertToInt(itemString);
							if (node->inventory[itemID].n > 0)
							{
								int x = node->cord.x;
								int y = node->cord.y;
								depositItemAt(x, y, itemID);
								node->inventory[itemID].n -= 1;
								sprintf(sendBuff, "%s deposited %s at (%d, %d)", node->uname, itemString, x, y);
								broadcast(sendBuff);
								continue;
							}
							else
							{
								sprintf(sendBuff, "There's no %s in your backpack!", itemString);
							}

							if (encryptSend(i, sendBuff) == -1)
							{
								perror("deposit");
							}
						}
						else if (strncmp(rcvBuff, "]move", sizeof("]move") - 1) == 0)
						{
							char directionString[10] = "";
							sscanf(rcvBuff, "]move %s\n", directionString);

							int ret = movePositionByFd(i, directionString);
							switch (ret)
							{
							case 0:
								sprintf(sendBuff, "%s move to (%d, %d)!", node->uname, node->cord.x, node->cord.y);
								broadcast(sendBuff);
								continue;
								break;
							case -1:
								sprintf(sendBuff, "OoOoOoOouch! I'm hurt .-.");
								break;
							default:
								break;
							}
							if (encryptSend(i, sendBuff) == -1)
							{
								perror("move");
							}
						}
						else if (strncmp(rcvBuff, "]give", sizeof("]give") - 1) == 0)
						{
							char itemString[10] = "";
							char whom[10];
							sscanf(rcvBuff, "]give %s %s", whom, itemString);

							clientInfo *fromNode = node;
							int itemID = convertToInt(itemString);
							clientInfo *toNode = findNodeByName(whom);
							bool giftHasBeenGiven = false;
							if (fromNode->inventory[itemID].n > 0 && whom != NULL)
							{
								if (inTheSameSpace(fromNode, toNode))
								{
									fromNode->inventory[itemID].n -= 1;
									toNode->inventory[itemID].n += 1;
									sprintf(sendBuff, "You gave %s a %s!", toNode->uname, itemString);
									giftHasBeenGiven = true;
								}
								else
								{
									sprintf(sendBuff, "%s and %s are not in the same space!", fromNode->uname, toNode->uname);
								}
							}
							else
							{
								sprintf(sendBuff, "There's no %s in your backpack OR no one named %s in this room!", itemString, whom);
							}

							if (encryptSend(i, sendBuff) == -1)
							{
								perror("give");
							}
							// also send a message to the client receiving the gift!
							if (giftHasBeenGiven)
							{
								bzero(sendBuff, BUFFER_MAX);
								sprintf(sendBuff, "%s gave %s a %s!", fromNode->uname, toNode->uname, itemString);
								broadcastExcept(sendBuff, i);
							}
						}
						else if (strncmp(rcvBuff, "]tell", sizeof("]tell") - 1) == 0)
						{
							char whom[10];
							char msg[100];
							sscanf(rcvBuff, "]tell %s %99[^\n]", whom, msg);
							clientInfo *toNode = findNodeByName(whom);
							if (toNode == NULL)
							{
								sprintf(sendBuff, "(server->) Sorry, there's no user named %s, use ]who to check who is in the chat room", whom);
								if (encryptSend(i, sendBuff) == -1)
								{
									perror("tell1");
								}
							}
							else
							{
								clientInfo *fromNode = node;
								if (fromNode == toNode)
								{
									sprintf(sendBuff, "Tell a secret to yourself? That's ok...");
								}
								else
								{
									sprintf(sendBuff, "(%s->) %s", fromNode->uname, msg);
								}
								if (encryptSend(toNode->fd, sendBuff) == -1)
								{
									perror("tell2");
								};
							}
						}
						else if (strncmp(rcvBuff, "]who", sizeof("]who") - 1) == 0)
						{
							copyMemberInList(sendBuff);
							if (encryptSend(i, sendBuff) == -1)
							{
								perror("who");
							}
						}
						else if (strncmp(rcvBuff, "]save", sizeof("]save") - 1) == 0)
						{
							int ret = userSave(node);
							if (ret == 0)
							{
								sprintf(sendBuff, "Status saved!");
							}
							else
							{
								sprintf(sendBuff, "Something wrong!");
							}
							if (encryptSend(i, sendBuff) == -1)
							{
								perror("save");
							}
						}
						else if (strncmp(rcvBuff, "]help", sizeof("]help") - 1) == 0)
						{
							strcpy(sendBuff, "===== USAGE ===== \n]setName newname\n]look x y\n]inventory\n]take something\n]deposit something\n]move dir\n]give someone something\n]tell someone msg\n]who\n]save\n]exit\n]help\n=================");
							if (encryptSend(i, sendBuff) == -1)
							{
								perror("help");
							}
						}
						else
						{
							printf("%s\n", rcvBuff);
							strcpy(sendBuff, "Invalid command: type in \"]help\" to check out the usage!");
							if (encryptSend(i, sendBuff) == -1)
							{
								perror("default");
							}
						}
					}
				}
			} // END handle data from client
		}	  // END got new incoming connection
	}		  // END looping through file descriptors
}
int main(int __argc, char **__argv)
{
	// srand(time(NULL));
	buildMap("map.txt");
	printMap();

	struct sockaddr_in6 serverAddr;
	int acceptSock;
	/* create INTERNET,TCP socket */
	acceptSock = socket(AF_INET6, SOCK_STREAM, 0);

	// set socket to be reused
	int optVal = 1;
	// allow the server to bind a port which previous connection exists
	setsockopt(acceptSock, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal));

	serverAddr.sin6_family = AF_INET6;
	serverAddr.sin6_addr = in6addr_any;
	serverAddr.sin6_port = htons(SERVER_PORT);
	/* bind protocol to socket */
	if (bind(acceptSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
	{
		perror("bind");
		exit(1);
	}
	else
	{
		printf("Socket successfully binded...\n");
	}

	// Now server is ready to listen and verification
	if ((listen(acceptSock, 5)) != 0)
	{
		printf("Listen failed...\n");
		exit(0);
	}
	else
	{
		printf("Server listening..\n");
	}
	userServe(acceptSock);
	close(acceptSock); // unreachable
}