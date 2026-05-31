#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    int port = cast_to_int(argv[2]);
    printf("Argument: %d", port);

    return 0;
}
