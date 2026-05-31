#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define DEFAULT_PORT 8080

int cast_to_int(const char *arg) {
    char *endptr;
    strtol(arg, &endptr, 10);
    if (*endptr == '\0') {
        return atoi(arg);
    }
    return DEFAULT_PORT;
}

int main(int argc, char *argv[]) {
    if (argc < 3 || strcmp(argv[1], "--port") != 0) {
        perror("Invalid command usage. Missing --port argument.");
        exit(1);
    }

    struct sockaddr_in addr;
    int port = cast_to_int(argv[2]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (errno == -1) {
        perror("Failed to create a socket.");
        exit(-1);
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    int res = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if (errno == -1 || res < 0) {
        printf("Failed to listen on port %d", port);
        close(sockfd);
        exit(-1);
    }

    int l_res = listen(sockfd, 0);
    if (errno == -1 || l_res < 0) {
        perror("Failed to listen to the server.");
        close(sockfd);
        exit(-1);
    }
    
    socklen_t addr_len = sizeof(addr);
    int acceptfd = accept(sockfd, (struct sockaddr *)&addr, &addr_len); 
    if (errno == -1  || acceptfd < 0) {
        perror("Failed to accept incoming connections");
        close(sockfd);
        close(acceptfd);
        exit(-1);
    }

    while(1) {
        sleep(1);
        printf("Connection is being accepted ...");
    }

    close(acceptfd);
    close(sockfd);
    return 0;
}
