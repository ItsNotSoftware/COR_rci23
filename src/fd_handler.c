#include "fd_handler.h"

FdHandler fd_handler = {0};
extern int tcp_server_fd;

void fd_handler_init() { FD_ZERO(&fd_handler.current_sockets); }

void fd_restore() { fd_handler.ready_sockets = fd_handler.current_sockets; }

void fd_add(int fd) {
    FD_SET(fd, &fd_handler.current_sockets);
    fd_handler.fds[fd_handler.fd_count++] = fd;

    fd_handler.max_fd = fd > fd_handler.max_fd ? fd : fd_handler.max_fd;
}

bool fd_is_set(int fd) { return FD_ISSET(fd, &fd_handler.ready_sockets); }

void fd_handler_select() {
    fd_restore();

    if (select(fd_handler.max_fd + 1, &fd_handler.ready_sockets, NULL, NULL, NULL) < 0) {
        perror("[SELECT ERROR]\n");
        exit(EXIT_FAILURE);
    }
}

bool fd_is_new_connection(int fd) { return fd == tcp_server_fd; }

bool fd_is_user_cmd(int fd) { return fd == STDIN_FILENO; }

void fd_remove(int fd) {
    close(fd);
    FD_CLR(fd, &fd_handler.current_sockets);

    // Remove the fd from the list
    for (int i = 0; i < fd_handler.fd_count; i++) {
        if (fd_handler.fds[i] == fd) {
            for (int j = i; j < fd_handler.fd_count - 1; j++) {
                fd_handler.fds[j] = fd_handler.fds[j + 1];
            }
            fd_handler.fd_count--;
            break;
        }
    }

    // Update max_fd
    for (int i = 0; i < fd_handler.fd_count; i++) {
        fd_handler.max_fd =
            fd_handler.fds[i] > fd_handler.max_fd ? fd_handler.fds[i] : fd_handler.max_fd;
    }
}

int get_max_fd() { return fd_handler.max_fd; }
