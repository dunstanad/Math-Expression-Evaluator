#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/epoll.h>
#include "eval.h"
#include "stack.h"

#define PORT "3490"
#define BACKLOG 10 // how many pending connections queue will hold
#define BUFFERSIZE 1024
#define MAX_EVENTS 20

// getaddrinfo() -> socket() -> setsockopt() -> bind() -> listen() -> accept() -> send() , recv() ->  close()

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{ // IPv4
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}
	// IPv6
	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

// helper to set a socket non-blocking
static int non_blocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// helper to check if buffer has continue msg
bool continue_msg(char *buffer){
	return (
		strcmp("Yes", buffer) == 0 || strcmp("YES", buffer) == 0 ||
						 strcmp("yes", buffer) == 0 || strcmp("y", buffer) == 0 || strcmp("Y", buffer) == 0 ||
		strcmp("quit", buffer) == 0 || strcmp("exit", buffer) == 0 ||
					strcmp("No", buffer) == 0 || strcmp("NO", buffer) == 0 ||
					strcmp("no", buffer) == 0 || strcmp("n", buffer) == 0 || strcmp("N", buffer) == 0
				);
}


int main()
{
	struct addrinfo hints, *res, *p;
	int rv;
	int listen_fd; // for binding, listening
	int opt = 1;
	int numbytes = 0;
	char buffer[BUFFERSIZE];
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use local IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &res)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for (p = res; p != NULL; p = p->ai_next)
	{
		if ((listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("Server : socket");
			continue;
		}
		// To prevent Address already in use error
		if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		{
			perror("Server : setsockopt");
			exit(1);
		}

		if (bind(listen_fd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(listen_fd);
			perror("Server : bind");
			continue;
		}
		break;
	}

	if (p == NULL)
	{
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(listen_fd, BACKLOG) == -1)
	{
		perror("listen");
		exit(1);
	}
	non_blocking(listen_fd); // make listening socket non-blocking

	// create epoll instance, watching the listening socket
	struct epoll_event ev, events[MAX_EVENTS];
	int epfd, nfds, i;

	epfd = epoll_create1(0);
	ev.events = EPOLLIN; // ready to read (i.e new connections)
	ev.data.fd = listen_fd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);
	printf("Server started on port %s. Waiting for connections...\n", PORT);

	// event loop
	while (1)
	{
		nfds = epoll_wait(epfd, events, MAX_EVENTS, 5000); // wait 5 seconds
		if (nfds < 0)
		{
			if (errno == EINTR)
				continue;
			perror("epoll_wait");
			break;
		}
		for (i = 0; i < nfds; i++)
		{
			int fd = events[i].data.fd;

			// a) new incoming connection
			if (fd == listen_fd)
			{
				struct sockaddr_storage cli_addr; // store client address
				socklen_t cli_len = sizeof(cli_addr);
				int conn_fd = accept(listen_fd, (struct sockaddr *)&cli_addr, &cli_len);

				if (conn_fd < 0)
					continue;
				non_blocking(conn_fd);
				ev.events = EPOLLIN | EPOLLET; // edge-triggered
				ev.data.fd = conn_fd;
				epoll_ctl(epfd, EPOLL_CTL_ADD, conn_fd, &ev);

				inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)&cli_addr), s, sizeof(s));
				printf("New connection from %s\n", s);
				// send welcome message
				sprintf(buffer, "Welcome to the Math Expression Evaluator!!!\n"
						"RULES:\n"
						"1. Do NOT use white spaces between a number (2  56)\n"
						"2. Use ONLY these operators(+,-,*,/,%%)\n"
						"3. Use ONLY Integers\n"
                        "4. To exit, enter(quit/exit/N/n)\n"
						"Please enter your Math Expression below:\n");
				send(conn_fd, buffer, strlen(buffer), 0);
				continue;
			}

			// b) data from client
			memset(buffer, 0, BUFFERSIZE);
			int n = recv(fd, buffer, BUFFERSIZE - 1, 0);
			if (n < 0)
			{
				epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
				close(fd);
				continue;
			}

			buffer[n] = '\0';
			// trim newline
			if (buffer[n - 1] == '\n')
				buffer[n - 1] = '\0';

			if (!continue_msg(buffer) && !ambiguous_spacing(buffer)) // no ambiguous spacing
			{
				removeSpaces(buffer);
                if(validate_infix(buffer)){ // send result
                    long result =  solve(buffer);
                    char out[BUFFERSIZE];
				    int len = snprintf(out, BUFFERSIZE, "Result of %s is %ld\nWould you like to continue? (y/n)\n", buffer, result);
				    send(fd, out, len, 0);
                }
                else{// invalid infix expression
                    const char *tryagain = "INVALID INPUT!!! Please enter a valid infix expression or (y/n)\nTry again\n";
					send(fd, tryagain, strlen(tryagain), 0);
                }
			}
            else if(ambiguous_spacing(buffer)){ // includes ambiguous spacing
                const char *tryagain = "INVALID INPUT(includes ambiguous spacing)!!! Please enter a valid math infix expression\nTry again\n";
				send(fd, tryagain, strlen(tryagain), 0);
            }
			else
			{
				// expecting 'y' or 'n' or invalid input
				if (strcmp("quit", buffer) == 0 || strcmp("exit", buffer) == 0 ||
					strcmp("No", buffer) == 0 || strcmp("NO", buffer) == 0 ||
					strcmp("no", buffer) == 0 || strcmp("n", buffer) == 0 || strcmp("N", buffer) == 0)
				{
					// goodbye
					const char *goodbye_msg = "Goodbye!\n";
					send(fd, goodbye_msg, strlen(goodbye_msg), 0);
					printf("Closing connection with client\n");
					epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
					close(fd);
				}
				else if (strcmp("Yes", buffer) == 0 || strcmp("YES", buffer) == 0 ||
						 strcmp("yes", buffer) == 0 || strcmp("y", buffer) == 0 || strcmp("Y", buffer) == 0)
				{
					// ask for number
					const char *prompt = "Please enter your Math Expression below\n";
					send(fd, prompt, strlen(prompt), 0);
				}
				else
				{
					// try again
					const char *tryagain = "INVALID INPUT!!! Please enter a valid math infix expression or (y/n)\nTry again\n";
					send(fd, tryagain, strlen(tryagain), 0);
				}
			}
		}
	}

	freeaddrinfo(res);
	close(listen_fd); // Close listening socket
	close(epfd);

	return 0;
}