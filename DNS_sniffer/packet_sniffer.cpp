#include<iostream>
#include<pcap/pcap.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/ether.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<netinet/udp.h>
#include<string.h>
#include<ctime>

using namespace std;

void callback(__u_char* user, const struct pcap_pkthdr *h, const __u_char* packet)
{
    ether_header* eth;
    eth = (struct ether_header*)packet;
    short net_type = ntohs(eth->ether_type);
    uint8_t* mac_addr = eth->ether_shost;
    if(net_type == ETHERTYPE_IP)
    {
        struct ip* iph;
        packet += sizeof(ether_header);
        iph = (struct ip*)packet;
        int ip_length = iph->ip_hl*4;
        int ip_protocol = iph->ip_p;
        in_addr src_ip = iph->ip_src;

        char src_ip_buffer[INET_ADDRSTRLEN] = "";
        inet_ntop(AF_INET, &src_ip, src_ip_buffer, INET_ADDRSTRLEN);

        if(ip_protocol == IPPROTO_UDP)
        {
            udphdr* udph;
            packet += ip_length;
            udph = (udphdr*)packet;
            int udp_len = 8;
            if(ntohs(udph->dest) == 53)
            {
                const __u_char* dns_data = packet + udp_len;
                int num_records = 0;
                if((dns_data[3] & 0x80) == 0)
                {
                    num_records = dns_data[4] << 8 | dns_data[5];
                    
                    timeval time = h->ts;
                    char timestamp[90];
                    struct tm* time_info = localtime(&time.tv_sec);
                    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", time_info);
                    // strcat(timestamp, ".");
                    // strcat(timestamp, to_string(time.tv_usec).c_str());
                    char entry[2048] = "";

                    int loc = 12;
                    for(int i = 0;i < num_records; i++)
                    {
                        char buffer[1024] = "";
                        int val = dns_data[loc];
                        int domain_len = 0;
                        while(val != 0)
                        {
                            strncat(buffer, (char*)dns_data + loc + 1, val);
                            strcat(buffer, ".");
                            domain_len += val + 1;
                            loc += val + 1;
                            val = dns_data[loc];
                        }
                        loc += 4;

                        strcpy(entry, timestamp);
                        strcat(entry, ",");
                        strcat(entry, src_ip_buffer);
                        strcat(entry, ",");
                        strncat(entry, buffer, domain_len - 1);
                        // strcat(entry, "\n");
                        // printf("%s\n", entry);
                        int w = fprintf((FILE*)user, "%s\n", entry);
                        fflush((FILE*)user);
                        printf("packet logged %d\n", w);
                    }
                }

            }
        }

    } 
}

int main(int argc, char* args[])
{

    if(argc != 3)
    {
        perror("Invalid Number of CLI Args provided");
        exit(1);
    }
    char* interface = args[1];
    char* log_name = args[2];

    char err_buf[PCAP_ERRBUF_SIZE];
    pcap_t* handler;

    FILE* file = fopen(log_name, "a");
    if(file == nullptr)
    {
        perror("Failed to open file handler");
        exit(1);
    }
    handler = pcap_open_live(interface, BUFSIZ, 0, 1000, err_buf);

    if(!handler)
    {
        perror("Failed to create packet sniffer handler");
        exit(1);
    }

    if(!pcap_loop(handler, 0, callback, (__u_char*)file))
        return 0;
    
    perror("PCAP_LOOP Error");
    exit(1);
}