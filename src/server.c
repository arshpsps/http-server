#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT "3490" // the port users will be connecting to

#define BACKLOG 10 // how many pending connections queue will hold

#define MAXDATASIZE 1000 // max number of bytes we can get at once

void sigchld_handler(int s) {
    (void)s; // quiet unused variable warning

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int generate_response(char str[]) {
    str[0] = '1';
    return 1;
}

int main(void) {
    char buf[MAXDATASIZE];
    int sockfd, new_fd, numbytes; // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
            -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ==
            -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while (1) { // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("server: got connection from %s\n", s);

        if ((numbytes = recv(new_fd, buf, MAXDATASIZE - 1, 0)) == -1) {
            perror("recv");
        }

        buf[numbytes] = '\0';
        printf("server: received '%s'\n", buf);
        char header[100];
        for (int i = 0; i < strlen(buf); i++) {
            if (buf[i] == '\r' || buf[i] == '\n') {
                break;
            }
            header[i] = buf[i];
        }
        printf("Header: '%s'\n", header);

        char url[20] = ""; // initialize empty to auto terminate with \0
        char *fromSlash = strstr(header, "/");
        for (int i = 0; i < strlen(fromSlash); i++) {
            if (fromSlash[i] == ' ') {
                break;
            }
            url[i] = fromSlash[i];
        }

        printf("URL: '%s'\n", url);

        if (!fork()) {
            close(sockfd);      // child doesn't need the listener
            char res[150] = ""; // initialize empty to auto terminate with \0
            strcat(res, "HTTP/1.1 200 OK\r\n"); // response / status line
            strcat(res, "");                    // header lines, ending in \r\n
            strcat(res, "\r\n"); // empty CRLF to indicate message body start
            strcat(res, "<html><body>"); // message body
            strcat(res, url);
            strcat(res, "</body></html>");

            if (send(new_fd, res, strlen(res), 0) == -1)
                perror("send");

            close(new_fd);
            exit(0);
        }

        // if (!fork()) {     // this is the child process
        //     if (send(new_fd, "Hello, world!", 13, 0) == -1)
        //         perror("send");
        //     close(new_fd);
        //     exit(0);
        // }
        close(new_fd); // parent doesn't need this
    }

    return 0;
}
