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

typedef enum TokType {
    Method,
    SP,
    RequestURI,
    HTTPVer,
    CRLF,
    RequestHeader,
    HeaderVal,
    StatusCode,
    ReasonPhrase,
    Body
} TokType;

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

void parse_request(char *req) {}

void embed_body(char *res) {
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("data/index.html", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
        strcat(res, line);
    }

    fclose(fp);
    if (line)
        free(line);
    return;
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

    printf("server: listening on Port 3490\n");
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

        parse_request(buf);

        // TODO: handle client requests externally
        char clientResponse[100] =
            ""; // initialize empty to auto terminate with \0
        for (int i = 0; i < strlen(buf); i++) {
            if ((buf[i] == '\r' && buf[i + 1] == '\n') || buf[i] == '\n') {
                break;
            }
            clientResponse[i] = buf[i];
        }
        printf("Header: '%s'\n", clientResponse);

        char *url = malloc(sizeof(char) *
                           20); // initialize empty to auto terminate with \0
        // char *new = realloc(url, 200);
        char *fromSlash = strstr(clientResponse, "/");
        for (int i = 0; i < strlen(fromSlash); i++) {
            if (fromSlash[i] == ' ') {
                break;
            }
            url[i] = fromSlash[i];
        }

        printf("URL: '%s'\n", url);

        // TODO: handle strings elegantly, maybe using functions to contruct
        // response line, headers & body.
        if (!fork()) {
            close(sockfd); // child doesn't need the listener
            // FIXME: string size ??? ig maybe check when strcat or dynamic
            // something wtf?
            char res[500] = ""; // initialize empty to auto terminate with \0
            strcat(res, "HTTP/1.1 200 OK\r\n"); // response / status line
            strcat(res, "");                    // header lines, ending in \r\n
            strcat(res, "\r\n"); // empty CRLF to indicate message body start
            // strcat(res, "<html><body>");
            // strcat(res, url);
            // strcat(res, "</body></html>");
            embed_body(res); // message body
            // printf(">>>%s\n", res);

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
        free(url);
        close(new_fd); // parent doesn't need this
    }

    return 0;
}
