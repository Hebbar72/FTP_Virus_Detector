#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#define MAX_EVENTS 10


int set_socket_non_blocking(int socket_fd)
 {
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1) 
    {
        perror("fcntl F_GETFL");
        return -1;
    }

    flags |= O_NONBLOCK;
    if (fcntl(socket_fd, F_SETFL, flags) == -1) 
    {
        perror("fcntl F_SETFL O_NONBLOCK");
        return -1;
    }

    return 0;
}

int main(int argc, char* args[])
{
    char* ip = args[1];
    int port = htons(atoi(args[2]));

    sockaddr_in* router_addr = new sockaddr_in();
    int addr_len = sizeof(*router_addr);
    int router_socket;
    router_addr->sin_family = AF_INET;
    router_addr->sin_port = port;
    inet_pton(AF_INET, ip, &router_addr->sin_addr);
    router_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(router_socket < 0)
    {
        perror("Error Creating Router Socket");
        exit(1);
    }

    if(set_socket_non_blocking(router_socket) < 0)
    {
        exit(1);
    }

    if(bind(router_socket, (sockaddr*)router_addr, sizeof(*router_addr)) < 0)
    {
        perror("Error Binding Router Socket");
        close(router_socket);
        exit(1);
    }
    
    if(listen(router_socket, 5) < 0)
    {
        perror("Error Listening Router Socket");
        close(router_socket);
        exit(1);
    }

    int epoll_fd;
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    struct epoll_event events[MAX_EVENTS];
    epoll_fd = epoll_create1(0);
    if(epoll_fd < 0)
    {
        perror("Error Epoll_fd Creation");
        exit(1);
    }

    ev.data.fd = router_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, router_socket, &ev) < 0) {
        perror("epoll_ctl: server_fd");
        exit(1);
    }

    sockaddr_in client_addr;
    int client_socket;
    while(1)
    {
	printf("magane\n");
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }
	printf("mahisha\n");

        for (int n = 0; n < nfds; ++n) 
        {
            if (events[n].data.fd == router_socket) 
            {
                printf("router_socket\n");
                client_socket = accept(router_socket, (struct sockaddr *)&client_addr, (socklen_t*)&addr_len);
                if(client_socket < 0) 
                {
                    continue;
                }

                set_socket_non_blocking(client_socket);
		ev.data.fd = client_socket;
                if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &ev) == -1) 
                {
                    perror("epoll_ctl: new_socket");
                    exit(EXIT_FAILURE);
                }
            }
            else 
            {
                printf("client_socket");
                char buffer[1024] = "";

                int x = recv(events[n].data.fd, buffer, 1024, 0);

                if(x > 0)
                    printf("%s\n", buffer);
                else if(x == 0)
                {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[n].data.fd, NULL);
                    close(events[n].data.fd);
                }
                else 
                {
                    perror("No Data Available");
                }
            }
        }
    }
}
