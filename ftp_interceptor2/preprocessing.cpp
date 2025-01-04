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
#include <libnetfilter_queue/pktbuff.h>
#include <linux/netfilter/nfnetlink_queue.h>
#include <netdb.h>

#define INTERCEPTOR_IP "0.0.0.0"
#define INTERCEPTOR_PORT 8000

using namespace std;

void add_snat_rule(uint32_t orig_src_ip, uint32_t orig_dst_ip, uint16_t orig_src_port, uint16_t orig_dst_port, char* interceptor_ip, int interceptor_port)
{
    char rule[200];
    memset(rule, 0, 200);
    snprintf(rule, sizeof(rule), "sudo iptables -t nat -A POSTROUTING -s %s -d %s -p tcp --sport %d --dport %d -j SNAT --to-source %s:%d",
    interceptor_ip,
    inet_ntoa((struct in_addr){.s_addr = orig_src_ip}),
    interceptor_port, ntohs(orig_src_port),
    inet_ntoa((struct in_addr){.s_addr = orig_dst_ip}),
    ntohs(orig_dst_port)
    );

    system(rule);
}

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
            int src_port, dst_port;

            if(tcp_header->syn && !tcp_header->ack)
            {
                add_snat_rule(ip_header->saddr, ip_header->daddr, tcp_header->source, tcp_header->dest, INTERCEPTOR_IP, INTERCEPTOR_PORT);
            }
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