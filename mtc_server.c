#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "utils.h"
#include "protocol.h"
#define MAX_CLIENT 4

int init_sd(int myport);
void do_service(int sd, int indice);
struct client all_client[MAX_CLIENT];
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
int count_client = 0;
static int verbose = 0;

#define PRINT(...)                         \
    do {                                   \
        if (verbose)                       \
            fprintf(stderr, __VA_ARGS__);  \
    } while (0)


void *body(void *args){

    struct sockaddr_in c_add;
    int base_sd, curr_sd;
    socklen_t addrlen;
    int myport;
    int err = 0;
    int indice = 0;
    pthread_mutex_lock(&mutex1);
    indice = count_client++;
    pthread_mutex_unlock(&mutex1);
    base_sd = (int) args;
    //début mutex
    // indice = count
    // count++
    //fin mutex
    int opt;
    while (!err) {
        addrlen = sizeof(c_add);
        curr_sd = accept(base_sd, CAST_ADDR(&c_add), &addrlen);
        if (curr_sd < 0)
            SYS_ERR("Accept failed!");

        PRINT("Client connected\n");
        do_service(curr_sd, indice);
        close(curr_sd);  
    }
    close(base_sd);
}

int main(int argc, char *argv[])
{
    struct sockaddr_in c_add;
    int base_sd, curr_sd;
    socklen_t addrlen;
    int myport;
    int err = 0;
    int opt;
  
    if (argc < 2)
        USR_ERR("usage: server [-v] <port>");

    while ((opt = getopt(argc, argv, "v")) != -1) {
        if (opt == 'v') verbose = 1;
        else USR_ERR("usage: server [-v] <port>");
    }

    if (optind >= argc) USR_ERR("Missing port. Usage: server [-v] <port>");
        
    myport = atoi(argv[optind]);
  
    base_sd = init_sd(myport);

    for ( int i =0 ; i < MAX_CLIENT ; i ++) {
        pthread_create (&all_client[i].tid, 0 , body , ( void *) base_sd );
    }
    while(1);
}


int init_sd(int myport)
{
    struct sockaddr_in my_addr;
    int sd;
    
    bzero(&my_addr, sizeof(struct sockaddr_in));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(myport);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    PRINT("Preparing socket\n");
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) SYS_ERR("Error in creating socket");
    
    if (bind(sd, CAST_ADDR(&my_addr), sizeof(my_addr)) < 0)
        SYS_ERR("Bind failed!");

    if (listen(sd, 5) < 0)
        SYS_ERR("listen failed!");
    
    PRINT("Server listening on port %d\n", myport);

    return sd;
}

void do_service(int sd, int indice)
{
    int i, l=1;
    char buffer[MSG_SIZE];

    struct message msg;
    bzero(&msg, sizeof(msg)); // just to avoid writing uninitialized data in the socket
 
    // first read the pseudo
    int r = read(sd, buffer, PSEUDO_SIZE);
    if (r < 0) SYS_ERR("Error in reading from the socket");

    buffer[r] = 0;
    char mypseudo[PSEUDO_SIZE];
    strcpy(mypseudo, buffer);
    PRINT("Received pseudo %s\n", mypseudo);
    strcpy(all_client[indice].pseudo, mypseudo);
    all_client[indice].socket = sd;
        
    do {
        l = read(sd, &msg, sizeof(msg));
        if (l == 0) break;
        strcpy(buffer, msg.msg);
        PRINT("Server: received %s\n", buffer);

        i = 0;
        while (buffer[i] != '\0') {
            buffer[i] = toupper(buffer[i]);
            i++;
        }
        PRINT("Server: sending %s\n", buffer);

        strcpy(msg.pseudo, mypseudo);
        strcpy(msg.msg, buffer);
        pthread_mutex_lock(&mutex2);
        for(int i=0; i<MAX_CLIENT; i++){
            write(all_client[i].socket, &msg, sizeof(msg));
        }
        pthread_mutex_unlock(&mutex2);
    } while (1);
}
