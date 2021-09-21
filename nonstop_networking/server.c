/**
 * nonstop_networking
 * CS 241 - Spring 2021
 */
#include "format.h"
#include "common.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <errno.h>

#include "includes/dictionary.h"
#include "includes/vector.h"

typedef struct info {
    int status;
    verb method;
    char header[1024];
    char filename[128];
} info;

void clean_up();
void server_exit(int status);
void sigpipe_handler(int signal);
void close_server();
void epoll_mod_event(int clientfd);
void parse_header(int clientfd, info* client_info);
int list_handler(int clientfd);
int get_handler(int clientfd, info* client_info);
int delete_handler(int clientfd, info* client_info);
void serve_client(int clientfd);
void run_server(char *port);

static int serverSocket;
static char* tempdir;
static int epollfd;

static dictionary* dic_clientfd_info;
static dictionary* dic_filename_filesize;
static vector* vector_filename;
static size_t filename_size;    //size of all filenames(not including '\n'), used in LIST

//status of client
#define STATUS_HEADER_NOT_PARSED 0
#define STATUS_HEADER_PARSED 1
#define STATUS_ERR_BAD_REQUEST -1
#define STATUS_ERR_BAD_FILE_SIZE -2
#define STATUS_ERR_NO_SUCH_FILE -3

void clean_up() {
    //free memory
    close(epollfd);
    //free client_info
    vector* vector_infos = dictionary_values(dic_clientfd_info);
    for (size_t i = 0; i < vector_size(vector_infos); i++) {
        info* client_info = vector_get(vector_infos, i);
        free(client_info);
    }
    vector_destroy(vector_infos);
    //unlink files in tempdir
    for (size_t i = 0; i < vector_size(vector_filename); i++) {
        char* cur_filename = vector_get(vector_filename, i);
        size_t path_length = strlen(tempdir) + strlen(cur_filename) + 2;
        char path[path_length];
        memset(path, 0, path_length);
        sprintf(path, "%s/%s", tempdir, cur_filename);
        unlink(path);
    }
    //destroy data structures
    dictionary_destroy(dic_clientfd_info);
    dictionary_destroy(dic_filename_filesize);
    vector_destroy(vector_filename);
    //remove tempdir
    rmdir(tempdir);
}

void server_exit(int status) {
    clean_up();
    exit(status);
}

//call when SIGPIPE is sent
void sigpipe_handler(int signal) {
    if (signal == SIGPIPE) {
        //Do nothing
    }
}

//call when SIGINT is sent
void close_server() {
    server_exit(1);
}

void epoll_mod_event(int clientfd) {
    struct epoll_event new_ev;
    memset(&new_ev, 0, sizeof(new_ev));
    new_ev.events = EPOLLOUT;
    new_ev.data.fd = clientfd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, clientfd, &new_ev);
}

//parse header and set status
void parse_header(int clientfd, info* client_info) {
    size_t total_amount_read = 0;
    size_t limit = 263;
    while (total_amount_read < limit) {
        //read one bit each time
        ssize_t bytes_read = read(clientfd, client_info->header + total_amount_read, 1);
        if (client_info->header[strlen(client_info->header) - 1] == '\n') {
            //read '\n', should stop
            break;
        } else if (bytes_read > 0) {
            total_amount_read += bytes_read;
        } else if (bytes_read == -1 && errno == EINTR) {
            continue;
        } else {
            //bytes_read == 0 || bytes_read == -1
            client_info->status = STATUS_ERR_BAD_REQUEST;
            break;
        }
    }
    if (total_amount_read >= limit) {
        client_info->status = STATUS_ERR_BAD_REQUEST;
    }
    if (client_info->status == STATUS_ERR_BAD_REQUEST) {
        //error occur
        print_invalid_response();
        epoll_mod_event(clientfd);
        return;
    }
    //determine the method of the request
    if (!strncmp("LIST\n", client_info->header, 5)) {
        //LIST
        client_info->method = LIST;
    } else if (!strncmp("GET ", client_info->header, 4)) {
        //GET
        client_info->method = GET;
        strcpy(client_info->filename, client_info->header + 4);
        client_info->filename[strlen(client_info->filename) - 1] = '\0';
    } else if (!strncmp("DELETE ", client_info->header, 7)) {
        //DELETE
        client_info->method = DELETE;
        strcpy(client_info->filename, client_info->header + 7);
        client_info->filename[strlen(client_info->filename) - 1] = '\0';
    } else if (!strncmp("PUT ", client_info->header, 4)) {
        //PUT
        client_info->method = PUT;
        strcpy(client_info->filename, client_info->header + 4);
        client_info->filename[strlen(client_info->filename) - 1] = '\0';

        //read size and data
        size_t path_length = strlen(tempdir) + strlen(client_info->filename) + 2;
        char path[path_length];
        memset(path, 0, path_length);
        sprintf(path, "%s/%s", tempdir, client_info->filename);
        int flag_file_exists = 0;
        if (access(path, F_OK) == 0) {
            //file exists
            flag_file_exists = 1;
        }
        FILE* file = fopen(path, "w+");
        size_t file_size;
        read_all_from_socket(clientfd, (char*)&file_size, sizeof(file_size));
        size_t total_bytes_read = 0;
        size_t blocksize = 512;
        char buf[blocksize];
        //in order to detect too much data error, enlarge the max size
        size_t max_size = file_size + 5;
        //read data from client and write to local file
        while (total_bytes_read < max_size) {
            if (max_size - total_bytes_read < blocksize) {
                blocksize = max_size - total_bytes_read;
            }
            size_t bytes_read = read_all_from_socket(clientfd, buf, blocksize);
            if (bytes_read == 0) break;
            fwrite(buf, sizeof(char), bytes_read, file);
            total_bytes_read += bytes_read;
        }
        fclose(file);
        //check error
        if (total_bytes_read != file_size) {
            unlink(path);
            client_info->status = STATUS_ERR_BAD_FILE_SIZE;
            epoll_mod_event(clientfd);
            return;
        }
        if (!flag_file_exists) {
            //file did not exist before and is newly created
            //push back to the vector that stores filenames in the tempdir
            vector_push_back(vector_filename, client_info->filename);
            filename_size += strlen(client_info->filename);
        }
        //store the file size in the dic_filename_filesize
        dictionary_set(dic_filename_filesize, client_info->filename, &file_size);
    } else {
        //bad request
        print_invalid_response();
        client_info->status = STATUS_ERR_BAD_REQUEST;
        epoll_mod_event(clientfd);
        return;
    }
    //header is parsed
    client_info->status = STATUS_HEADER_PARSED;
    epoll_mod_event(clientfd);
}

int list_handler(int clientfd) {
    //LIST
    write_all_to_socket(clientfd, "OK\n", 3);
    size_t size = vector_size(vector_filename) + filename_size;
    if (vector_size(vector_filename) > 0) size--;
    write_all_to_socket(clientfd, (char*)&size, sizeof(size));
    for (size_t i = 0; i < vector_size(vector_filename); i++) {
        char* cur_filename = vector_get(vector_filename, i);
        write_all_to_socket(clientfd, cur_filename, strlen(cur_filename));
        if (i != vector_size(vector_filename) - 1) {
            write_all_to_socket(clientfd, "\n", 1);
        }
    }
    return 0;
}

int get_handler(int clientfd, info* client_info) {
    //GET
    size_t path_length = strlen(tempdir) + strlen(client_info->filename) + 2;
    char path[path_length];
    memset(path, 0, path_length);
    sprintf(path, "%s/%s", tempdir, client_info->filename);
    FILE* file = fopen(path, "r");
    if (!file) {
        //file not exist
        client_info->status = STATUS_ERR_NO_SUCH_FILE;
        epoll_mod_event(clientfd);
        return -1;
    }
    write_all_to_socket(clientfd, "OK\n", 3);
    size_t size = *(size_t*)dictionary_get(dic_filename_filesize, client_info->filename);
    write_all_to_socket(clientfd, (char*)&size, sizeof(size));
    if (write_localfile_to_socket(clientfd, file, size) == -1) {
        client_info->status = STATUS_ERR_NO_SUCH_FILE;
        epoll_mod_event(clientfd);
        return -1;
    }
    //close file
    fclose(file);
    return 0;
}

int delete_handler(int clientfd, info* client_info) {
    //DELETE
    size_t path_length = strlen(tempdir) + strlen(client_info->filename) + 2;
    char path[path_length];
    memset(path, 0, path_length);
    sprintf(path, "%s/%s", tempdir, client_info->filename);
    if (access(path, F_OK) != 0) {
        //file not exist
        client_info->status = STATUS_ERR_NO_SUCH_FILE;
        epoll_mod_event(clientfd);
        return -1;
    }
    unlink(path);
    for (size_t i = 0; i < vector_size(vector_filename); i++) {
        if (!strcmp(vector_get(vector_filename, i), client_info->filename)) {
            //found
            filename_size -= strlen(client_info->filename);
            dictionary_remove(dic_filename_filesize, client_info->filename);
            vector_erase(vector_filename, i);
            write_all_to_socket(clientfd, "OK\n", 3);
            return 0;
        }
    }
    //should not reach this
    client_info->status = STATUS_ERR_NO_SUCH_FILE;
    epoll_mod_event(clientfd);
    return -1;
}

//check status to assign job
void serve_client(int clientfd) {
    info* client_info = dictionary_get(dic_clientfd_info, &clientfd);
    int status = client_info->status;
    switch (status)
    {
    case STATUS_HEADER_NOT_PARSED:
        //Header not parsed yet
        parse_header(clientfd, client_info);
        return;
    case STATUS_HEADER_PARSED:
        //Header parsed
        //what type of request was it?
        if (client_info->method == PUT) {
            //already done in parse_header
            write_all_to_socket(clientfd, "OK\n", 3);
        } else if (client_info->method == LIST) {
            if (list_handler(clientfd) == -1) return;
        } else if (client_info->method == GET) {
            if (get_handler(clientfd, client_info) == -1) return;
        } else if (client_info->method == DELETE) {
            if (delete_handler(clientfd, client_info) == -1) return;
        }
        break;
    case STATUS_ERR_BAD_REQUEST:
        //err_bad_request
        write_all_to_socket(clientfd, "ERROR\n", 6);
        write_all_to_socket(clientfd, err_bad_request, strlen(err_bad_request));
        break;
    case STATUS_ERR_BAD_FILE_SIZE:
        //err_bad_file_size
        write_all_to_socket(clientfd, "ERROR\n", 6);
        write_all_to_socket(clientfd, err_bad_file_size, strlen(err_bad_file_size));
        break;
    case STATUS_ERR_NO_SUCH_FILE:
        //err_no_such_file
        write_all_to_socket(clientfd, "ERROR\n", 6);
        write_all_to_socket(clientfd, err_no_such_file, strlen(err_no_such_file));
        break;
    }

    //client service done
    epoll_ctl(epollfd, EPOLL_CTL_DEL, clientfd, NULL);
    free(client_info);
    dictionary_remove(dic_clientfd_info, &clientfd);
    shutdown(clientfd, SHUT_RDWR);
    close(clientfd);
}

void run_server(char *port) {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror(NULL);
        server_exit(1);
    }
    int optval = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror(NULL);
        server_exit(1);
    }
    optval = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) == -1) {
        perror(NULL);
        server_exit(1);
    }
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int s = getaddrinfo(NULL, port, &hints, &res);
    if (s != 0) {
        freeaddrinfo(res);
        res = NULL;
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        server_exit(1);
    }
    if (bind(serverSocket, res->ai_addr, res->ai_addrlen) != 0) {
        freeaddrinfo(res);
        res = NULL;
        perror(NULL);
        server_exit(1);
    }
    int max_client = 100;
    if (listen(serverSocket, max_client) != 0) {
        freeaddrinfo(res);
        res = NULL;
        perror(NULL);
        server_exit(1);
    }
    freeaddrinfo(res);
    res = NULL;

    epollfd = epoll_create(42);
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = serverSocket;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, serverSocket, &ev);
    struct epoll_event array[max_client];

    while (1) {
        int num_events = epoll_wait(epollfd, array, max_client, 1000);
        if (num_events == -1) {
            server_exit(1);
        }
        for (int i = 0; i < num_events; i++) {
            int fd = array[i].data.fd;
            if (fd == serverSocket) {
                int clientfd = accept(serverSocket, NULL, NULL);
                if (clientfd == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                    server_exit(1);
                }
                struct epoll_event new_ev;
                memset(&new_ev, 0, sizeof(new_ev));
                new_ev.events = EPOLLIN;
                new_ev.data.fd = clientfd;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &new_ev);
                //add information to dic_clientfd_info
                info* information = calloc(1, sizeof(info));
                dictionary_set(dic_clientfd_info, &clientfd, information);
            } else {
                serve_client(fd);
            }
        }
    }
}

int main(int argc, char **argv) {
    // good luck!
    if (argc != 2) {
        print_server_usage();
        exit(1);
    }
    char* port = argv[1];
    
    //handle signal
    signal(SIGPIPE, sigpipe_handler);
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = close_server;
    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction");
        exit(1);
    }

    //make temp directory
    char template[] = "XXXXXX";
    tempdir = mkdtemp(template);
    print_temp_directory(tempdir);
    
    //create data structures
    dic_clientfd_info = int_to_shallow_dictionary_create();
    dic_filename_filesize = string_to_unsigned_int_dictionary_create();
    vector_filename = string_vector_create();

    //run the server
    run_server(port);
}
