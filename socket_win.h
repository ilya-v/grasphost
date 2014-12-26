#ifndef _socket_win__h__
#define _socket_win__h__
#ifdef _WIN32

#include <memory>
#include <winsock2.h>

#include "socket.h"

struct ServerSocket::Impl;

struct ClientSocket::Impl
{
    bool Send(const void * data, const unsigned size);
    void Close();

    Impl(SOCKET client_socket) : client_socket(client_socket)  {}
    ~Impl() { Close();  }

private:
    SOCKET client_socket = INVALID_SOCKET;
};


struct ServerSocket::Impl
{
    ~Impl();
    bool Init(const char *port_name);
    std::unique_ptr<ClientSocket::Impl> Accept();
    bool IsGood() const { return server_socket != INVALID_SOCKET; }

private:
    SOCKET server_socket = INVALID_SOCKET;
    void Close();
};


#endif
#endif
