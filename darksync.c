#include "darksync.h"

// Ip linked list
void IPL_add(uint32_t ip, IP_List* root, char* nickname){
    if(!(*root)){
        (*root) = calloc(1,sizeof(struct ip_list_s));
        (*root)->ip = ip;
        (*root)->next = NULL;
        memcpy((*root)->nick,nickname,20);
    }
    else{
        IP_List temp = *root;
        while(temp->next){
            temp = temp->next;
        }
        temp->next = calloc(1,sizeof(struct ip_list_s));
        temp->next->ip = ip;
        memcpy(temp->next->nick,nickname,20);
        temp->next->next = NULL;
    }
}

void IPL_print(IP_List root){
    if(root){
        print_ip(root->ip);
        printf(" (%s)",root->nick);
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

char* IPL_contains(uint32_t ip, IP_List root){
    char* ret = NULL;
    if(root){
        if(root->ip == ip){
            ret = malloc(20);
            memcpy(ret, root->nick, 20);
            return ret;
        }
        if(root->next)
            ret = IPL_contains(ip,root->next);
    }
    return ret;
}

int IPL_remove(uint32_t ip, IP_List* root){
    IP_List temp = *root;
    if(temp->ip == ip){ // its the head
        temp = (*root)->next;
        free(*root);
        (*root) = temp;
        return 1;
    }
    else{ // find it
        IP_List prev = (*root);
        IP_List current = (*root)->next;
        while(current && current->ip != ip){
            prev = current;
            current = prev->next;
        }
        // remove current
        if(current){
            IP_List temp = current->next;
            free(current);
            prev->next = temp;
            return 1;
        }
        else
            return 0;
    }
}

// Message List

void MSG_add(char* message, char* nick, uint32_t time, MSG_List* messages){
    if(!(*messages)){
        (*messages) = calloc(1,sizeof(struct message_list_s));
        memcpy((*messages)->message, message, MAXMSGLEN);
        memcpy((*messages)->nick, nick, 20);
        (*messages)->time = time;
        (*messages)->next = NULL;
    }
    else{
        MSG_List temp = *messages;
        while(temp->next)
            temp = temp->next;
        temp->next = calloc(1,sizeof(struct message_list_s));
        memcpy(temp->next->message, message, MAXMSGLEN);
        memcpy(temp->next->nick, nick, 20);
        temp->next->time = time;
        temp->next->next = NULL;
    }
}

void MSG_destroy(MSG_List messages){
    if(messages){
        if(messages->next)
            MSG_destroy(messages->next);
        free(messages);
    }
}

void MSG_display(MSG_List messages){
    if(messages){
        printf("[%s @ ",messages->nick);
        uint32_t* temp_time = calloc(4,1);
        memcpy(temp_time,&(messages->time),4);
        print_time(temp_time);
        free(temp_time);
        printf("]: %s",messages->message);
        if(messages->next)
            MSG_display(messages->next);
    }
}

void print_time(uint32_t* time){
    struct tm* t;
    t = localtime((time_t*)time);
    printf("%02d:%02d", t->tm_hour,t->tm_min);
}

void wprint_time(WINDOW* w, uint32_t* time){
    struct tm* t;
    t = localtime((time_t*)time);
    wprintw(w,"%02d:%02d", t->tm_hour,t->tm_min);
}

// Aux
uint32_t conv_ip(char* ip){
    uint32_t result=0;
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

// encryption
void generate_key_256(){
    uint8_t key[32];
    getrandom(key,32,0);
    char path[50] = {0};
    snprintf(path, sizeof path, "%s/.darksync/keys/key_%ld", getenv("HOME"),time(NULL));
    FILE* key_file = fopen(path,"w");
    fwrite(key,1,32,key_file);
}

void load_key(char* key, Metadata meta){
    if(key[0]!='0'&&key[1]!='\0'){
        char path[100] = "";
        strcat(path,getenv("HOME"));
        strcat(path,"/.darksync/keys/");
        strcat(path,key);
        FILE* keyfile = fopen(path,"r");
        if(!keyfile){
            fprintf(stderr,"Key does not exist. Exiting\n");
            meta->keyloaded = 0;
            exit(EXIT_FAILURE);
        }
        fread(meta->key,1,32,keyfile);
        meta->keyloaded = 1;
    }
}

int send_message_encrypted(Message m, int socket, struct AES_ctx* context){
    int buffer_size = m->size;
    if(buffer_size<16)
        buffer_size = 16;
    else if(buffer_size%16)//apply padding
        buffer_size = (buffer_size/16)*16+(buffer_size%16)*16;
    uint8_t* buffer = calloc(buffer_size, 1);
    buffer[0] = m->identifier;
    if(m->size > 1)
        for(int byte = 1; byte < m->size; byte++){
            buffer[byte] = (m->message)[byte-1];
        }
    //AES_CBC_encrypt_buffer(context,buffer,buffer_size);
    send(socket , buffer , buffer_size, 0 );
    free(buffer);
    return 0;
}

// Display
void display(Metadata meta){
    WINDOW* w = meta->win;
    WINDOW* mb = meta->message_board;
    WINDOW* s = meta->messenger;
    WINDOW* b = meta->banner;
    WINDOW* st = meta->status;
    WINDOW* ms = meta->message_sender;
    // banner
    wprintw(b,"\n");
    waddstr(b,"     ___           __\n");
    waddstr(b,"    / _ \\___ _____/ /__ _____ _____  ____\n");
    waddstr(b,"   / // / _ `/ __/  '_/(_-< // / _ \\/ __/\n");
    waddstr(b,"  /____/\\_,_/_/ /_/\\_\\/___|_, /_//_/\\__/\n");
    waddstr(b,"                         /___/\n");
    // status
    wmove(st,1,0);
    wprintw(st," Connections: %d\n",meta->ip_count-1);
    wprintw(st," Name: %s\n",meta->nick);
    wprintw(st," IP: ");
    uint8_t octet[4];
    for(int i = 0 ; i < 4 ; i++)
        octet[i] = meta->my_ip >> (i * 8);
    wprintw(st,"%d.%d.%d.%d\n",octet[0],octet[1],octet[2],octet[3]);
    wprintw(st," Port(R/S): %d/%d\n",RPORT,SPORT);
    char* enc_state = "No key loaded";
    if(meta->keyloaded)
        enc_state = "AES-256 Active";
    wprintw(st," Encryption: %s\n",enc_state);
    // borders
    wborder(w,0,0,0,0,0,0,0,0);
    wborder(st,0,0,0,0,ACS_TTEE,0,ACS_BTEE,ACS_RTEE);
    wborder(b,0,0,0,0,0,0,ACS_LTEE,0);
    wborder(s,0,0,0,0,ACS_LTEE,ACS_RTEE,0,0);
    // message board
    wmove(mb,0,0);
    display_mb(meta->messages,mb);
    // messenger
    wmove(s,1,2);
    waddch(s,'>');
    // get the input
    wmove(ms,0,0);
    // refresh
    wrefresh(w);
    wrefresh(mb);
    wrefresh(b);
    wrefresh(st);
    wrefresh(s);
}

void display_mb(MSG_List messages, WINDOW* mb){
    if(messages){
        wprintw(mb,"[%s @ ",messages->nick);
        uint32_t* temp_time = calloc(4,1);
        memcpy(temp_time,&(messages->time),4);
        wprint_time(mb,temp_time);
        free(temp_time);
        wprintw(mb,"]: %s\n",messages->message);
        if(messages->next)
            display_mb(messages->next,mb);
    }
}

// Blacklist
void load_blacklist(IP_List* root, Metadata meta){
    char path[1024] = {0};
    strcat(path, getenv("HOME"));
    strcat(path,"/.darksync/blacklist.txt");
    FILE* blacklist = fopen(path, "r");
    if(blacklist){
        size_t n = 0;
        char* ip = NULL;
        int nread = 0;
        nread = getline(&ip,&n,blacklist);
        while(nread != -1){
            IPL_add(conv_ip(ip),root,"bad");
            meta->blacklist_count++;
            free(ip);
            ip = NULL;
            nread = getline(&ip,&n,blacklist);
        }
        fclose(blacklist);
        free(ip);
    }
}

void dump_blacklist(IP_List root){
    char path[1024] = {0};
    strcat(path, getenv("HOME"));
    strcat(path,"/.darksync/blacklist.txt");
    FILE* blacklist = fopen(path, "w+");
    IP_List temp = root;
    while(temp){
        uint8_t octet[4]={0};
        for(int i = 0 ; i < 4 ; i++)
            octet[i] = temp->ip >> (i * 8);
        fprintf(blacklist,"%d.%d.%d.%d\n",octet[0],octet[1],octet[2],octet[3]);
        temp = temp->next;
    }
    fclose(blacklist);
}

// Voids
void print_usage(){
    fprintf(stderr,"Usage: darksync [key] [node_ip] [nickname] [interface]\n \
            \tkey: AES key name, 0 to start with no key loaded (careful, if you send a message in this mode you may be blacklisted).\n \
            \tnode_ip: ip of active chat node (enter \"p\" to start in passive mode)\n \
            \tnickname: chat nickname\n \
            \tinterface: desired interface, IE: wlp4s0\n\n \
            \tNote: place key file in $HOME/.darksync/keys dir\n");
}

void create_directories(){
    struct stat st = {0};
    char path[1024] = {0};
    strcat(path, getenv("HOME"));
    strcat(path,"/.darksync");
    if (stat(path, &st) == -1)
        mkdir(path, 0700);
    strcat(path,"/keys");
    if (stat(path,&st) == -1)
        mkdir(path, 0700);
    char fp[1024] = {0};
    strcat(fp, getenv("HOME"));
    strcat(fp,"/.darksync");
	strcat(fp,"/files");
	if (stat(fp,&st) == -1)
        mkdir(fp, 0700);
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
    if(strlen(argv[3])>19){
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
int init_socket(int port){
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
    address.sin_port = htons( port );
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
    uint8_t* buffer = calloc(1024, 1);
    buffer[0] = m->identifier;
    if(m->size > 1)
        for(int byte = 1; byte < m->size; byte++){
            buffer[byte] = (m->message)[byte-1];
        }
    send(socket , buffer , 1024, 0 );
    free(buffer);
    return 0;
}

// Threading
void* message_reciever_worker(void* arg){
    Metadata meta = (Metadata)arg;
    while(meta->lock!=2){
        if (listen(meta->reciever_s, MAXCONN) < 0){
            fprintf(stderr, "failed on listen\n");
            exit(EXIT_FAILURE);
        }
        struct sockaddr_in address;
        int addrlen = sizeof(address);
        int new_socket;
        if ((new_socket = accept(meta->reciever_s, (struct sockaddr *)&address,(socklen_t*)&addrlen))<0){
            if(meta->lock!=2){
                printf("failed on accept");
                exit(EXIT_FAILURE);
            }
            else{
                break;
            }
        }
        char* nick = IPL_contains(address.sin_addr.s_addr,meta->blacklist);
        if(nick){ // blacklist check
            close(new_socket); // ciao ciao
            free(nick);
        }
        else{
            int buf_len = (MAXFILESIZE/16)*16+(MAXFILESIZE%16)*16;
            uint8_t* message = calloc(buf_len,1);
            read(new_socket , message, buf_len);
            //AES_CBC_decrypt_buffer(meta->encrypt_context,message,buf_len);
            if(message[0]==ACTIVE_NODES_REQ){ // node list request
                lock(meta);
                //extract nickname
                char temp_nick[20] = {0};
                for(int c = 1; c <= 20; c++){
                    temp_nick[c-1] = message[c];
                }
                //add message
                char* conn = calloc(100,1);
                strcat(conn,inet_ntoa(address.sin_addr));
                strcat(conn," (");
                strcat(conn,temp_nick);
                strcat(conn,") connected.\0");
                MSG_add(conn, "~", time(NULL), &(meta->messages));
                free(conn);
                meta->ip_count = 2;
                display(meta);
                meta->ip_count = 1;
                Message ip_list_message = calloc(1,sizeof(struct message_s));
                ip_list_message->identifier = NODE_RES;
                ip_list_message->size = (meta->ip_count*4)+2+20+1+(meta->blacklist_count*4);
                ip_list_message->message = calloc((meta->ip_count*4)+2+20+1+(meta->blacklist_count*4),1);
                ip_list_message->message[0] = meta->ip_count;
                for(int c = 1; c <= 20; c++){
                    ip_list_message->message[c] = meta->nick[c-1];
                }
                int offset = 1+20;
                IP_List temp = meta->ip_list;
                for(int ip = 0; ip < meta->ip_count; ip++){// for each ip
                    uint32_t ipad = temp->ip; // copy the ip
                    for(int b = 3; b >= 0; b--){ // bytes in decending order
                        ip_list_message->message[offset+b] |= ipad&0xFF; // set the byte
                        ipad >>= 8; // shift to next
                    }
                    offset+=4; // jump forward 4 bytes in the message
                    temp = temp->next; // get the next ip
                }
                // add current blacklist
                ip_list_message->message[offset] = meta->blacklist_count;
                offset++;
                temp = meta->blacklist;
                for(int ip = 0; ip < meta->blacklist_count; ip++){// for each ip
                    uint32_t ipad = temp->ip; // copy the ip
                    for(int b = 3; b >= 0; b--){ // bytes in decending order
                        ip_list_message->message[offset+b] |= ipad&0xFF; // set the byte
                        ipad >>= 8; // shift to next
                    }
                    offset+=4; // jump forward 4 bytes in the message
                    temp = temp->next; // get the next ip
                }
                // Add the new ip
                IPL_add(address.sin_addr.s_addr,&(meta->ip_list),temp_nick);
                meta->ip_count++;
                //
                unlock(meta);
                send_message_encrypted(ip_list_message, new_socket, meta->encrypt_context);
                free(ip_list_message->message);
                free(ip_list_message);
                close(new_socket);
            }
            else if(message[0]==DISCONNECT){ // disconnect
                char* conn = calloc(100,1);
                char* temp_nick = IPL_contains(address.sin_addr.s_addr,meta->ip_list);
                strcat(conn,inet_ntoa(address.sin_addr));
                strcat(conn," (");
                strcat(conn,temp_nick);
                strcat(conn,") disconnected.\0");
                MSG_add(conn, "~", time(NULL), &(meta->messages));
                free(conn);
                free(temp_nick);
                IPL_remove(address.sin_addr.s_addr,&(meta->ip_list));
                meta->ip_count--;
                display(meta);
                close(new_socket);
            }
            else if(message[0]==STD_MSG){ // normal message
                uint32_t t = 0;
                for(int b = 3; b>=0; b--){
                    t |= message[1+MAXMSGLEN+b];
                    if(b!=0)
                        t<<=8;
                }
                char* nick = IPL_contains(address.sin_addr.s_addr,meta->ip_list);
                MSG_add((char*)(message+1),nick,t,&(meta->messages));
                free(nick);
                display(meta);
                close(new_socket);
            }
            else if(message[0]==HELLO){ // new peer
                char temp_nick[20] = {0};
                for(int c = 1; c <= 20; c++){
                    temp_nick[c-1] = message[c];
                }
                lock(meta);
                char* nick = IPL_contains(address.sin_addr.s_addr,meta->ip_list);
                if(!nick){
                    IPL_add(address.sin_addr.s_addr,&(meta->ip_list),temp_nick);
                    meta->ip_count++;
                }
                else
                    free(nick);
                unlock(meta);
                close(new_socket);
            }
            else if(message[0]==BL_UPD){ // new ip to blacklist
                uint32_t address = 0;
                for(int b = 3; b>=0; b--){ // each byte in address
                    address |= message[1+b]; // get the byte
                    if(b!=0) // shift if not the end byte
                        address <<= 8;
                }
                lock(meta);
                char* nick = IPL_contains(address,meta->blacklist);
                if(!nick){
                    IPL_add(address,&(meta->blacklist),"bad"); // add the ip to master list
                    meta->blacklist_count++;
                }
                else
                    free(nick);
                unlock(meta);
                close(new_socket);
            }
            else if(message[0]==F_MSG){
                lock(meta);
                char* msg = calloc(100,1);
                char* nick = IPL_contains(address.sin_addr.s_addr,meta->ip_list);
                strcat(msg,"new file recieved: ");
                strcat(msg,(char*)(message+1));
                strcat(msg,". placed in files directory.\0");
                char path[1024] = {0};
                strcat(path, getenv("HOME"));
                strcat(path,"/.darksync/");
                strcat(path,(char*)(message+1));
                FILE* fp = fopen(path,"w");
                fwrite(message+261, 1, (int)*(message+257), fp);
                MSG_add(msg, nick, time(NULL), &(meta->messages));
                free(msg);
                display(meta);
                unlock(meta);
            }
            else{ // otherwise drop
                lock(meta);
                char* bad = calloc(100,1);
                strcat(bad, "WARNING: bad message from ");
                strcat(bad, inet_ntoa(address.sin_addr));
                strcat(bad, ". Dropping and blacklisting.\0");
                MSG_add(bad, "~", time(NULL), &(meta->messages));
                free(bad);
                display(meta);
                IPL_add(address.sin_addr.s_addr,&(meta->blacklist),"bad");
                meta->blacklist_count++;
                meta->emit_black = 1;
                unlock(meta);
                close(new_socket);
            }
            free(message);
        }
    }
    return 0;
}

void* message_sender_worker(void* arg){
    Metadata meta = (Metadata)arg;
    while(meta->lock!=2){
        lock(meta);
        if(meta->emit_black&&meta->ip_count>1){ // new blacklist item, send it to everyone
            //grab the most recent addition to the list
            IP_List temp = meta->ip_list;
            while(temp->next)
                temp = temp->next;
            uint32_t new_black = temp->ip;
            Message bl_message = calloc(1,sizeof(struct message_s));
            bl_message->identifier = BL_UPD;
            bl_message->size = 21; // ident and ip
            bl_message->message = calloc(20,1);
            for(int b = 3; b >= 0; b--){ // bytes in decending order
                bl_message->message[b] |= new_black&0xFF; // set the byte
                new_black >>= 8; // shift to next
            }
            IP_List temp_ip = meta->ip_list->next;
            for(int ip=1; ip < meta->ip_count; ip++){
                // connect to node
                struct sockaddr_in node;
                node.sin_family = AF_INET;
                node.sin_port = htons(RPORT);
                node.sin_addr.s_addr = temp_ip->ip;
                while(connect(meta->sender_s, (struct sockaddr *)&node, sizeof(node)) < 0);
                send_message_encrypted(bl_message, meta->sender_s, meta->encrypt_context);
                close(meta->sender_s);
                meta->sender_s = init_socket(SPORT);
                temp_ip=temp_ip->next;
            }
            free(bl_message->message);
            free(bl_message);
        }
        unlock(meta);
        char* message = calloc(MAXMSGLEN, 1);
        wgetnstr(meta->message_sender,message,MAXMSGLEN-1);
        if(message[0]=='/'&&message[1]=='q'&&message[2]=='\0'){
            // send disconnect
            lock(meta);
            if(meta->ip_count > 1){
                Message disconnect = calloc(1,sizeof(struct message_s));
                disconnect->identifier = DISCONNECT;
                disconnect->size = 1;
                IP_List temp_ip = meta->ip_list->next;
                for(int ip = 1; ip < meta->ip_count; ip++){
                    struct sockaddr_in node;
                    node.sin_family = AF_INET;
                    node.sin_port = htons(RPORT);
                    node.sin_addr.s_addr = temp_ip->ip;
                    while(connect(meta->sender_s, (struct sockaddr *)&node,sizeof(node)) < 0);
                    send_message_encrypted(disconnect, meta->sender_s, meta->encrypt_context);
                    close(meta->sender_s);
                    meta->sender_s = init_socket(SPORT);
                    temp_ip=temp_ip->next;
                }
                free(disconnect);
            }
            display(meta);
            unlock(meta);
            meta->lock = 2;
            shutdown(meta->reciever_s,SHUT_RDWR);
        }
        else if(message[0]=='/'&&message[1]=='l'&&message[2]=='\0'){
            lock(meta);
            char* mes = calloc(30+(20*(meta->ip_count)),1);
            strcat(mes,"Connected:\n");
            IP_List temp = meta->ip_list;
            while(temp){
                strcat(mes,"\t     ");
                strcat(mes,temp->nick);
                if(temp->next)
                    strcat(mes,"\n");
                temp = temp->next;
            }
            MSG_add(mes,"~",time(NULL),&meta->messages);
            display(meta);
            unlock(meta);
        }
        else if(message[0]=='/'&&message[1]=='k'&&message[2]=='\0'){
            generate_key_256();
            char* mes = "new key generated and placed in keys dir.";
            lock(meta);
            MSG_add(mes,"~",time(NULL),&meta->messages);
            display(meta);
            unlock(meta);
        }
        else if(message[0]=='/'&&message[1]=='h'&&message[2]=='\0'){
            char* mes = "/q: quit\n\t     /h: this message\n\t     /l: list online\n\t     /k: generate new key\n\t     /f [filename in .darksync/files]: send a file";
            lock(meta);
            MSG_add(mes,"~",time(NULL),&meta->messages);
            display(meta);
            unlock(meta);
        }
        else if(message[0]=='/'&&message[1]=='f'&&message[2]==' '){
	        //get file path
	        char fp[256] = {0};
            int pointer = 3;
            while((pointer-3)<256&&message[pointer]!='\0'){
                fp[pointer-3] = message[pointer];
                if(message[pointer]=='\0'){
                    break; // string name complete
                }
                pointer++;
            }
            if((pointer-3)==255){
                wprintw(meta->message_sender, "Filename length to long\n");
            }
            else{
                char path[1024] = {0};
                strcat(path, getenv("HOME"));
                strcat(path,"/.darksync/");
                strcat(path,fp);
                FILE* fts = fopen(path,"r");
                if(!fts){
                    wprintw(meta->message_sender,"File not found.\n");
                }
                else{
                    fseek(fts, 0L, SEEK_END);
                    int sz = ftell(fts);
                    rewind(fts);
                    if(sz > MAXFILESIZE){
                        wprintw(meta->message_sender,"File to large, must be less than %d bytes.\n",MAXFILESIZE);
                    }
                    else{
                        Message mes = calloc(1,sizeof(struct message_s));
                        mes->identifier = F_MSG;
                        mes->size = 1 + sz;
                        mes->message = calloc(1,sz+256+4);
                        memcpy(mes->message,fp,256);
                        *(mes->message+256) = sz;
                        fread(mes->message, 1, sz+256, fts);

                        lock(meta);
                        char* temp_nick = calloc(20,1);
                        memcpy(temp_nick, meta->nick, 20);
                        MSG_add(message, temp_nick, time(NULL), &(meta->messages));
                        free(temp_nick);
                        IP_List temp_ip = meta->ip_list->next;
                        for(int ip = 1; ip < meta->ip_count; ip++){
                            struct sockaddr_in node;
                            node.sin_family = AF_INET;
                            node.sin_port = htons(RPORT);
                            node.sin_addr.s_addr = temp_ip->ip;
                            while(connect(meta->sender_s, (struct sockaddr *)&node,sizeof(node)) < 0);
                            send_message_encrypted(mes, meta->sender_s, meta->encrypt_context);
                            close(meta->sender_s);
                            meta->sender_s = init_socket(SPORT);
                            temp_ip=temp_ip->next;
                        }
                        free(mes);
                        unlock(meta);
                    }
                }
            }
	    }
        else{ // normal message
            lock(meta);
            uint32_t t = (uint32_t)time(NULL);
            if(meta->ip_count > 1){
                Message mes = calloc(1,sizeof(struct message_s));
                mes->identifier = STD_MSG;
                mes->size = 1+MAXMSGLEN+4;
                mes->message = calloc(MAXMSGLEN+4,1);
                memcpy(mes->message,message,MAXMSGLEN);
                memcpy((mes->message)+MAXMSGLEN, &t, 4);
                // add message to messages
                char* temp_nick = calloc(20,1);
                memcpy(temp_nick, meta->nick, 20);
                MSG_add(message, temp_nick, t, &(meta->messages));
                free(temp_nick);
                IP_List temp_ip = meta->ip_list->next;
                for(int ip = 1; ip < meta->ip_count; ip++){
                    struct sockaddr_in node;
                    node.sin_family = AF_INET;
                    node.sin_port = htons(RPORT);
                    node.sin_addr.s_addr = temp_ip->ip;
                    while(connect(meta->sender_s, (struct sockaddr *)&node,sizeof(node)) < 0);
                    send_message_encrypted(mes, meta->sender_s, meta->encrypt_context);
                    close(meta->sender_s);
                    meta->sender_s = init_socket(SPORT);
                    temp_ip=temp_ip->next;
                }
                free(mes);
            }
            display(meta);
            unlock(meta);
        }
        free(message);
    }
    return 0;
}

// Locks
void lock(Metadata meta){
    while(meta->lock);
    meta->lock = 1;
}

void unlock(Metadata meta){
    meta->lock = 0;
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
        MSG_destroy(meta->messages);
        IPL_destroy(meta->ip_list);
        IPL_destroy(meta->blacklist);
        close(meta->reciever_s);
        close(meta->sender_s);
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
        args->node_ip = calloc(20,1);
        args->nickname = calloc(20,1);
        strncpy(args->key, argv[1], strlen(argv[1])+1);
        strncpy(args->nickname, argv[3], strlen(argv[3])+1);

        // Create Dirs
        create_directories();

        // Initialize Metadata
        Metadata meta = calloc(1,sizeof(struct metadata_s));
        memcpy(meta->nick,args->nickname,20);
        meta->ip_list = NULL;
        meta->blacklist = NULL;
        meta->messages = NULL;
        meta->emit_black = 0;
        meta->blacklist_count = 0;
        meta->encrypt_context = calloc(1,sizeof(struct AES_ctx));
        load_key(args->key,meta);
        load_blacklist(&meta->blacklist, meta);
        if(argv[2][0]=='p'){
            meta->ipassive = 1;
            strncpy(args->node_ip, "passive", 8);
        }
        else{
            strncpy(args->node_ip, argv[2], strlen(argv[2])+1);
            meta->ipassive = 0;
        }
        meta->ip_count = 1;
        meta->my_ip = get_ip_of_interface(argv[4]);
        if( !meta->my_ip ){
            fprintf(stderr,"%s is not a valid interface.\n",argv[4]);
            exit(EXIT_FAILURE);
        }
        IPL_add(meta->my_ip,&(meta->ip_list),meta->nick); //initial list only contains yourself
        meta->reciever_s = init_socket(RPORT);
        meta->sender_s = init_socket(SPORT);

        //AES init
        if(meta->keyloaded)
          AES_init_ctx(meta->encrypt_context,meta->key);

        // Screen
        initscr();
        int h, w;
        getmaxyx(stdscr, h, w);//print header
        if( w<=70 )
            w = 70;
        meta->win = newwin(h, w, 0, 0);
        meta->message_board = newwin(h-12,w-4,7,2);
        meta->messenger = newwin(5,w,h-5,0);
        meta->message_sender = newwin(3,w-6,h-4,4);
        scrollok(meta->message_board, TRUE);
        scrollok(meta->message_sender, TRUE);
        meta->banner = newwin(7,w,0,0);
        meta->status = newwin(7,30,0,w-30);

        display(meta);

        // ask for the itial nodes ip list
        if(!meta->ipassive){
            // create the message;
            Message request = calloc(1,sizeof(struct message_s));
            request->identifier = ACTIVE_NODES_REQ;
            request->size = 1+20; // ident and nick
            request->message = malloc(20);
            memcpy(request->message,meta->nick,20);
            // connect to node
            struct sockaddr_in node;
            node.sin_family = AF_INET;
            node.sin_port = htons(RPORT);
            if(inet_pton(AF_INET, args->node_ip, &node.sin_addr)<=0){
                printf("\nInvalid address/ Address not supported \n");
                exit(EXIT_FAILURE);
            }
            while(connect(meta->sender_s, (struct sockaddr *)&node, sizeof(node)) < 0);
            send_message_encrypted(request, meta->sender_s, meta->encrypt_context);
            free(request->message);
            free(request);
            int buf_len = (((MAXCONN*4)+2+20)/16)*16+(((MAXCONN*4)+2+20)%16)*16;
            uint8_t buffer[buf_len];
            for(int i = 0; i < buf_len; i++)
                buffer[i] = 0;
            read(meta->sender_s, buffer, buf_len);
            //AES_CBC_decrypt_buffer(meta->encrypt_context,buffer,buf_len);
            uint8_t size = buffer[1];
            //extract nickname
            char temp_nick[20] = {0};
            for( int c = 1; c <= 20; c++ ){
                temp_nick[c-1] = buffer[c-1];
            }
            //connection message
            char* conn = calloc(100,1);
            strcat(conn,inet_ntoa(node.sin_addr));
            strcat(conn," (");
            strcat(conn,temp_nick);
            strcat(conn,") connected.\0");
            MSG_add(conn,"~",time(NULL),&(meta->messages));
            free(conn);
            display(meta);
            //add to ip list
            int ipad = 2+20;
            for(ipad = ipad; ipad < (size*4)+2+20; ipad+=4){ // for each ip address
                uint32_t address = 0;
                for(int b = 0; b < 4; b++){ // each byte in address
                    address |= buffer[ipad+b]; // get the byte
                    if(b!=3) // shift if not the end byte
                        address <<= 8;
                }
                IPL_add(address,&(meta->ip_list),temp_nick); // add the ip to master list
                meta->ip_count++;
            }
            uint8_t black_size = buffer[ipad];
            ipad++;
            for(ipad = ipad; ipad < (size*4)+2+20+1+(black_size*4);ipad+=4){
                uint32_t address = 0;
                for(int b = 0; b < 4; b++){ // each byte in address
                    address |= buffer[ipad+b]; // get the byte
                    if(b!=3) // shift if not the end byte
                        address <<= 8;
                }
                char* nick = IPL_contains(address,meta->blacklist);
                if(!nick){
                    IPL_add(address,&(meta->blacklist),"bad"); // add the ip to master list
                    meta->blacklist_count++;
                }
                else
                    free(nick);
            }
            //reinit socket
            close(meta->sender_s);
            meta->sender_s = init_socket(SPORT);
            // say hello
            request = calloc(1,sizeof(struct message_s));
            request->identifier = HELLO;
            request->size = 1+20; // ident and nick
            request->message = malloc(20);
            memcpy(request->message,meta->nick,20);
            IP_List temp = meta->ip_list->next;
            for(int ip = 1; ip < meta->ip_count; ip++){
                // connect to node
                struct sockaddr_in node;
                node.sin_family = AF_INET;
                node.sin_port = htons(RPORT);
                node.sin_addr.s_addr = temp->ip;
                while(connect(meta->sender_s, (struct sockaddr *)&node, sizeof(node)) < 0);
                send_message_encrypted(request, meta->sender_s, meta->encrypt_context);
                close(meta->sender_s);
                meta->sender_s = init_socket(SPORT);
                temp=temp->next;
            }
            free(request->message);
            free(request);
        }

        // Initial display
        display(meta);

        // Initialize Threads
        pthread_t thread_id_reciever, thread_id_sender;
        void* thread_ret;
        pthread_create(&thread_id_reciever, NULL, message_reciever_worker, meta);
        pthread_create(&thread_id_sender, NULL, message_sender_worker, meta);
        pthread_join(thread_id_reciever, &thread_ret);
        pthread_join(thread_id_sender, &thread_ret);

        // Save the blacklist
        if(meta->blacklist)
            dump_blacklist(meta->blacklist);
        // Free the malloc
        destructor(args,meta);
        endwin();
    }
    return 0;
}
