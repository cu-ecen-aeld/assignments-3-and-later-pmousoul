/*
** aesdsocket.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#include <signal.h>

#include <syslog.h>

// struct sigaction incomplete error related
#define _XOPEN_SOURCE 700

#define PORT "9000"  // the port users will be connecting to
#define BACKLOG 10   // how many pending connections queue will hold

// max number of bytes we can get at once (8MiB)
#define MAXDATASIZE (8192*1024)

char* filename = "/var/tmp/aesdsocketdata";


void initialize_logging(const char *app_name) {
  openlog(app_name, LOG_PID | LOG_CONS, LOG_USER);
  syslog(LOG_INFO, "Logging initialized for %s", app_name);
}

void log_message(int priority, const char *message) {
    syslog(priority, "%s", message);
}

void sigchld_handler(int s) {
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

void sighandler(int dummy) {
    // write to syslog
    log_message(LOG_INFO, "Caught signal, exiting");

    // close the logging system
    closelog();

    // Attempt to delete the filename file
    if (remove(filename) == 0) {
        printf("Exiting: File deleted successfully.\n");
    } else {
        printf("Exiting: Error: Unable to delete the file.\n");
    }

    // exit gracefully calling exit()
    // completing any open connection operations,
    // closing any open sockets 
    exit(0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main(int argc, char**argv)
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    // initialize syslog logging
    initialize_logging(argv[0]);

    // socket structures setup
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "Server setup: getaddrinfo: %s\n", gai_strerror(rv));
        exit(-1);
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("Server setup: socket error.");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("Server setup: setsockopt error.");
            exit(-1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("Server setup: bind error.");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "Server setup: failed to bind.\n");
        return -1;
    }

    // listen to the socket
    if (listen(sockfd, BACKLOG) == -1) {
        perror("Server setup: listen error.");
        exit(-1);
    }

    // run as a daemon
    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        switch(fork()) {                  // become background process
            case -1: return -1;           // fork() failed
            case 0: break;                // child falls through
            default: exit(0); // parent terminates
        }

        if(setsid() == -1) {              // become leader of new session
            exit(-1);
        }
    }

    // Execute sighandler() when the following signals are received 
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
    
    // reap all dead processes
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("Server: sigaction error.");
        exit(-1);
    }

    printf("Server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof(their_addr);
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("Server: accept connection error.");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof(s));
        printf("Server: got connection from %s\n", s);

        // write accept connection message to syslog
        char message[100];
        sprintf(message,"%s%s", "Accepted connection from ", s);
        log_message(LOG_INFO, message);

        // create file descriptor for append to filename file
        int a_fd;
        a_fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (a_fd == -1) {
            perror("Clent service: File for append could not be created.");
            exit(-1);
        }

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener

            // allocate memory for reading data packet
            char* buffer = (char*)malloc(MAXDATASIZE * sizeof(char));
            if (buffer == NULL) {
                perror("Client sevice: Memory allocation for packet data failed!");
                exit(-1);
            }

            // receive from socket
            int numbytes = 0;
            if ((numbytes = recv(new_fd, buffer, MAXDATASIZE, 0)) == -1) {
                perror("Client service: Receive failed!");
                exit(-1);
            }

            // append to filename file
            if(buffer[numbytes-1] == '\n') {
                ssize_t nr;
                nr = write(a_fd, buffer, numbytes);
                if (nr == -1) {
                    perror("Client service: Appending to file failed.");
                    exit(-1);
                }
                else {
                    // sent to the client the content of the filename file
                    // by reading it line by line

                    // Create a file pointer and open the filename file in read mode.
                    FILE* file = fopen(filename, "r");

                    // Check if the file was opened successfully.
                    if (file != NULL) {
                        // Read each line from the file and store it in the
                        // 'line' buffer.
                        while (fgets(buffer, sizeof(buffer), file)) {
                            // Send each line to the client.
                            if (send(new_fd, buffer, strlen(buffer), 0) == -1) {
                                perror("Client service: send to client error.");
                            }
                        }

                        // Close the file stream once all lines have been read.
                        fclose(file);
                    }
                    else {
                        // Print an error message to the standard error
                        // stream if the file cannot be opened.
                        fprintf(stderr, "Client service: Unable to open file for reading!\n");
                    }
                }

            }

            free(buffer);
            close(new_fd);
            exit(0);
        }
        
        // close filename file descriptor
        close(a_fd);

        // close descriptor of accepted connection
        close(new_fd);  // parent doesn't need this

        // write closed connection message to syslog
        sprintf(message,"%s%s", "Closed connection from ", s);
        log_message(LOG_INFO, message);
    }

    // the following line is never reached
    return 0;
}