// ChatApp.cpp

#include "Lan.hpp"
#include "Thread.hpp"
#include <iostream>

int main() {
    Lan lan;
    lan.init();

    lan.start_udp_broadcast();
    lan.start_udp_listener();
    lan.start_tcp_listener();
    lan.start_peer_cleanup_thread();

    while (lan.running) {
        std::cout << "\n--- LAN Chat ---\n";
        std::cout << "Discovered Peers:\n";

        {
            std::lock_guard<std::mutex> lock(lan.peer_mutex);
            int i = 1;
            for (auto& [ip, _] : lan.peers) {
                std::cout << i++ << ". " << ip << "\n";
            }
        }

        std::cout << "Enter peer number to chat, 0 to refresh, -1 to quit: ";
        int choice;
        std::cin >> choice;

        if (choice == -1) {
            lan.running = false;
            break;
        }

        std::string selected_ip;
        {
            std::lock_guard<std::mutex> lock(lan.peer_mutex);
            int i = 1;
            for (auto& [ip, _] : lan.peers) {
                if (i == choice) {
                    selected_ip = ip;
                    break;
                }
                i++;
            }
        }

        if (selected_ip.empty()) {
            std::cout << "Invalid choice.\n";
            continue;
        }

        std::cin.ignore();
        std::string msg;
        std::cout << "Enter message: ";
        std::getline(std::cin, msg);
        lan.send_to_peer(selected_ip, msg);
    }

    lan.cleanup();
    std::cout << "Goodbye!\n";
    return 0;
}
