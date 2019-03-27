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

int send_message(Message m, int socket){
    uint8_t* buffer = calloc(m->size, 1);
    buffer[0] = m->identifier;
    if(m->size > 1)
        for(int byte = 1; byte <= m->size; byte++){
            buffer[byte] = (m->message)[byte-1];
        }
    send(socket , buffer , m->size, 0 ); 
    return 0;
}

// Threading
void* message_reciever_worker(void* arg){
    Metadata meta = (Metadata)arg;
    printf("Reciever: %d current active IP addresses\n",meta->ip_count);
    printf("Reciever: waiting for messages...\n");
    if (listen(meta->master_sock, MAXCONN) < 0){ 
        fprintf(stderr, "failed on listen\n"); 
        exit(EXIT_FAILURE); 
    }
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int new_socket;
    if ((new_socket = accept(meta->master_sock, (struct sockaddr *)&address,(socklen_t*)&addrlen))<0){
        fprintf(stderr, "failed on accept");
        exit(EXIT_FAILURE);
    }
    printf("Reciever: ");
    print_ip(address.sin_addr.s_addr);
    printf(" connected.\n");
    uint8_t message[1024] = {0};
    read(new_socket , message, 1024); 
    if(message[0]==ACTIVE_NODES_REQ){ // node list request
        meta->lock = 1;
        IPL_add(address.sin_addr.s_addr,&(meta->ip_list));
        meta->ip_count++;
        Message ip_list_message = calloc(1,sizeof(struct message_s));
        ip_list_message->identifier = NODE_RES;
        ip_list_message->size = (meta->ip_count*4)+1;
        ip_list_message->message = calloc((meta->ip_count*4)+2,1);
        ip_list_message->message[0] = meta->ip_count;
        int offset = 1;
        IP_List temp = meta->ip_list;
        for(int ip = 0; ip < meta->ip_count; ip++){
            memcpy(ip_list_message+offset,&(temp->ip),4);
            offset+=4;
            temp = temp->next;
        }
        meta->lock = 0;
        send_message(ip_list_message, new_socket);
        close(new_socket);
    }
    else if(message[0]==DISCONNECT){ // disconnect 

    }
    else if(message[0]==STD_MSG){ // normal message 

    }
    else{ // otherwise drop
        close(new_socket);    
    }
    return 0;
}

void* message_sender_worker(void* arg){
    Metadata meta = (Metadata)arg;
    printf("Sender: %d current active IP addresses\n",meta->ip_count);
    return 0;
}

// Destruction 
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
        if(!meta->ipassive){
            printf("Asking %s for active ip list...\n",args->node_ip);
            // create the message;
            Message request = calloc(1,sizeof(struct message_s));
            request->identifier = ACTIVE_NODES_REQ;
            request->size = 1;
            // connect to node
            struct sockaddr_in node;
            node.sin_family = AF_INET;
            node.sin_port = htons(LPORT);
            if(inet_pton(AF_INET, args->node_ip, &node.sin_addr)<=0){
                printf("\nInvalid address/ Address not supported \n");
                exit(EXIT_FAILURE);
            }
            while(connect(meta->master_sock, (struct sockaddr *)&node, sizeof(node)) < 0);
            send_message(request, meta->master_sock); 
            free(request);
            uint8_t* buffer[MAXCONN*4]={0};
            read(meta->master_sock, buffer, MAXCONN*4);
            printf("List recieved from %s.\n",args->node_ip);
            //add to ip list 
        }

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
