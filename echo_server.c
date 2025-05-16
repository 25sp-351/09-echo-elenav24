#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <limits.h>

#define DEFAULT_PORT 80
#define BACKLOG      10
#define BUFFER_SIZE     1024

typedef struct {
    int  sockfd;
    int  verbose;
} client_args_t;

void *handle_client(void *arg) {
    client_args_t *cargs = arg;
    FILE *fp = fdopen(cargs->sockfd, "r+");
    if (!fp) {
        close(cargs->sockfd);
        free(cargs);
        return NULL;
    }

    char buf[BUFFER_SIZE];
    while (fgets(buf, sizeof(buf), fp)) {
        size_t len = strlen(buf);
        if (cargs->verbose) {
            printf("Received: %s", buf);
            fflush(stdout);
        }
        if (fwrite(buf, 1, len, fp) < len) {
            break;
        }
        fflush(fp);
    }

    fclose(fp);
    free(cargs);
    return NULL;
}

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    int verbose = 0;

    // -p and -v arguments
    for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-p") == 0 && i+1 < argc) {
        errno = 0;
        char *endptr;
        long val = strtol(argv[++i], &endptr, 10);

        if (errno == ERANGE || val < 1 || val > 65535) {
            fprintf(stderr, "Invalid port number (out of range): %s\n", argv[i]);
            exit(EXIT_FAILURE);
        }
        if (*endptr != '\0') {
            fprintf(stderr, "Port contains non-numeric characters: %s\n", argv[i]);
            exit(EXIT_FAILURE);
        }

        port = (int)val;
    }
    else if (strcmp(argv[i], "-v") == 0) {
        verbose = 1;
    }
    else {
        fprintf(stderr,
            "Usage: %s [-p port] [-v]\n"
            "  -p [port]    Listen on this port (default %d)\n"
            "  -v           Verbose: print each received line\n",
            argv[0], DEFAULT_PORT);
        exit(EXIT_FAILURE);
    }
}

    signal(SIGPIPE, SIG_IGN);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in srv = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port        = htons(port),
    };

    if (bind(listenfd, (struct sockaddr*)&srv, sizeof(srv)) < 0) {
        perror("bind");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    if (listen(listenfd, BACKLOG) < 0) {
        perror("listen");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    if (verbose) {
        printf("Echo server listening on port %d (verbose)\n", port);
    } else {
        printf("Echo server listening on port %d\n", port);
    }

    while (1) {
        struct sockaddr_in cli;
        socklen_t         cli_len = sizeof(cli);
        int               clientfd;

        clientfd = accept(listenfd, (struct sockaddr*)&cli, &cli_len);
        if (clientfd < 0) {
            perror("accept");
            continue;
        }

        char addrbuf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cli.sin_addr, addrbuf, sizeof(addrbuf));
        printf("Connection from %s:%d\n",
               addrbuf, ntohs(cli.sin_port));

        pthread_t tid;
        client_args_t *cargs = malloc(sizeof(*cargs));
        cargs->sockfd  = clientfd;
        cargs->verbose = verbose;

        if (pthread_create(&tid, NULL, handle_client, cargs) != 0) {
            perror("pthread_create");
            close(clientfd);
            free(cargs);
        } else {
            pthread_detach(tid);
        }
    }

    close(listenfd);
    return 0;
}