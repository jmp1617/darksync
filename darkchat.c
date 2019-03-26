#include "darkchat.h"

// Ip linked list
void IPL_add(uint32_t ip, IP_List* root){
    if(!(*root)){
        (*root) = calloc(1,sizeof(struct ip_list_s));
        (*root)->ip = ip;
        (*root)->next = NULL;
    }
    else{
        IP_List temp = *root;
        while(temp->next){
            temp = temp->next;
        }
        temp->next = calloc(1,sizeof(struct ip_list_s));
        temp->next->ip = ip;
        temp->next->next = NULL;
    }
}

void IPL_print(IP_List root){
    if(root){
        print_ip(root->ip);
        printf("\n");
        if(root->next)
            IPL_print(root->next);
    }
}

void IPL_destroy(IP_List root){
    if(root){
        if(root->next)
            IPL_destroy(root->next);
        free(root);
    }
}

// Aux
uint32_t conv_ip(char* ip){
    uint32_t result=0x00000000;
    char oct[16]={0};
    int o;
    int p = 0;
    //rip apart inputed data and validate
    for(int octet = 0; octet<4; octet++){
        o = octet*4;
        if(octet<3){ // first 3 octets
            while(ip[p]!='.'){
                if(p==o+4){
                    fprintf(stderr, "IP: %s is not a valid IP.\n",ip);
                    exit(EXIT_FAILURE);
                }
                oct[o]=ip[p];
                p++;
                o++;
            }
            p++;
            oct[o] ='\0';
        }
        else{ // last octet
            strncpy(oct+12,ip+p,4);
            oct[15] = '\0';
        }
    }
    // convert to uint32_t while checking fields
    for(int byte=3;byte>=0;byte--){ 
        char b[4]={0};
        strncpy(b,oct+(byte*4),4);
        int x = strtol(b,NULL,10);
        if(x>255){
            fprintf(stderr,"IP: %s is not a valid IP.\n",ip);
            exit(EXIT_FAILURE);
        }
        result |= (uint8_t)x;
        if(byte>0)
            result <<= 8;
    }
    return result;
}

// Voids
void print_usage(){
    fprintf(stderr,"Usage: darkchat [key] [node_ip] [nickname] [interface]\n \
            \tkey: AES key name\n \
            \tnode_ip: ip of active chat node (enter \"p\" to start in passive mode)\n \
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
    for(size_t byte=0; byte<strlen(argv[1]); byte++){
        uint8_t b = argv[1][byte];
        if( b > 122 || b < 33 ){
            fprintf(stderr,"Key file name should only contain ASCII 33 - 122.\n");
            exit(EXIT_FAILURE);
        }
    }
    // check ip 
    if(!(argv[2][0]=='p'&&strlen(argv[2])==1)){
        if(strlen(argv[2])>15){
            fprintf(stderr,"Invalid IP.\n");
            exit(EXIT_FAILURE);
        }
        for(size_t byte=0; byte<strlen(argv[2]); byte++){
            uint8_t b = argv[2][byte];
            if( (b < 48 || b > 57) && b != 46 ){
                fprintf(stderr,"Invalid IP.\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    // check nickname
    if(strlen(argv[3])>20){
        fprintf(stderr,"Try using a shorter nickname.\n");
        exit(EXIT_FAILURE);
    }
    for(size_t byte=0; byte<strlen(argv[3]); byte++){
        uint8_t b = argv[3][byte];
        if( b > 122 || b < 33 ){
            fprintf(stderr,"Nickname should only contain ASCII 33 - 122.\n");
            exit(EXIT_FAILURE);
        }
    }
}

void print_ip(uint32_t ip){
    uint8_t octet[4];
    for(int i = 0 ; i < 4 ; i++)
        octet[i] = ip >> (i * 8);
    printf("%d.%d.%d.%d",octet[0],octet[1],octet[2],octet[3]);
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
       break; 
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
        IPL_destroy(meta->ip_list);
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
        strncpy(args->nickname, argv[3], strlen(argv[3])+1);
        
        // Create Dirs
        create_directories();
        
        // Initialize Metadata
        Metadata meta = calloc(1,sizeof(struct metadata_s));
        if(argv[2][0]=='p'){
            meta->ipassive = 1;
            strncpy(args->node_ip, "passive", 8);
        }
        else{
            strncpy(args->node_ip, argv[2], strlen(argv[2])+1);
            meta->ipassive = 0;
        }
        if(!meta->ipassive)
            meta->ip_count = 1;
        else
            meta->ip_count = 0;
        meta->my_ip = get_ip_of_interface(argv[4]);
        if( !meta->my_ip ){
            fprintf(stderr,"%s is not a valid interface.\n",argv[4]);
            exit(EXIT_FAILURE);
        }
        IPL_add(meta->my_ip,&(meta->ip_list)); //initial list only contains yourself
        printf("Welcome, %s\nService binding to ",args->nickname);
        print_ip(meta->my_ip);
        printf(":%d\n",LPORT);
        // Print the initial data
        printf("\nActive IP list:\n");
        IPL_print(meta->ip_list);
        printf("\n");
        
        meta->master_sock = init_socket();
        printf("Socket Initialized\n"); 
        
        // ask for the itial nodes ip list 
        if(meta->ipassive){ // if you are the first node on the network
            printf("Passive: waiting for clients...\n");
            if (listen(meta->master_sock, MAXCONN) < 0){ 
                fprintf(stderr, "failed on listen\n"); 
                exit(EXIT_FAILURE); 
            } 
        }
        else{
            printf("Asking %s for active ip list...\n",args->node_ip);
        }

        // Initialize Threads
        //pthread_t thread_id_reciever, thread_id_sender;
        //void* thread_ret;
        //pthread_create(&thread_id_reciever, NULL, message_reciever_worker, meta);
	    //pthread_create(&thread_id_sender, NULL, message_sender_worker, meta);
        //pthread_join(thread_id_reciever, &thread_ret);
        //pthread_join(thread_id_sender, &thread_ret);
        
        // Free the malloc
        destructor(args,meta);
    }
    return 0;
}
