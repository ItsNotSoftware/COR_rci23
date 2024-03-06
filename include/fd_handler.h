#ifndef FD_HANDLER_H
#define FD_HANDLER_H

#include "headers.h"

typedef struct
{
    int max_fd;
    int fds[101]; // open file descriptors
    int fd_count;

    fd_set current_sockets, ready_sockets;
} FdHandler;

void fd_handler_init();
void fd_add(int fd);
bool fd_is_set(int fd);
void fd_handler_select();
bool fd_is_new_connection(int fd);
bool fd_is_user_cmd(int fd);
void fd_remove(int fd);
int get_max_fd();

#endif
