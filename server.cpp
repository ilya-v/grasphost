#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <atomic>

#include "server.h"

#pragma comment(lib, "Ws2_32.lib")

namespace DataServerImpl {

    struct  DataServer {

        bool IsGood() const { return client_thread.joinable(); }

        std::string  port = "11003";

        std::queue<std::string>     data_queue;
        std::mutex                  data_mutex;
        std::condition_variable     data_cv;
        std::thread                 client_thread;
        std::atomic_bool            is_client_connected;
        HANDLE                      data_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        SOCKET  server_socket = INVALID_SOCKET;

        void SendData(const std::string &data)
        {           
            if (!is_client_connected || !IsGood())
                return;
            std::lock_guard<std::mutex> data_guard(data_mutex);
            data_queue.push(data);
            SetEvent(data_event);
        }

        DataServer() {
            WSADATA wsaData = {};

            const int wsa_init_res = WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (!!wsa_init_res) {
                std::cout << "WSA Startup Error" << wsa_init_res << std::endl;
                return;
            }

            struct addrinfo *addr = nullptr, *ptr = nullptr, hints = {};
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
            hints.ai_flags = AI_PASSIVE;

            const int addr_res = getaddrinfo(nullptr, port.c_str(), &hints, &addr);
            if (!!addr_res) {
                std::cout << "getaddrinfo Error" << addr_res << std::endl;
                WSACleanup();
                return;
            }

            server_socket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
            if (server_socket == INVALID_SOCKET) {
                std::cout << "Error creating socket: " << WSAGetLastError() << std::endl;
                freeaddrinfo(addr);
                WSACleanup();
                return;
            }

            const int bind_res = bind(server_socket, addr->ai_addr, (int)addr->ai_addrlen);
            if (bind_res == SOCKET_ERROR) {
                std::cout << "Error binding socket: " << WSAGetLastError() << std::endl;
                freeaddrinfo(addr);
                closesocket(server_socket);
                server_socket = INVALID_SOCKET;
                WSACleanup();
                return;
            }

            freeaddrinfo(addr);

            if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
                std::cout << "Listen failed with error: " << WSAGetLastError() << std::endl;
                closesocket(server_socket);
                server_socket = INVALID_SOCKET;
                WSACleanup();
                return;
            }
            is_client_connected = false;
            client_thread = std::thread(&DataServer::workerThread, this);
        }

        std::string  wipeQueue(bool & stop_flag)
        {
            WaitForSingleObject(data_event, INFINITE);
            std::string  data_string;
            std::lock_guard<std::mutex> data_lock(data_mutex);
            for (; !data_queue.empty(); data_queue.pop())
            {
                stop_flag = data_queue.front().empty();
                data_string += data_queue.front();
            }
            return data_string;
        }


        void workerThread()
        {        
            for (bool stop_flag = false; !stop_flag && server_socket != INVALID_SOCKET; Sleep(1000))
            {
                is_client_connected = false;
                for (SOCKET client_socket = accept(server_socket, nullptr, nullptr); client_socket != INVALID_SOCKET && server_socket != INVALID_SOCKET;)
                {   
                    is_client_connected = true;
                    std::string  data_string = wipeQueue(stop_flag);
                    const int send_res = send(client_socket, data_string.c_str(), data_string.length(), 0);
                    if (send_res == SOCKET_ERROR || stop_flag)
                    {
                        shutdown(client_socket, SD_SEND);
                        closesocket(client_socket);
                        client_socket = INVALID_SOCKET;
                        is_client_connected = false;
                    }
                }
            }
        }

        ~DataServer()
        {
            closesocket(server_socket);
            server_socket = INVALID_SOCKET;

            if (client_thread.joinable())
            {
                SendData(std::string());
                client_thread.join();
            }
            WSACleanup();
            CloseHandle(data_event);
        }
    };
}

void DataServer::SendData(const std::string &data) { data_server->SendData(data); }
bool DataServer::IsGood() const { return data_server->IsGood(); }
DataServer::DataServer() { data_server.reset(new DataServerImpl::DataServer); }
DataServer::~DataServer() {}
