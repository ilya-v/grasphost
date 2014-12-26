#ifndef _socket__h__
#define _socket__h__

#include <memory>

struct ClientSocket
{
    ~ClientSocket();
    bool IsGood() const;
    void Send(const void *data, const unsigned size);
    void Close();

private:
    struct Impl;
    std::unique_ptr<Impl> p_impl;

    ClientSocket(std::unique_ptr<ClientSocket::Impl> & p);
    friend struct ServerSocket;
};


struct ServerSocket
{
    ServerSocket();
    ~ServerSocket();

    bool IsGood() const;
    bool Init(const char *port_name);
    ClientSocket Accept();

private:
    struct Impl;
    friend struct ClientSocket::Impl;
    std::unique_ptr<Impl> p_impl;
};

#endif