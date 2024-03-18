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

#define INFO(msg) printf("[INFO]: %s\n", msg)
#define ERROR(msg) printf("[ERROR]: %s -> %s:%d\n", msg, __FILE__, __LINE__)
#define WARNING(msg) printf("[WARNING]: %s -> %s:%d\n", msg, __FILE__, __LINE__)
#define DEBUG(msg) printf("[DEBUG]: %s -> %s:%d\n", msg, __FILE__, __LINE__)
#define DEBUG2(msg1, msg2) printf("[DEBUG]: %s %s -> %s:%d\n", msg1, msg2, __FILE__, __LINE__)

#endif
