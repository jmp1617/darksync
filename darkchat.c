#include "darkchat.h"

void print_usage(){
    fprintf(stderr,"Usage: darkchat [key] [node_ip] [nickname]\n \
            \tkey: AES key name\n \
            \tnode_ip: ip of active chat node\n \
            \tnickname: chat nickname\n\n \
            \tNote: place key file in $HOME/.darknet/keys dir\n");
}

int main(int argc, char* argv[]){
    if( argc != 4 )
        print_usage();
    else{
        // Load arguments
        Arguments args = calloc(1, sizeof(struct arguments_s)); //TODO free
        args->key = calloc(strlen(argv[1])+1, sizeof(char)); //TODO free
        args->node_ip = calloc(strlen(argv[2])+1, sizeof(char)); //TODO free
        args->nickname = calloc(strlen(argv[3])+1, sizeof(char)); //TODO free
        strncpy(args->key, argv[1], strlen(argv[1])+1);
        strncpy(args->node_ip, argv[2], strlen(argv[2])+1);
        strncpy(args->nickname, argv[3], strlen(argv[3])+1);
        printf("%s, %s, %s\n",args->key,args->node_ip,args->nickname);
    }
    return 0;
}
