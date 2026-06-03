#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_PORT 8080
#define HTTP_REQUEST_SIZE 2048
#define HTTP_REQUEST_PATH_MAX_SIZE 256
#define HTTP_REQUEST_VERB_MAX_SIZE 16
#define RESPONSE_SIZE 16384

char *load_file(const char *path) {
    FILE *file = fopen(path, "rb");

    if (file == NULL) {
        return NULL;
    }

    if (fseek(file, 0L, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    long file_size = ftell(file);

    if (file_size < 0) {
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0L, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    char *buffer = malloc(file_size + 1);

    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, file_size, file);

    if (bytes_read != (size_t)file_size) {
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[file_size] = '\0';

    fclose(file);

    return buffer;
}

void identify_request(char http_request[],
                      char *verb,
                      char *path) {
    char *token = strtok(http_request, " ");

    if (token != NULL) {
        strncpy(verb, token, HTTP_REQUEST_VERB_MAX_SIZE - 1);
        verb[HTTP_REQUEST_VERB_MAX_SIZE - 1] = '\0';
    }

    token = strtok(NULL, " ");

    if (token != NULL) {
        strncpy(path, token, HTTP_REQUEST_PATH_MAX_SIZE - 1);
        path[HTTP_REQUEST_PATH_MAX_SIZE - 1] = '\0';
    }
}

int cast_to_int(const char *arg) {
    char *endptr;

    long val = strtol(arg, &endptr, 10);

    if (*endptr != '\0' || val <= 0 || val > 65535) {
        return DEFAULT_PORT;
    }

    return (int)val;
}

void send_response(int client_fd,
                   const char *status,
                   const char *content_type,
                   const char *body) {
    char response[RESPONSE_SIZE];

    int written = snprintf(
        response,
        sizeof(response),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        status,
        content_type,
        body
    );

    if (written < 0) {
        perror("snprintf");
        return;
    }

    send(client_fd, response, strlen(response), 0);
}

int main(int argc, char *argv[]) {
    if (argc < 3 || strcmp(argv[1], "--port") != 0) {
        fprintf(stderr, "Usage: %s --port <port>\n", argv[0]);
        return 1;
    }

    int port = cast_to_int(argv[2]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;

    if (setsockopt(
            sockfd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &opt,
            sizeof(opt)) < 0) {

        perror("setsockopt");
        close(sockfd);
        return 1;
    }

    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(
            sockfd,
            (struct sockaddr *)&addr,
            sizeof(addr)) < 0) {

        perror("bind");
        close(sockfd);
        return 1;
    }

    if (listen(sockfd, SOMAXCONN) < 0) {
        perror("listen");
        close(sockfd);
        return 1;
    }

    printf("Listening on port %d\n", port);

    while (1) {
        socklen_t addr_len = sizeof(addr);

        int client_fd =
            accept(
                sockfd,
                (struct sockaddr *)&addr,
                &addr_len
            );

        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        char http_request[HTTP_REQUEST_SIZE];

        memset(http_request, 0, sizeof(http_request));

        ssize_t bytes_received =
            recv(
                client_fd,
                http_request,
                sizeof(http_request) - 1,
                0
            );

        if (bytes_received < 0) {
            perror("recv");
            close(client_fd);
            continue;
        }

        if (bytes_received == 0) {
            close(client_fd);
            continue;
        }

        char verb[HTTP_REQUEST_VERB_MAX_SIZE] = {0};
        char path[HTTP_REQUEST_PATH_MAX_SIZE] = {0};

        identify_request(http_request, verb, path);

        if (strcmp(verb, "GET") != 0) {
            send_response(
                client_fd,
                "405 METHOD NOT ALLOWED",
                "text/plain; charset=UTF-8",
                "405 Method Not Allowed"
            );

            close(client_fd);
            continue;
        }

        if (strstr(path, "..") != NULL) {
            send_response(
                client_fd,
                "403 FORBIDDEN",
                "text/plain; charset=UTF-8",
                "403 Forbidden"
            );

            close(client_fd);
            continue;
        }

        char full_path[512];

        if (strcmp(path, "/") == 0) {
            snprintf(
                full_path,
                sizeof(full_path),
                "./index.html"
            );
        } else {
            snprintf(
                full_path,
                sizeof(full_path),
                ".%s",
                path
            );
        }

        char *content = load_file(full_path);

        if (content == NULL) {
            send_response(
                client_fd,
                "404 NOT FOUND",
                "text/plain; charset=UTF-8",
                "404 Not Found"
            );

            close(client_fd);
            continue;
        }

        send_response(
            client_fd,
            "200 OK",
            "text/html; charset=UTF-8",
            content
        );

        free(content);

        close(client_fd);
    }

    close(sockfd);

    return 0;
}
