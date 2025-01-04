#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <linux/netfilter_ipv4.h>

#define MAX_EVENTS 10
#define BUFFER_SIZE 4096

struct connection {
    int client_fd;
    int server_fd;
    struct sockaddr_in client_addr;
    struct sockaddr_in server_addr;
};

void set_nonblocking(int sock) {
    int opts;
    opts = fcntl(sock, F_GETFL);
    if (opts < 0) {
        perror("fcntl(F_GETFL)");
        exit(EXIT_FAILURE);
    }
    opts = (opts | O_NONBLOCK);
    if (fcntl(sock, F_SETFL, opts) < 0) {
        perror("fcntl(F_SETFL)");
        exit(EXIT_FAILURE);
    }
}

int create_server_socket(int port) {
    int server_fd;
    struct sockaddr_in address;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    set_nonblocking(server_fd);
    return server_fd;
}

int connect_to_server(struct sockaddr_in *dest_addr) {
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)dest_addr, sizeof(*dest_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    set_nonblocking(sockfd);
    return sockfd;
}

void handle_connection(int epollfd, struct connection *conn, int source_fd, int dest_fd) {
    char buffer[BUFFER_SIZE];
    int valread = read(source_fd, buffer, BUFFER_SIZE);
    if (valread > 0) {
        send(dest_fd, buffer, valread, 0);
    } else if (valread == 0) {
        epoll_ctl(epollfd, EPOLL_CTL_DEL, source_fd, NULL);
        epoll_ctl(epollfd, EPOLL_CTL_DEL, dest_fd, NULL);
        close(source_fd);
        close(dest_fd);
        free(conn);
    } else {
        perror("read error");
    }
}

int main(int argc, char const *argv[]) {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    struct epoll_event ev, events[MAX_EVENTS];
    int epollfd;

    server_fd = create_server_socket(8000);

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl: server_fd");
        exit(EXIT_FAILURE);
    }

    while (1) {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == server_fd) {
                new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
                if (new_socket == -1) {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                set_nonblocking(new_socket);

                struct connection *conn = (connection*)malloc(sizeof(struct connection));
                conn->client_fd = new_socket;
                conn->client_addr = address;

                // Get original destination
                socklen_t destlen = sizeof(conn->server_addr);
                getsockopt(new_socket, SOL_IP, SO_ORIGINAL_DST, &conn->server_addr, &destlen);

                conn->server_fd = connect_to_server(&conn->server_addr);
                if (conn->server_fd < 0) {
                    free(conn);
                    close(new_socket);
                    continue;
                }

                ev.events = EPOLLIN | EPOLLET;
                ev.data.ptr = conn;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, new_socket, &ev) == -1) {
                    perror("epoll_ctl: new_socket");
                    exit(EXIT_FAILURE);
                }
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn->server_fd, &ev) == -1) {
                    perror("epoll_ctl: server_fd");
                    exit(EXIT_FAILURE);
                }
            } else {
                struct connection *conn = (connection*)events[n].data.ptr;
                if (events[n].data.fd == conn->client_fd) {
                    handle_connection(epollfd, conn, conn->client_fd, conn->server_fd);
                } else {
                    handle_connection(epollfd, conn, conn->server_fd, conn->client_fd);
                }
            }
        }
    }

    return 0;
}