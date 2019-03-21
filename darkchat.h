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

// Metadata 
struct metadata_s{
    int ip_count;
    uint32_t my_ip;
    int master_sock;
    uint32_t* ip_list;
    unsigned int lock: 1;
};
typedef struct metadata_s* Metadata;

// IP list
struct ip_node_s{
    uint32_t* address_list;
};
typedef struct ip_node_s* IPs;

//------- voids
void print_usage(); // print the usage
void create_directories(); // create the .darkchat and keys if not present

//------- socket
int init_socket();
uint32_t get_ip_of_iterface(char* interface);

//------- threading
void* message_reciever_worker(void* arg);
void* message_sender_worker(void* arg);

//------- destruction
void destructor(Arguments args, Metadata meta);
