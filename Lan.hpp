// Lan.hpp
#pragma once

#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <iostream>
#include <vector>

#include "Thread.hpp"

#ifdef _WIN32
    typedef int socklen_t;
    #define close_socket closesocket
#else
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <unistd.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    typedef int SOCKET;
    #define close_socket close
#endif

#define UDP_PORT 50000
#define TCP_PORT 50001
#define BUFFER_SIZE 1024

class Lan {
public:
    std::map<std::string, std::chrono::steady_clock::time_point> peers;
    std::mutex peer_mutex;
    bool running = true;

    void init() {
#ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    }

    void cleanup() {
#ifdef _WIN32
        WSACleanup();
#endif
    }

    void start_udp_broadcast() {
        Thread::run([&]() {
            SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
            int broadcast = 1;
            setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast));

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(UDP_PORT);
            addr.sin_addr.s_addr = INADDR_BROADCAST;

            std::string msg = "LAN_CHAT_DISCOVERY";

            while (running) {
                sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&addr, sizeof(addr));
                Thread::sleep_ms(2000);
            }

            close_socket(sock);
        });
    }

    void start_udp_listener() {
        Thread::run([&]() {
            SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(UDP_PORT);
            addr.sin_addr.s_addr = INADDR_ANY;

            if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
                std::cerr << "UDP bind failed\n";
                return;
            }

            char buffer[BUFFER_SIZE];
            sockaddr_in sender{};
            socklen_t sender_len = sizeof(sender);

            while (running) {
                int len = recvfrom(sock, buffer, BUFFER_SIZE - 1, 0, (sockaddr*)&sender, &sender_len);
                if (len > 0) {
                    buffer[len] = '\0';
                    std::string ip = inet_ntoa(sender.sin_addr);
                    std::lock_guard<std::mutex> lock(peer_mutex);
                    peers[ip] = std::chrono::steady_clock::now();
                }
            }

            close_socket(sock);
        });
    }

    void start_tcp_listener() {
        Thread::run([&]() {
            SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(TCP_PORT);
            addr.sin_addr.s_addr = INADDR_ANY;

            if (bind(server, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR || listen(server, 5) == SOCKET_ERROR) {
                std::cerr << "TCP server failed\n";
                return;
            }

            while (running) {
                sockaddr_in client{};
                socklen_t len = sizeof(client);
                SOCKET client_sock = accept(server, (sockaddr*)&client, &len);
                if (client_sock != INVALID_SOCKET) {
                    char buffer[BUFFER_SIZE];
                    int read = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
                    if (read > 0) {
                        buffer[read] = '\0';
                        std::string sender_ip = inet_ntoa(client.sin_addr);
                        std::cout << "\n[" << sender_ip << "]: " << buffer << "\n> ";
                        std::cout.flush();
                    }
                    close_socket(client_sock);
                }
            }

            close_socket(server);
        });
    }

    void send_to_peer(const std::string& ip, const std::string& msg) {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(TCP_PORT);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        if (connect(sock, (sockaddr*)&addr, sizeof(addr)) != SOCKET_ERROR) {
            send(sock, msg.c_str(), msg.size(), 0);
        } else {
            std::cerr << "Could not connect to " << ip << "\n";
        }

        close_socket(sock);
    }

    void start_peer_cleanup_thread(int timeout_seconds = 6) {
        Thread::run([&, timeout_seconds]() {
            while (running) {
                {
                    std::lock_guard<std::mutex> lock(peer_mutex);
                    auto now = std::chrono::steady_clock::now();
                    for (auto it = peers.begin(); it != peers.end(); ) {
                        if (std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count() > timeout_seconds) {
                            std::cout << "Removing inactive peer: " << it->first << "\n";
                            it = peers.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
                Thread::sleep_ms(2000);
            }
        });
    }
};
