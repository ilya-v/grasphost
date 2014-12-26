#include "socket.h"

#include "socket_win.h"
#include "utils.h"

ClientSocket::ClientSocket(std::unique_ptr<ClientSocket::Impl> &p) : p_impl(std::move(p))  {}
ClientSocket::~ClientSocket() {}

bool ClientSocket::IsGood() const { return !! p_impl; }

void ClientSocket::Send(const void *data, const unsigned size)
{
    if (p_impl && !p_impl->Send(data, size))
        Close();
}

void ClientSocket::Close() { p_impl.reset(); }


ServerSocket::ServerSocket() : p_impl(make_unique<ServerSocket::Impl>()) {}
ServerSocket::~ServerSocket() {}

bool ServerSocket::Init(const char *port_name) { return p_impl->Init(port_name); }
ClientSocket ServerSocket::Accept() { return ClientSocket{ p_impl->Accept() }; }
bool ServerSocket::IsGood() const { return p_impl->IsGood(); }
