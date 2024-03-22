#include "fd_handler.h"

FdHandler fd_handler;
extern int tcp_server_fd;

/*
 * Initializes the fd_handler struct
 */
void fd_handler_init() {
    FdHandler new = {0};
    fd_handler = new;
    FD_ZERO(&fd_handler.current_sockets);
}

/*
 * Restores the ready_sockets to the current_sockets
 */
void fd_restore() { fd_handler.ready_sockets = fd_handler.current_sockets; }

void fd_add(int fd) {
    FD_SET(fd, &fd_handler.current_sockets);
    fd_handler.fds[fd_handler.fd_count++] = fd;

    fd_handler.max_fd = fd > fd_handler.max_fd ? fd : fd_handler.max_fd;
}

/*
 * Checks if the fd is set in the ready_sockets
 *
 * @param fd: the file descriptor to check
 *
 * @return: true if the fd is set, false otherwise
 */
bool fd_is_set(int fd) { return FD_ISSET(fd, &fd_handler.ready_sockets); }

/*
 * Select warpper
 */
void fd_handler_select() {
    fd_restore();

    if (select(fd_handler.max_fd + 1, &fd_handler.ready_sockets, NULL, NULL, NULL) < 0) {
        perror("[SELECT ERROR]\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Checks if the fd is a new connection
 *
 * @param fd: the file descriptor to check
 *
 * @return: true if the fd is a new connection, false otherwise
 */
bool fd_is_new_connection(int fd) { return fd == tcp_server_fd; }

/*
 * Checks if the fd is a user command
 *
 * @param fd: the file descriptor to check
 *
 * @return: true if the fd is a user command, false otherwise
 */
bool fd_is_user_cmd(int fd) { return fd == STDIN_FILENO; }

/*
 * Checks if the fd is a client
 *
 * @param fd: the file descriptor to check
 *
 * @return: true if the fd is a client, false otherwise
 */
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

/*
 * Checks if the fd is a client
 *
 * @param fd: the file descriptor to check
 *
 * @return: true if the fd is a client, false otherwise
 */
int get_max_fd() { return fd_handler.max_fd; }
