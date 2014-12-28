#ifndef __socket_posix__h__
#define __socket_posix__h__

#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(__CYGWIN__) 

#include <unistd.h>
#if defined _POSIX_VERSION

#include "socket.h"
#include <memory>

struct ClientSocket::Impl
{
    bool Send(const void * data, const unsigned size);
    void Close();

    Impl(int client_socket) : client_socket(client_socket)  {}
    ~Impl() { Close(); }

private:
    int client_socket = -1;
};


struct ServerSocket::Impl
{
    ~Impl();
    bool Init(const char *port_name);
    std::unique_ptr<ClientSocket::Impl> Accept();
    bool IsGood() const { return server_socket >= 0; }

private:
    int server_socket = -1;
    void Close();
};




#endif
#endif
#endif
