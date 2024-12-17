#include "server.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static volatile sig_atomic_t keep_running = 1;

static void handle_signal(int signum) {
    (void)signum;
    keep_running = 0;
}

int main(void) {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    struct MHD_Daemon *daemon = server_start(8080);
    if (!daemon) {
        fprintf(stderr, "Failed to start server\n");
        return EXIT_FAILURE;
    }

    printf("Server is running at http://localhost:8080\n");
    printf("Press Ctrl+C to stop...\n");

    while (keep_running) {
        sleep(1);
    }

    server_stop(daemon);
    return EXIT_SUCCESS;
}
