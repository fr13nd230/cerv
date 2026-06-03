#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_PORT 8080
#define HTTP_REQUEST_SIZE 2048
#define HTTP_REQUEST_PATH_MAX_SIZE 255
#define HTTP_REQUEST_VERB_MAX_SIZE 7

void identify_request(char http_request[], char *verb, char *path) {
  char *token = strtok(http_request, " ");
  if (token != NULL) {
    strcpy(verb, token);
  }
  token = strtok(NULL, " ");
  if (token != NULL) {
    strcpy(path, token);
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

int main(int argc, char *argv[]) {
  if (argc < 3 || strcmp(argv[1], "--port") != 0) {
    printf("Invalid command usage: missing --port argument.");
    exit(1);
  }

  struct sockaddr_in addr;
  int port = cast_to_int(argv[2]);

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("Failed to create a socket");
    exit(1);
  }

  int opt = 1;
  int re_use_addr =
      setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (re_use_addr < 0) {
    perror("Failed to re use the same address");
    close(sockfd);
    exit(1);
  }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  int res = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
  if (res < 0) {
    perror("Failed to listen on sepecified port");
    close(sockfd);
    exit(1);
  }

  int l_res = listen(sockfd, SOMAXCONN);
  if (l_res < 0) {
    perror("Failed to listen to the server");
    close(sockfd);
    exit(1);
  }

  int acceptfd;
  char verb[HTTP_REQUEST_VERB_MAX_SIZE], path[HTTP_REQUEST_PATH_MAX_SIZE];
  socklen_t addr_len = sizeof(addr);
  char http_request[HTTP_REQUEST_SIZE];
  const char *http_response = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/plain; charset=UTF-8\r\n"
                              "Connection: close\r\n"
                              "\r\n"
                              "C TCP Server\n"
                              "Hello from a raw C TCP Socket!\n";

  while (1) {
    acceptfd = accept(sockfd, (struct sockaddr *)&addr, &addr_len);
    if (acceptfd < 0) {
      perror("Failed to accept incoming connections");
      close(sockfd);
      exit(1);
    }

    memset(http_request, 0, HTTP_REQUEST_SIZE);
    ssize_t bytes_received =
        recv(acceptfd, http_request, sizeof(http_request) - 1, 0);

    if (bytes_received < 0) {
      perror("Receiving messages has failed");
      break;
    } else if (bytes_received == 0) {
      printf("Client has disconnected.");
      break;
    }

    identify_request(http_request, verb, path);
    if (strcmp(verb, "GET") == 0) {
      printf("GET request has being received at: %s\n", path);
    }

    int send_res = send(acceptfd, http_response, strlen(http_response) - 1, 0);
    if (send_res < 0) {
      perror("Failed to send response to client");
    }

    close(acceptfd);
    acceptfd = -1;
  }

  close(acceptfd);
  close(sockfd);
  return 0;
}
