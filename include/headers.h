#ifndef HEADERS_H
#define HEADERS_H

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
#define STR_SIZE 256
#define MAX_NUMBER_OF_NODES 16

#define INFO(x) printf("[INFO]: %s\n", x)
#define ERROR(x) printf("[ERROR]: %s -> %s:%d\n", x, __FILE__, __LINE__)
#define WARNING(x) printf("[WARNING]: %s -> %s:%d\n", x, __FILE__, __LINE__)
#define DEBUG(x)                                             \
    printf("[DEBUG]: %s -> %s:%d\n", x, __FILE__, __LINE__); \
    fflush(stdout)
#define DEBUG2(x1, x2) printf("[DEBUG]: %s %s -> %s:%d\n", x1, x2, __FILE__, __LINE__)

#endif
