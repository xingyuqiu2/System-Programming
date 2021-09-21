/**
 * nonstop_networking
 * CS 241 - Spring 2021
 */
#include "format.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netdb.h>

#include "common.h"

static int socket_fd;

char **parse_args(int argc, char **argv);
verb check_args(char **args);

void connect_to_server(const char *host, const char *port) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int s = getaddrinfo(host, port, &hints, &res);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        freeaddrinfo(res);
        res = NULL;
        exit(1);
    }
    socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (socket_fd == -1) {
        freeaddrinfo(res);
        res = NULL;
        perror("socket");
        exit(1);
    }
    int ok = connect(socket_fd, res->ai_addr, res->ai_addrlen);
    if (ok == -1) {
        freeaddrinfo(res);
        res = NULL;
        perror("connect");
        exit(1);
    }
    freeaddrinfo(res);
    res = NULL;
}

void client_exit(char** args, int status) {
    close(socket_fd);
    free(args);
    exit(status);
}

int main(int argc, char **argv) {
    // Good luck!
    char** args = parse_args(argc, argv);
    verb method_verb = check_args(args);
    char* host = args[0];
    char* port = args[1];
    char* method = args[2];
    char* remote = args[3];
    char* local = args[4];
    
    connect_to_server(host, port);

    //client request
    if (method_verb == LIST) {
        //send client request
        char* buffer = calloc(1, strlen(method) + 2);
        sprintf(buffer, "%s\n", method);
        size_t count = strlen(buffer);
        ssize_t bytes_written = write_all_to_socket(socket_fd, buffer, count);
        free(buffer);
        if (bytes_written == -1 || (size_t)bytes_written < count) {
            print_connection_closed();
            client_exit(args, 1);
        }
        if (shutdown(socket_fd, SHUT_WR) == -1) {
            perror("shutdown");
        }

    } else if (method_verb == GET || method_verb == DELETE) {
        //send client request
        char* buffer = calloc(1, strlen(method) + strlen(remote) + 3);
        sprintf(buffer, "%s %s\n", method, remote);
        size_t count = strlen(buffer);
        ssize_t bytes_written = write_all_to_socket(socket_fd, buffer, count);
        free(buffer);
        if (bytes_written == -1 || (size_t)bytes_written < count) {
            print_connection_closed();
            client_exit(args, 1);
        }
        if (shutdown(socket_fd, SHUT_WR) == -1) {
            perror("shutdown");
        }

    } else if (method_verb == PUT) {
        //send client request
        char* buffer = calloc(1, strlen(method) + strlen(remote) + 3);
        sprintf(buffer, "%s %s\n", method, remote);
        size_t count = strlen(buffer);
        ssize_t bytes_written = write_all_to_socket(socket_fd, buffer, count);
        free(buffer);
        if (bytes_written == -1 || (size_t)bytes_written < count) {
            print_connection_closed();
            client_exit(args, 1);
        }
        //write local file to server
        //get stat
        struct stat s;
        if (stat(local, &s) == -1) exit(1);
        size_t file_size = s.st_size;
        write_all_to_socket(socket_fd, (const char*)&file_size, sizeof(file_size));
        //open local file
        FILE* local_file = fopen(local, "r");
        if (!local_file) {
            //file not exist
            print_connection_closed();
            client_exit(args, 1);
        }
        if (write_localfile_to_socket(socket_fd, local_file, file_size) == -1) {
            //close local file
            fclose(local_file);
            print_connection_closed();
            client_exit(args, 1);
        }
        //close local file
        fclose(local_file);
        if (shutdown(socket_fd, SHUT_WR) == -1) {
            perror("shutdown");
        }
    }

    //server response
    char* response = calloc(1, 4);
    ssize_t response_bytes_read = read_all_from_socket(socket_fd, response, 3);
    if (response_bytes_read == -1) {
        print_connection_closed();
        client_exit(args, 1);
    }
    if (!strcmp(response, "OK\n")) {
        //OK
        printf("OK\n");
        if (method_verb == PUT || method_verb == DELETE) {
            print_success();
        } else if (method_verb == GET) {
            size_t file_size;
            read_all_from_socket(socket_fd, (char*)&file_size, sizeof(file_size));
            FILE* local_file = fopen(local, "w+");
            if (!local_file) {
                print_connection_closed();
                client_exit(args, 1);
            }
            size_t total_bytes_read = 0;
            size_t blocksize = 512;
            char buf[blocksize];
            //in order to detect too much data error, enlarge the max size
            size_t max_size = file_size + 5;
            //read data from server and write to local file
            while (total_bytes_read < max_size) {
                if (max_size - total_bytes_read < blocksize) {
                    blocksize = max_size - total_bytes_read;
                }
                size_t bytes_read = read_all_from_socket(socket_fd, buf, blocksize);
                if (bytes_read == 0) break;
                fwrite(buf, sizeof(char), bytes_read, local_file);
                total_bytes_read += bytes_read;
            }
            //check error
            if (total_bytes_read < file_size) {
                print_too_little_data();
                client_exit(args, 1);
            } else if (total_bytes_read > file_size) {
                print_received_too_much_data();
                client_exit(args, 1);
            }
            fclose(local_file);

        } else if (method_verb == LIST) {
            size_t size;
            read_all_from_socket(socket_fd, (char*)&size, sizeof(size));
            //in order to detect too much data error, enlarge the max size
            size_t max_size = size + 5;
            char buf[max_size];
            memset(buf, 0, max_size);
            ssize_t bytes_read = read_all_from_socket(socket_fd, buf, max_size - 1);
            //check error
            if (bytes_read == -1) {
                print_connection_closed();
                client_exit(args, 1);
            }
            if ((size_t)bytes_read < size) {
                print_too_little_data();
                client_exit(args, 1);
            } else if ((size_t)bytes_read > size) {
                print_received_too_much_data();
                client_exit(args, 1);
            }
            printf("%zu%s", size, buf);
        }
    } else {
        //ERROR
        char* error = "ERROR\n";
        response = realloc(response, strlen(error) + 1);
        read_all_from_socket(socket_fd, response + response_bytes_read, strlen(error) - response_bytes_read);
        response[strlen(error)] = 0;
        if (!strcmp(response, error)) {
            char message[21];
            ssize_t message_read = read_all_from_socket(socket_fd, message, 20);
            message[20] = 0;
            if (message_read == -1) {
                print_connection_closed();
                client_exit(args, 1);
            }
            print_error_message(message);
        } else {
            print_invalid_response();
            client_exit(args, 1);
        }

    }
    free(response);
    if (shutdown(socket_fd, SHUT_RD) == -1) {
        perror("shutdown");
    }
    client_exit(args, 0);
}

/**
 * Given commandline argc and argv, parses argv.
 *
 * argc argc from main()
 * argv argv from main()
 *
 * Returns char* array in form of {host, port, method, remote, local, NULL}
 * where `method` is ALL CAPS
 */
char **parse_args(int argc, char **argv) {
    if (argc < 3) {
        return NULL;
    }

    char *host = strtok(argv[1], ":");
    char *port = strtok(NULL, ":");
    if (port == NULL) {
        return NULL;
    }

    char **args = calloc(1, 6 * sizeof(char *));
    args[0] = host;
    args[1] = port;
    args[2] = argv[2];
    char *temp = args[2];
    while (*temp) {
        *temp = toupper((unsigned char)*temp);
        temp++;
    }
    if (argc > 3) {
        args[3] = argv[3];
    }
    if (argc > 4) {
        args[4] = argv[4];
    }

    return args;
}

/**
 * Validates args to program.  If `args` are not valid, help information for the
 * program is printed.
 *
 * args     arguments to parse
 *
 * Returns a verb which corresponds to the request method
 */
verb check_args(char **args) {
    if (args == NULL) {
        print_client_usage();
        exit(1);
    }

    char *command = args[2];

    if (strcmp(command, "LIST") == 0) {
        return LIST;
    }

    if (strcmp(command, "GET") == 0) {
        if (args[3] != NULL && args[4] != NULL) {
            return GET;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "DELETE") == 0) {
        if (args[3] != NULL) {
            return DELETE;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "PUT") == 0) {
        if (args[3] == NULL || args[4] == NULL) {
            print_client_help();
            exit(1);
        }
        return PUT;
    }

    // Not a valid Method
    print_client_help();
    exit(1);
}
