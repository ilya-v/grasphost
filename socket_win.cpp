#include "socket_win.h"
#include "socket.h"

#ifdef _WIN32

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include <iostream>
#include "utils.h"

#pragma comment(lib, "Ws2_32.lib")

bool ClientSocket::Impl::Send(const void * data, const unsigned size)
{
    return SOCKET_ERROR != send(client_socket, (const char*)data, size, 0);
}

void  ClientSocket::Impl::Close()
{
    if (client_socket == INVALID_SOCKET)  return;
    
    shutdown(client_socket, SD_SEND);
    closesocket(client_socket);
    client_socket = INVALID_SOCKET;
}

struct WSAInitializer {
    bool is_good = false;

    WSAInitializer() {
        WSADATA wsaData = {};
        const int wsa_init_res = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (!!wsa_init_res)
            std::clog << "WSA Startup Error" << wsa_init_res << std::endl;
        is_good = !wsa_init_res;
    }

    ~WSAInitializer(){ WSACleanup(); }
};

ServerSocket::Impl::~Impl() { Close(); }

void ServerSocket::Impl::Close()
{
    if (INVALID_SOCKET != server_socket) closesocket(server_socket);
    server_socket = INVALID_SOCKET;
}

bool ServerSocket::Impl::Init(const char *port_name)
{
    static WSAInitializer  wsa_initializer;
    if (!wsa_initializer.is_good)   return false;

    Close();

    using addrinfo_t = struct addrinfo;
    std::unique_ptr<addrinfo_t, decltype(&freeaddrinfo)> addr(nullptr, &freeaddrinfo);
    {
        addrinfo_t *p_addr = nullptr, hints = {};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        const int addr_res = getaddrinfo(nullptr, port_name, &hints, &p_addr);
        if (!!addr_res) {
            std::cout << "getaddrinfo Error" << addr_res << std::endl;
            return false;
        }
        addr = decltype(addr)(p_addr, &freeaddrinfo); 
    }    

    server_socket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (server_socket == INVALID_SOCKET) {
        std::cout << "Error creating socket: " << WSAGetLastError() << std::endl;
        return false;
    }

    const int bind_res = bind(server_socket, addr->ai_addr, (int)addr->ai_addrlen);
    if (bind_res == SOCKET_ERROR) {
        std::cout << "Error binding socket: " << WSAGetLastError() << std::endl;
        Close();
        return false;
    }

    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "Listen failed with error: " << WSAGetLastError() << std::endl; 
        Close();
        return false;
    }

    return true;
}

std::unique_ptr<ClientSocket::Impl> ServerSocket::Impl::Accept()
{
    SOCKET client_socket = accept(server_socket, nullptr, nullptr);
    return (client_socket != INVALID_SOCKET) ? make_unique<ClientSocket::Impl>(client_socket) : nullptr;
}


#endif