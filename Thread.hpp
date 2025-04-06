// Thread.hpp
#pragma once

#ifdef _WIN32
    #include <winsock2.h>
    #include <windows.h>
    #include <process.h>
    #define THREAD_RETURN unsigned __stdcall
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <pthread.h>
    #include <unistd.h>
    #define THREAD_RETURN void*
#endif

#include <functional>
#include <thread>

class Thread {
public:
    template <typename Callable>
    static void run(Callable&& func) {
        std::thread(std::forward<Callable>(func)).detach();
    }

    static void sleep_ms(int ms) {
#ifdef _WIN32
        Sleep(ms);
#else
        usleep(ms * 1000);
#endif
    }
};
