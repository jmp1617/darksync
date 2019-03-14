#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

//------- Structures
// User Input
struct arguments_s{
    char* key;
    char* node_ip;
    char* nickname;
};

typedef struct arguments_s* Arguments;

//------- Usage
void print_usage();
