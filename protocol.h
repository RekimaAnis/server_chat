#pragma once

#define MSG_SIZE    128
#define PSEUDO_SIZE 16
#define OUTPUT_BUFFER_SIZE 148

struct message {
    char pseudo[PSEUDO_SIZE];
    char msg[MSG_SIZE];
};

struct client{
    char pseudo[PSEUDO_SIZE];
    int socket;
    pthread_t tid;
};
