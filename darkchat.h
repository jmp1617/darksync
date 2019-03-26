#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <pthread.h>

#define LPORT 8686
#define MAXCONN 50

//------- Structures
// User Input
struct arguments_s{
    char* key;
    char* node_ip;
    char* nickname;
};
typedef struct arguments_s* Arguments;

// Ip linked list
struct ip_list_s{
    uint32_t ip;
    struct ip_list_s* next;
};
typedef struct ip_list_s* IP_List;

void IPL_add(uint32_t ip, IP_List* root);
void IPL_print(IP_List root);
void IPL_destroy(IP_List root);

// Metadata 
struct metadata_s{
    int ip_count;
    uint32_t my_ip;
    int master_sock;
    IP_List ip_list;
    unsigned int lock: 1;
    unsigned int ipassive: 1;
};
typedef struct metadata_s* Metadata;

//------- voids
void print_usage(); // print the usage
void create_directories(); // create the .darkchat and keys if not present
void check_args(); // validate arguments
void print_ip(uint32_t ip); // print in human readable

//------- aux
uint32_t conv_ip(char* ip); // check and convert the ip

//------- socket
int init_socket();
uint32_t get_ip_of_iterface(char* interface);

//------- threading
void* message_reciever_worker(void* arg);
void* message_sender_worker(void* arg);

//------- destruction
void destructor(Arguments args, Metadata meta);
