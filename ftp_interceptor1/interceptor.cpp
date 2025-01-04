#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>

using namespace std;

static int callback(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data) 
{
    uint32_t id = 0;
    struct nfqnl_msg_packet_hdr *ph = nfq_get_msg_packet_hdr(nfa);
    if (ph) 
    {
        id = ntohl(ph->packet_id);
    }

    unsigned char* packet_data;

    int len = nfq_get_payload(nfa, &packet_data);
    if (len >= 0) {
        iphdr* ip_header = (struct iphdr*)packet_data;
        if (ip_header->protocol == IPPROTO_TCP) 
        {
            tcphdr* tcp_header = (struct tcphdr*)(packet_data + (ip_header->ihl*4));

            char src_ip[INET_ADDRSTRLEN], dst_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(ip_header->saddr), src_ip, INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &(ip_header->daddr), dst_ip, INET_ADDRSTRLEN);
            int src_port, dst_port;
            src_port = ntohs(tcp_header->source);
            dst_port = ntohs(tcp_header->dest);
            cout << "*********PACKET RECEIVED*********\n";
            cout << "Source IP: " << src_ip << std::endl;
            cout << "Destination IP: " << dst_ip << std::endl;
            cout << "Source Port: " << src_port << std::endl;
            cout << "Destination Port: " << dst_port << std::endl;

            int payload_offset = (ip_header->ihl * 4) + (tcp_header->doff * 4);
            int payload_length = len - payload_offset;
            if (payload_length > 0) 
            {
                cout << "Payload: ";
                for (int i = 0; i < payload_length && i < 20; i++) 
                {
                    printf("%c", packet_data[payload_offset + i]);
                }
                cout << "\n";
            }
            cout << "*********************************\n";
            
        }
    }

    return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
}

int main(int argn, char* args[])
{   
    uint8_t client_buffer[1024];
    struct nfq_handle *h;
    struct nfq_q_handle *qh;
    int fd;
    int rv;
    char buf[4096] __attribute__ ((aligned));

    cout << "Opening library handle\n";
    h = nfq_open();
    if(!h) 
    {
        perror("nfq_open Error:");
        exit(1);
    }

    cout << "Unbinding existing nf_queue handler for AF_INET (if any)\n";
    if (nfq_unbind_pf(h, AF_INET) < 0) 
    {
        perror("nfq_unbind Error:");
        exit(1);
    }

    cout << "Binding nfnetlink_queue as nf_queue handler for AF_INET\n";
    if (nfq_bind_pf(h, AF_INET) < 0) 
    {
        perror("nfq binding Error:");
        exit(1);
    }

    cout << "Binding this socket to queue \'1\'\n";
    qh = nfq_create_queue(h, 1, &callback, NULL);
    if (!qh) 
    {
        perror("queue creation Error:");
        exit(1);
    }

    cout << "Setting copy_packet mode\n";
    if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) 
    {
        perror("copy mode Error:");
        exit(1);
    }

    fd = nfq_fd(h);

    cout << "Waiting for packets...\n";

    while ((rv = recv(fd, buf, sizeof(buf), 0)) >= 0) 
    {
        nfq_handle_packet(h, buf, rv);
    }

    cout << "Unbinding from queue 1\n";
    nfq_destroy_queue(qh);

    cout << "Closing library handle\n";
    nfq_close(h);
}