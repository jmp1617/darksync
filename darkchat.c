#include "darkchat.h"

// Voids
void print_usage(){
    fprintf(stderr,"Usage: darkchat [key] [node_ip] [nickname] [interface]\n \
            \tkey: AES key name\n \
            \tnode_ip: ip of active chat node\n \
            \tnickname: chat nickname\n \
            \tinterface: desired interface, IE: wlp4s0\n\n \
            \tNote: place key file in $HOME/.darknet/keys dir\n");
}

void create_directories(){
        struct stat st = {0};
        char path[1024] = {0};
        strcat(path,"/home/");
        strcat(path,getlogin());
        strcat(path,"/.darkchat");
        if (stat(path, &st) == -1)
            mkdir(path, 0700);
        strcat(path,"/keys");
        if (stat(path,&st) == -1)
            mkdir(path, 0700);
}

void check_args(char* argv[]){
    // check key file
    if(strlen(argv[1])>50){
        fprintf(stderr,"Key filename to long, rename it.\n");
        exit(EXIT_FAILURE);
    }
    // check nickname
    if(strlen(argv[3])>20){
        fprintf(stderr,"Try using a shorter nickname.\n");
        exit(EXIT_FAILURE);
    }
}

// Socket
int init_socket(){
    struct sockaddr_in address;
    int sockfd;
    int opt = 1;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0){ 
        fprintf(stderr,"socket failed"); 
        exit(EXIT_FAILURE); 
    }
    // force port 8686
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){ 
        fprintf(stderr,"setsockopt"); 
        exit(EXIT_FAILURE); 
    }
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( LPORT );
    if (bind(sockfd, (struct sockaddr *)&address, sizeof(address))<0){
        fprintf(stderr,"bind failed");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

uint32_t get_ip_of_interface(char* interface){
    int fd;
 	struct ifreq ifr;
 	fd = socket(AF_INET, SOCK_DGRAM, 0);
 	ifr.ifr_addr.sa_family = AF_INET;
 	strncpy(ifr.ifr_name, interface, IFNAMSIZ-1); 
 	ioctl(fd, SIOCGIFADDR, &ifr);
 	close(fd);
    return (((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr).s_addr; 
}

void* message_reciever_worker(void* arg){
    Metadata meta = (Metadata)arg;
    printf("Reciever: %d current active IP addresses\n",meta->ip_count);
    while(1){ // wait for connections
        return 0;
    }
    return 0;
}

void* message_sender_worker(void* arg){
    Metadata meta = (Metadata)arg;
    printf("Sender: %d current active IP addresses\n",meta->ip_count);
    return 0;
}

void destructor(Arguments args, Metadata meta){
    if(args){
        free(args->key);
        free(args->node_ip);
        free(args->nickname);
        free(args);
    }
    if(meta){
        close(meta->master_sock);
        free(meta);
    }
}

int main(int argc, char* argv[]){
    if( argc != 5 )
        print_usage();
    else{
        // Check arguments
        check_args(argv);
        // Load arguments
        Arguments args = calloc(1, sizeof(struct arguments_s));
        args->key = calloc(1,strlen(argv[1])+1);
        args->node_ip = calloc(1,strlen(argv[2])+1);
        args->nickname = calloc(1,strlen(argv[3])+1);
        strncpy(args->key, argv[1], strlen(argv[1])+1);
        strncpy(args->node_ip, argv[2], strlen(argv[2])+1);
        strncpy(args->nickname, argv[3], strlen(argv[3])+1);
        // Create Dirs
        create_directories();
        // Initialize Metadata
        Metadata meta = calloc(1,sizeof(struct metadata_s));
        meta->ip_count = 1;
        meta->my_ip = get_ip_of_interface(argv[4]);
        if( !meta->my_ip ){
            fprintf(stderr,"%s is not a valid inerface.\n",argv[4]);
            exit(EXIT_FAILURE);
        }
        uint32_t ip_addr = meta->my_ip;
        uint8_t octet[4];
        for(int i = 0 ; i < 4 ; i++)
            octet[i] = ip_addr >> (i * 8);
        printf("Welcome, %s\nService binding to %d.%d.%d.%d:%d\n",args->nickname,octet[0],octet[1],octet[2],octet[3],LPORT);
        // Create Master Socket - used to accept conncetions and recieve messages
        meta->master_sock = init_socket();
        printf("Socket Initialized\n");
        
        // Initialize Threads
        pthread_t thread_id_reciever, thread_id_sender;
        void* thread_ret;
        pthread_create(&thread_id_reciever, NULL, message_reciever_worker, meta);
	    pthread_create(&thread_id_sender, NULL, message_sender_worker, meta);
        pthread_join(thread_id_reciever, &thread_ret);
        pthread_join(thread_id_sender, &thread_ret);
        // Free the malloc
        destructor(args,meta);
    }
    return 0;
}
