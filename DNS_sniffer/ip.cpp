#include<iostream>
#include<iomanip>
#include<unordered_map>
#include<netinet/in.h>
#include<arpa/inet.h>

using namespace std;

class Node
{
public:
    Node* arr[2] = {nullptr, nullptr};
    bool end = false;
    uint32_t entry;
    int count = 0;
};

class Routing_table
{
public:
    Node* root;

    Routing_table()
    {
        root = new Node();
        root->count = 1;
        root->end = true;
    }

    void insert(uint32_t ip, int mask, uint32_t entry)
    {
        Node* temp = root;
        root->count++;
        for(int i = 0;i < mask;i++)
        {
            int bit = ((ip & 0x80000000) >> 31);

            if(temp->arr[bit] == nullptr)
            {
                temp->arr[bit] = new Node();
            }

            temp = temp->arr[bit];
            (temp->count)++;
            ip = ip << 1;
        }

        temp->end = true;
        temp->entry = entry;
    }

    uint32_t find(uint32_t ip)
    {
        Node* temp = root;
        uint32_t last_matching_ip = 0;
        for(int i = 0;i < 32;i++)
        {
            int bit = ((ip & 0x80000000) >> 31);
            if(temp->arr[bit] == nullptr)
                break;
            temp = temp->arr[bit];

            if(temp->end)
                last_matching_ip = temp->entry;
            ip = ip << 1;
        }

        return last_matching_ip;
    }

    int delete_entry(uint32_t entry, int size, Node* root, int count = 0)
    {
        // cout << "count" << count << "\n";
        if(count > size)
            return -2;
        
        if(root == nullptr)
            return -1;

        int bit = (entry & 0x80000000) >> 31;
        int ans = delete_entry(entry << 1, size, root->arr[bit], count + 1);
        // cout << "ans" << count << ":" << ans << "\n";
        if(ans == -1)
            return -1;
        if(ans == 0)
        {
            free(root->arr[bit]);
            root->arr[bit] = nullptr;
        }
        else if(ans == -2)
            root->end = false;

        (root->count)--;
        return root->count;

    }
};

class routing_entry
{
    int mask;
    uint32_t ip;
    string ip_str;
    uint32_t next_hop;
    string next_hop_str;
    string interface_name;
};

int main(int argc, char* args[])
{
    if(argc < 3)
    {
        cout << "Invalid Number of Inputs\n";
        exit(1);
    }
    unordered_map<uint32_t, routing_entry*> routing_table;

    char* ip1 = args[1];
    int mask1 = atoi(args[2]);
    int ip_addr1 = 0;
    inet_pton(AF_INET, ip1, &ip_addr1);
    ip_addr1 = htonl(ip_addr1);
    if(mask1 > 32 || mask1 < 0)
    {
        cout << "Invalid Mask";
        exit(1);
    }

    char* ip2 = args[3];
    int mask2 = atoi(args[4]);
    int ip_addr2 = 0;
    inet_pton(AF_INET, ip2, &ip_addr2);
    ip_addr2 = htonl(ip_addr2);
    if(mask2 > 32 || mask2 < 0)
    {
        cout << "Invalid Mask";
        exit(1);
    }

    char* ip3 = args[5];
    int ip_addr3 = 0;
    inet_pton(AF_INET, ip3, &ip_addr3);
    ip_addr3 = htonl(ip_addr3);
    

    Routing_table routing_helper = Routing_table();
    int entry1 = ip_addr1;
    entry1 = entry1 >> (32 - mask1);
    entry1 = entry1 << (32 - mask1);
    routing_helper.insert(ip_addr1, mask1, entry1);
    int entry2 = ip_addr2;
    entry2 = entry2 >> (32 - mask2);
    entry2 = entry2 << (32 - mask2);
    routing_helper.insert(ip_addr2, mask2, entry2);
    int ans = ntohl(routing_helper.find(ip_addr3));
    char sol[32] = "";
    inet_ntop(AF_INET, &ans, sol, 32);
    // cout << (routing_helper.root->arr[1]->arr[1]->arr[1] != nullptr)
    cout << string(sol) << "\n";
    Node* temp = routing_helper.root;
    routing_helper.delete_entry(entry1, mask1, temp);
    int ans1 = ntohl(routing_helper.find(ip_addr3));
    char sol1[32] = "";
    inet_ntop(AF_INET, &ans1, sol1, 32);
    // cout << (routing_helper.root->arr[1]->arr[1]->arr[1] != nullptr)
    cout << string(sol1) << "\n";
    cout << routing_helper.root->end << "\n";
}