#include "socket_posix.h"

#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(__CYGWIN__)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string>
#include <iostream>
#include "utils.h"

namespace {
    void close_socket(int &fd_socket) {
        if (fd_socket < 0)  return;    
        close(fd_socket);
        fd_socket = -1;
    }

    const int port_num = 11003;
}


bool ClientSocket::Impl::Send(const void * data, const unsigned size)
{
    return size == send(client_socket, (const char*)data, size, 0);
}

void  ClientSocket::Impl::Close()  { close_socket(client_socket); }
ServerSocket::Impl::~Impl() { Close(); }

void ServerSocket::Impl::Close()   { close_socket(server_socket); }

bool ServerSocket::Impl::Init(const char *port_name)
{
    Close();

    if (server_socket = socket(AF_INET, SOCK_STREAM, 0) < 0) {
        std::cout << "Error creating socket: " << errno << std::endl;
        return false;
    }

    int dummy = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &dummy, sizeof(int));

    struct sockaddr_in serv_addr = {0, };
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_num);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(server_socket, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "Error binding socket: " << errno << std::endl;
        Close();
        return false;
    }

    if (listen(server_socket, 5) < 0) {
        std::cout << "Listen failed with error: " << errno << std::endl;
        Close();
        return false;
    }

    return true;
}

std::unique_ptr<ClientSocket::Impl> ServerSocket::Impl::Accept()
{
    const int client_socket = accept(server_socket, nullptr, nullptr);
    return (client_socket >= 0) ? make_unique<ClientSocket::Impl>(client_socket) : nullptr;
}


#endif