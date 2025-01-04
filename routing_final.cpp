#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
#include <thread>

#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <linux/netfilter_ipv4.h>

#define MAX_EVENTS 10

#ifndef SLEEP_TIME
    #define SLEEP_TIME 1000
#endif

using namespace std;

unordered_map<int, bool> socket_active;
unordered_map<string, pair<int, int>> conn_map;
unordered_map<int, string> response_map; 
unordered_map<int, string> helper_map;

sem_t sem_active, sem_response;

struct connection
{
    string src_ip;
    string dst_ip;
    int src_port;
    int dst_port;
    int src_socket;
    int dst_socket;
    int trigger;
};

void send_file(string file_name, int dest, int src, int dest_socket, int src_socket, int dir)
{
    sem_wait(&sem_response);
	string response_msg = response_map[src];
    sem_post(&sem_response);
    string cmd = "sudo clamdscan --fdpass " + file_name;
    int virus_detected;
 	do 
    {
        virus_detected = system(cmd.c_str());
    } while (virus_detected == -1 && errno == EINTR);

    FILE* file = fopen(file_name.c_str(), "rb");

    try
    {
	    if(virus_detected)
		    throw runtime_error("426 Virus Detected");
	
        char buffer[1024] = "";
        int x = fread(buffer, 1, 1024, file);
        while(x)
        {
            if(send(dest_socket, buffer, x, 0) < 0)
		        perror("Error thread send");
            x = fread(buffer, 1, 1024, file);
        }  

        string resp_temp;
        while(dir)
        {
            sem_wait(&sem_response);
            resp_temp = response_map[src];
            sem_post(&sem_response);
            if(resp_temp.find("150") != 0)
                break;
            usleep(SLEEP_TIME);
        }
        close(dest_socket);
	    close(src_socket);
        
        if(dir)
            send(src, resp_temp.c_str(), resp_temp.length(), 0);

        fclose(file);
	
        remove(file_name.c_str());
        sem_wait(&sem_active);
	    socket_active[src] = true;
        socket_active[dest] = true;
        sem_post(&sem_active);
    }

    catch(const std::exception& e)
    {
	    string resp_temp;
        while(dir)
        {
            sem_wait(&sem_response);
            resp_temp = response_map[src];
            sem_post(&sem_response);
            if(resp_temp.find("150") != 0)
                break;
            usleep(SLEEP_TIME);
        }
	    send(src, "1451 VIRUS DETECTED.\n\0", strlen("1451 VIRUS DETECTED.\n\0"), 0);

        if(dir)
        {
            sem_wait(&sem_active);
            socket_active[src] = true;
            socket_active[dest] = true;
            sem_post(&sem_active);
        }
    	close(dest_socket);
        close(src_socket);

        while(!dir)
        {
            sem_wait(&sem_response);
            resp_temp = response_map[src];
            sem_post(&sem_response);
            if(resp_temp.find("150") != 0)
                break;
            usleep(SLEEP_TIME);
        }

		if(!dir)
        {
            sem_wait(&sem_active);
            socket_active[src] = true;
            socket_active[dest] = true;
            sem_post(&sem_active);
        }
        
        remove(file_name.c_str());
    }
}

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

    sem_init(&sem_active, 0, 1);
    sem_init(&sem_response, 0, 1);

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
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP;
    struct epoll_event events[MAX_EVENTS];
    epoll_fd = epoll_create1(0);
    if(epoll_fd < 0)
    {
        perror("Error Epoll_fd Creation");
        exit(1);
    }

    connection* router_conn = new connection();
    router_conn->trigger = router_socket;
    ev.data.ptr = router_conn;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, router_socket, &ev) < 0) {
        perror("epoll_ctl: server_fd");
        exit(1);
    }

    sockaddr_in client_addr;
    sockaddr_in server_addr;
    int client_socket;
    char addr_str[INET_ADDRSTRLEN];
    while(1)
    {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        while (nfds == -1) 
        {
            nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
	    }

        for (int n = 0; n < nfds; ++n) 
        {
            if (((connection*)events[n].data.ptr)->trigger == router_socket) 
            {
                client_socket = accept(router_socket, (struct sockaddr *)&client_addr, (socklen_t*)&addr_len);
                if(client_socket < 0) 
                {
                    continue;
                }

                set_socket_non_blocking(client_socket);

                connection* conn[2] = {new connection(), new connection()};
                
                memset(addr_str, INET_ADDRSTRLEN, 0);
                inet_ntop(AF_INET, &client_addr.sin_addr, addr_str, sizeof(addr_str));
                conn[0]->src_ip = string(addr_str);
                conn[0]->src_port = ntohs(client_addr.sin_port);
                conn[0]->src_socket = client_socket;
                conn[0]->trigger = client_socket;
                getsockopt(conn[0]->src_socket, SOL_IP, SO_ORIGINAL_DST, &server_addr, (socklen_t *)&addr_len);

                memset(addr_str, INET_ADDRSTRLEN, 0);
                inet_ntop(AF_INET, &server_addr.sin_addr, addr_str, sizeof(addr_str));
                conn[0]->dst_ip = string(addr_str);
                conn[0]->dst_port = ntohs(server_addr.sin_port);
                conn[0]->dst_socket = socket(AF_INET, SOCK_STREAM, 0);

		        conn[1]->src_ip = conn[0]->src_ip;
                conn[1]->src_port = conn[0]->src_port;
		        conn[1]->src_socket = conn[0]->src_socket;
		        conn[1]->dst_ip = conn[0]->dst_ip;
                conn[1]->dst_port = conn[0]->dst_port;
		        conn[1]->dst_socket = conn[0]->dst_socket;
		        conn[1]->trigger = conn[1]->dst_socket;

		        connect(conn[0]->dst_socket, (sockaddr*)&server_addr, sizeof(server_addr));
                set_socket_non_blocking(conn[0]->dst_socket);
                ev.data.ptr = conn[0];

                if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn[0]->src_socket, &ev) < 0) 
                {
                    perror("epoll_ctl: new_socket");
                    exit(EXIT_FAILURE);
                }   

                ev.data.ptr = conn[1];
                if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn[0]->dst_socket, &ev) < 0) 
                {
                    perror("epoll_ctl: new_socket");
                    exit(EXIT_FAILURE);
                }


                if(ntohs(server_addr.sin_port) != 21)
                {
                    sem_wait(&sem_active);
                    socket_active[conn[0]->src_socket] = false;
                    socket_active[conn[0]->dst_socket] = false;
                    sem_post(&sem_active);
                }
                else
                {
                    sem_wait(&sem_active);
                    socket_active[conn[0]->src_socket] = true;
                    socket_active[conn[0]->dst_socket] = true;
                    sem_post(&sem_active);
                }
            }
            else 
            {
                char buffer[1024] = "";
                int s_socket, d_socket;
                if(((connection*)events[n].data.ptr)->trigger == ((connection*)events[n].data.ptr)->src_socket)
                {
                    s_socket = ((connection*)events[n].data.ptr)->src_socket;
                    d_socket = ((connection*)events[n].data.ptr)->dst_socket;
                }
                else 
                {
                    s_socket = ((connection*)events[n].data.ptr)->dst_socket;
                    d_socket = ((connection*)events[n].data.ptr)->src_socket;
                }
                
                memset(buffer, 0, 1024);
                int x = recv(s_socket, buffer, 1024, 0);
                if(x > 0)
                {
                    sem_wait(&sem_response);
			        response_map[d_socket].assign(buffer, buffer + x);
                    sem_post(&sem_response);
                    sem_wait(&sem_active);
                    int activated = socket_active[s_socket];
                    sem_post(&sem_active);
                    if(activated)
                        send(d_socket, buffer, x, 0);			
                    else if(((connection*)events[n].data.ptr)->dst_port != 21)
                    {
                        string file_name = ((connection*)events[n].data.ptr)->dst_ip + ":" + to_string(((connection*)events[n].data.ptr)->dst_port) + ":" + ((connection*)events[n].data.ptr)->src_ip;
			            FILE* file = fopen(file_name.c_str(), "ab");
                        if(fwrite(buffer, 1, x, file) != x)
				            perror("fwrite error");
                        fclose(file);
			
                    }
                }
                else if(x == 0)
                {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, s_socket, NULL);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, d_socket, NULL);

                    if(((connection*)events[n].data.ptr)->dst_port != 21)
                    {
                        string file_name = ((connection*)events[n].data.ptr)->dst_ip + ":" + to_string(((connection*)events[n].data.ptr)->dst_port) + ":" + ((connection*)events[n].data.ptr)->src_ip;
                        auto conn_sockets = conn_map[file_name];
                        conn_map.erase(file_name);
                        sem_wait(&sem_active);
                        socket_active.erase(s_socket);
                        socket_active.erase(d_socket);
                        sem_post(&sem_active);

                        sem_wait(&sem_response);
                        string t_first = response_map[conn_sockets.first];
                        string t_second = response_map[conn_sockets.second];
                        sem_post(&sem_response);

                        helper_map.erase(conn_sockets.first);
			            if(t_second.find("150") == 0)
			            {
                            if(t_first.find("STOR") == 0)
                            {
                                thread t1(send_file, file_name, conn_sockets.first, conn_sockets.second, d_socket, s_socket, 0);
			                    t1.detach();
                            }
                            else
                            {
                                thread t1(send_file, file_name, conn_sockets.first, conn_sockets.second, d_socket, s_socket, 1);
			                    t1.detach();
                            }
                
                        }
                        else if(t_second.find("226") == 0)
                        {

                        }
                        else
                        {
                            strcpy(buffer, "426 Virus Detected");
                            send(conn_sockets.first, buffer, strlen(buffer), 0);
                        }
                    }
			        else
                    {
                        close(s_socket);
                        close(d_socket);
                    }
                }
                else 
                {
                    perror("No Data Available");
                }

                if(((connection*)events[n].data.ptr)->dst_port == 21 && !strncmp(buffer, "229 Entering Extended Passive Mode", 34))
                {
                    char temp[1024] = "";
                    strcpy(temp, buffer);
                    strtok(temp, "|||");
                    char* temp_str = strtok(NULL, "|");
                    conn_map[((connection*)events[n].data.ptr)->dst_ip + ":" + string(temp_str) + ":" + ((connection*)events[n].data.ptr)->src_ip] = {s_socket, d_socket};
                    helper_map[((connection*)events[n].data.ptr)->dst_socket] = string(temp_str);
		        }
                else if(((connection*)events[n].data.ptr)->dst_port == 21 && (!strncmp(buffer, "150", 3)))
                {
			        int a = conn_map[((connection*)events[n].data.ptr)->dst_ip + ":" + helper_map[((connection*)events[n].data.ptr)->dst_socket] + ":" + ((connection*)events[n].data.ptr)->src_ip].first;
			        int b = conn_map[((connection*)events[n].data.ptr)->dst_ip + ":" + helper_map[((connection*)events[n].data.ptr)->dst_socket] + ":" + ((connection*)events[n].data.ptr)->src_ip].second;
                    sem_wait(&sem_active);
                    socket_active[a] = false;
                    socket_active[b] = false;
                    sem_post(&sem_active);
                }
            }
        }
    }
}
